#include "PCH.h"
#include "PostProcessing.h"

#include <gl/ShaderObject.h>
#include <gl/resources/FrameBufferObject.h>
#include <gl/SamplerObject.h>
#include <gl/ScreenAlignedTriangle.h>

PostProcessing::PostProcessing()
{
  EZ_LOG_BLOCK("PostProcessing");

  m_pTonemapCombine = EZ_DEFAULT_NEW(gl::ShaderObject)("tonemapCombine");
  m_pTonemapCombine->AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "screenTri.vert");
  m_pTonemapCombine->AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "postpro/tonemapCombine.frag");
  m_pTonemapCombine->CreateProgram();
}


PostProcessing::~PostProcessing()
{
  EZ_DEFAULT_DELETE(m_pTonemapCombine);
}


void PostProcessing::ApplyAndRenderToBackBuffer(gl::FramebufferObject& hdrSceneBuffer)
{
  const gl::SamplerObject& pointSampler = gl::SamplerObject::GetSamplerObject(gl::SamplerObject::Desc(
    gl::SamplerObject::Filter::NEAREST, gl::SamplerObject::Filter::NEAREST, gl::SamplerObject::Filter::NEAREST, gl::SamplerObject::Border::CLAMP));

  auto hdrSceneColorTex = hdrSceneBuffer.GetColorAttachments()[0].pTexture;

  glEnable(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  gl::FramebufferObject::BindBackBuffer();
  glViewport(0, 0, hdrSceneColorTex->GetWidth(), hdrSceneColorTex->GetHeight());
  m_pTonemapCombine->Activate();
  pointSampler.BindSampler(0);
  hdrSceneColorTex->Bind(0);
  gl::ScreenAlignedTriangle::Draw();

  glDisable(GL_FRAMEBUFFER_SRGB);
}