#include "PCH.h"
#include "Background.h"
#include "gl/ScreenAlignedTriangle.h"
#include "gl/resources/textures/TextureCube.h"
#include "gl/resources/FramebufferObject.h"


Background::Background(ezUInt32 cubemapResolution, GLenum format) :
  m_backgroundShader("renderSkybox"),
  m_scatteringShader("athmosphericScattering")
{
  EZ_LOG_BLOCK("Background");

  m_backgroundShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "screenTri.vert");
  m_backgroundShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "background.frag");
  m_backgroundShader.CreateProgram();

  m_scatteringShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "screenTri.vert");
  m_scatteringShader.AddShaderFromFile(gl::ShaderObject::ShaderType::GEOMETRY, "eachCubemapFace.geom");
  m_scatteringShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "athmosphericScattering.frag");
  m_scatteringShader.CreateProgram();

  m_pSkyboxCubemap = EZ_DEFAULT_NEW(gl::TextureCube)(cubemapResolution, format, 1);
  m_pSkyboxFrameBuffer = EZ_DEFAULT_NEW(gl::FramebufferObject)({ gl::FramebufferObject::Attachment(m_pSkyboxCubemap) });

  // who would dare to ever reset that oô
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}


Background::~Background(void)
{
  EZ_DEFAULT_DELETE(m_pSkyboxCubemap);
  EZ_DEFAULT_DELETE(m_pSkyboxFrameBuffer);
}

void Background::UpdateCubemap()
{
  m_pSkyboxFrameBuffer->Bind(true);

  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  m_scatteringShader.Activate();
  gl::ScreenAlignedTriangle::Draw();

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
}

void Background::Draw()
{
  glDepthFunc(GL_EQUAL);
  glDepthMask(GL_FALSE);
  m_backgroundShader.Activate();
  m_pSkyboxCubemap->Bind(0);
  gl::ScreenAlignedTriangle::Draw();
  glDepthFunc(GL_LESS);
}