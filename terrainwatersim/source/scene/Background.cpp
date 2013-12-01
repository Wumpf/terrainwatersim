#include "PCH.h"
#include "Background.h"
#include "gl/ScreenAlignedTriangle.h"


Background::Background()
{
  m_BackgroundShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "screenTri.vert");
  m_BackgroundShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "background.frag");
  m_BackgroundShader.CreateProgram();
}


Background::~Background(void)
{
}

void Background::Draw()
{
  glDepthFunc(GL_EQUAL);
  glDepthMask(GL_FALSE);
  m_BackgroundShader.Activate();
  gl::ScreenAlignedTriangle::Draw();
  glDepthFunc(GL_LESS);
}