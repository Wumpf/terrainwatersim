#include "PCH.h"
#include "PostProcessing.h"

#include <gl/ShaderObject.h>
#include <gl/resources/FrameBufferObject.h>
#include <gl/SamplerObject.h>
#include <gl/ScreenAlignedTriangle.h>
#include <gl/resources/textures/Texture2D.h>

PostProcessing::PostProcessing(const ezSizeU32& screenSize) :
  m_pLuminanceLogFBO(NULL),
  m_pLuminanceLogTexture(NULL),
  m_luminanceReadIdx(0)
{
  EZ_LOG_BLOCK("PostProcessing");

  m_pTonemapCombine = EZ_DEFAULT_NEW(gl::ShaderObject)("tonemapCombine");
  m_pTonemapCombine->AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "screenTri.vert");
  m_pTonemapCombine->AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "postpro/tonemapCombine.frag");
  m_pTonemapCombine->CreateProgram();

  m_pLuminanceStart = EZ_DEFAULT_NEW(gl::ShaderObject)("luminanceStart");
  m_pLuminanceStart->AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "screenTri.vert");
  m_pLuminanceStart->AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "postpro/luminanceStart.frag");
  m_pLuminanceStart->CreateProgram();

  m_pLuminanceAdapt = EZ_DEFAULT_NEW(gl::ShaderObject)("luminanceAdapt");
  m_pLuminanceAdapt->AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "screenTri.vert");
  m_pLuminanceAdapt->AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "postpro/luminanceAdapt.frag");
  m_pLuminanceAdapt->CreateProgram();

  m_postProSettingsUBO.Init({ m_pTonemapCombine, m_pLuminanceAdapt, m_pLuminanceStart }, "PostProcessing");
  SetLuminanceAdaptationSpeed(5.0f);
  SetExposure(0.2f);

  for(unsigned int i = 0; i < 2; ++i)
  {
    m_pLuminanceFinalTexture[i] = EZ_DEFAULT_NEW(gl::Texture2D)(1, 1, GL_R32F, -1);
    m_pLuminanceFinalFBO[i] = EZ_DEFAULT_NEW(gl::FramebufferObject)({ gl::FramebufferObject::Attachment(m_pLuminanceFinalTexture[i]) });
  }
}

void PostProcessing::RecreateScreenSizeDependentTextures(const ezSizeU32& screenSize)
{
  if(m_pLuminanceLogFBO)
    EZ_DEFAULT_DELETE(m_pLuminanceLogFBO);
  if(m_pLuminanceLogTexture)
    EZ_DEFAULT_DELETE(m_pLuminanceLogTexture);
  
  m_pLuminanceLogTexture = EZ_DEFAULT_NEW(gl::Texture2D)(screenSize.width / 2, screenSize.height / 2, GL_R32F, -1);
  m_pLuminanceLogFBO = EZ_DEFAULT_NEW(gl::FramebufferObject)({ gl::FramebufferObject::Attachment(m_pLuminanceLogTexture) });
}


PostProcessing::~PostProcessing()
{
  EZ_DEFAULT_DELETE(m_pTonemapCombine);
  EZ_DEFAULT_DELETE(m_pLuminanceStart);
  EZ_DEFAULT_DELETE(m_pLuminanceAdapt);
  EZ_DEFAULT_DELETE(m_pLuminanceLogFBO);
  EZ_DEFAULT_DELETE(m_pLuminanceLogTexture);
  EZ_DEFAULT_DELETE(m_pLuminanceFinalTexture[0]);
  EZ_DEFAULT_DELETE(m_pLuminanceFinalTexture[1]);
  EZ_DEFAULT_DELETE(m_pLuminanceFinalFBO[0]);
  EZ_DEFAULT_DELETE(m_pLuminanceFinalFBO[1]);
}

void PostProcessing::SetExposure(float exposure)
{
  m_postProSettingsUBO["TonemapExposure"].Set(exposure);
}

void PostProcessing::SetLuminanceAdaptationSpeed(float luminanceAdaptationSpeed)
{
  m_luminanceAdaptationSpeed = luminanceAdaptationSpeed;
}

void PostProcessing::ApplyAndRenderToBackBuffer(ezTime lastFrameDuration, gl::FramebufferObject& hdrSceneBuffer)
{
  // Update timer and set UBO.
  m_postProSettingsUBO["LuminanceInterpolator"].Set(static_cast<float>(1.0f - ezMath::Exp(-lastFrameDuration.GetSeconds() * m_luminanceAdaptationSpeed)));
  m_postProSettingsUBO.BindBuffer(3);


  // Prepare sampler objects.
  const gl::SamplerObject& pointSampler = gl::SamplerObject::GetSamplerObject(gl::SamplerObject::Desc(
    gl::SamplerObject::Filter::NEAREST, gl::SamplerObject::Filter::NEAREST, gl::SamplerObject::Filter::NEAREST, gl::SamplerObject::Border::CLAMP));
  const gl::SamplerObject& linearSampler = gl::SamplerObject::GetSamplerObject(gl::SamplerObject::Desc(
    gl::SamplerObject::Filter::LINEAR, gl::SamplerObject::Filter::LINEAR, gl::SamplerObject::Filter::LINEAR, gl::SamplerObject::Border::CLAMP));

  auto hdrSceneColorTex = hdrSceneBuffer.GetColorAttachments()[0].pTexture;

  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  // Initial luminance extract pass
  m_pLuminanceLogFBO->Bind(true);
  m_pLuminanceStart->Activate();
  hdrSceneColorTex->Bind(0);
  linearSampler.BindSampler(0);
  gl::ScreenAlignedTriangle::Draw();

  // Luminance adaptation
  m_pLuminanceFinalFBO[1 - m_luminanceReadIdx]->Bind(true);
  m_pLuminanceLogTexture->GenMipMaps(); // create mipmaps now because before this texture was still bound.
  m_pLuminanceAdapt->Activate();
  m_pLuminanceLogTexture->Bind(0);
  m_pLuminanceFinalTexture[m_luminanceReadIdx]->Bind(1);
  pointSampler.BindSampler(0);
  pointSampler.BindSampler(1);
  gl::ScreenAlignedTriangle::Draw();

  // Combine and tonemap, perform gamma correction - uses old luminance to avoid unnecessary stalls
  glEnable(GL_FRAMEBUFFER_SRGB);

  gl::FramebufferObject::BindBackBuffer();
  glViewport(0, 0, hdrSceneColorTex->GetWidth(), hdrSceneColorTex->GetHeight());
  m_pTonemapCombine->Activate();
  hdrSceneColorTex->Bind(0);
  m_pLuminanceFinalTexture[m_luminanceReadIdx]->Bind(1);
  pointSampler.BindSampler(0);
  pointSampler.BindSampler(1);
  gl::ScreenAlignedTriangle::Draw();

  glDisable(GL_FRAMEBUFFER_SRGB);

  m_luminanceReadIdx = 1 - m_luminanceReadIdx;
}