#pragma once

namespace gl
{
  class ShaderObject;
  class FramebufferObject;
};

class PostProcessing
{
public:
  PostProcessing();
  ~PostProcessing();

  void ApplyAndRenderToBackBuffer(gl::FramebufferObject& hdrSceneBuffer);

private:
  gl::ShaderObject* m_pTonemapCombine;
};

