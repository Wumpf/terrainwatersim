#pragma once

#include "gl/resources/UniformBuffer.h"
#include "gl/ShaderObject.h"

// declarations
namespace gl
{
  class Font;
  class UniformBuffer;
  class ScreenAlignedTriangle;
  class TimerQuery;
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
  void CreateVolumeTexture();

  gl::UniformBuffer m_CameraUBO;
  gl::UniformBuffer m_TimeUBO;
  gl::UniformBuffer m_GlobalSceneInfo;

  class Terrain* m_pTerrain;
  class Background* m_pBackground;

  ezUniquePtr<gl::TimerQuery> m_ExtractGeometryTimer;
  ezUniquePtr<gl::TimerQuery> m_DrawTimer;

  ezUniquePtr<class AntTweakBarInterface> m_UserInterface;

  std::shared_ptr<const gl::ScreenAlignedTriangle> m_pScreenAlignedTriangle;
  ezUniquePtr<FreeCamera> m_pCamera;

  ezUniquePtr<gl::Font> m_pFont;
};

