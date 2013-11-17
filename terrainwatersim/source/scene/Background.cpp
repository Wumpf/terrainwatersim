#include "PCH.h"
#include "Background.h"
#include "gl/ScreenAlignedTriangle.h"


Background::Background(const std::shared_ptr<const gl::ScreenAlignedTriangle>& pScreenAlignedTriangle) :
  m_pScreenAlignedTriangle(pScreenAlignedTriangle)
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
  m_BackgroundShader.Activate();
  m_pScreenAlignedTriangle->Draw();
}