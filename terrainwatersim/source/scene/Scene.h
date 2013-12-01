#pragma once

#include "gl/resources/UniformBuffer.h"

// declarations
namespace gl
{
  class Font;
  class UniformBuffer;
  class ScreenAlignedTriangle;
  class TimerQuery;
  class Texture2D;
  class FramebufferObject;
  class ShaderObject;
};
class FreeCamera;

/// The scene where all the rendering stuff happens
class Scene
{
public:
  Scene(const class RenderWindowGL& renderWindow);
  ~Scene();
  
  ezResult Update(ezTime lastFrameDuration);
  ezResult Render(ezTime lastFrameDuration);

  void RenderUI();

private:
  void InitConfig();
  void RecreateScreenBuffers();

  ezUniquePtr<gl::FramebufferObject> m_pLinearHDRFramebuffer;
  ezUniquePtr<gl::Texture2D> m_pLinearHDRBuffer;
  ezUniquePtr<gl::Texture2D> m_pDepthBuffer;

  ezUniquePtr<gl::ShaderObject> m_pCopyShader;

  gl::UniformBuffer m_CameraUBO;
  gl::UniformBuffer m_TimeUBO;
  gl::UniformBuffer m_GlobalSceneInfo;

  class Terrain* m_pTerrain;
  class Background* m_pBackground;

  ezUniquePtr<gl::TimerQuery> m_pTerrainDrawTimer;
  ezUniquePtr<gl::TimerQuery> m_pWaterDrawTimer;
  ezUniquePtr<gl::TimerQuery> m_pSimulationTimer;

  ezUniquePtr<class AntTweakBarInterface> m_UserInterface;

  ezUniquePtr<FreeCamera> m_pCamera;

  ezUniquePtr<gl::Font> m_pFont;
};

