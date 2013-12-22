#pragma once

#include <gl/resources/UniformBuffer.h>

namespace gl
{
  class ShaderObject;
  class FramebufferObject;
  class Texture2D;
};

class PostProcessing
{
public:
  PostProcessing(const ezSizeU32& screenSize);
  ~PostProcessing();

  void RecreateScreenSizeDependentTextures(const ezSizeU32& screenSize);
  void ApplyAndRenderToBackBuffer(ezTime lastFrameDuration, gl::FramebufferObject& hdrSceneBuffer);

  void SetExposure(float exposure);
  void SetLuminanceAdaptationSpeed(float luminanceAdaptationSpeed);


private:
  float m_luminanceAdaptationSpeed;

  gl::ShaderObject* m_pTonemapCombine;
  gl::ShaderObject* m_pLuminanceStart;
  gl::ShaderObject* m_pLuminanceAdapt;

  gl::UniformBuffer m_postProSettingsUBO;

  gl::FramebufferObject* m_pLuminanceLogFBO;
  gl::Texture2D* m_pLuminanceLogTexture;
  gl::FramebufferObject* m_pLuminanceFinalFBO[2];
  gl::Texture2D* m_pLuminanceFinalTexture[2];
  ezUInt32 m_luminanceReadIdx;
};

