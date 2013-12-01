#pragma once

#include "gl/ShaderObject.h"

namespace gl
{
   class ScreenAlignedTriangle;
};

class Background
{
public:
  Background();
  ~Background();

  void Draw();

  const gl::ShaderObject& GetShader() { return m_BackgroundShader; }

private:
  gl::ShaderObject m_BackgroundShader;
};

