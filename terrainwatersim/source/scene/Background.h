#pragma once

#include "gl/ShaderObject.h"

namespace gl
{
   class TextureCube;
   class FramebufferObject;
};

class Background
{
public:
  Background(ezUInt32 cubemapResolution, GLenum format = GL_RGBA16F);
  ~Background();

  void UpdateCubemap();
  void Draw();

  const gl::ShaderObject& GetBackgroundShader() { return m_backgroundShader; }
  const gl::ShaderObject& GetScatteringShader() { return m_scatteringShader; }

private:
  gl::TextureCube* m_pSkyboxCubemap;
  gl::FramebufferObject* m_pSkyboxFrameBuffer;

  gl::ShaderObject m_scatteringShader;
  gl::ShaderObject m_backgroundShader;
};

