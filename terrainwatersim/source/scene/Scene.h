#pragma once

#include <gl/resources/UniformBuffer.h>
#include <gl/ShaderObject.h>

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
  void InitGlobalUBO();
  void RecreateScreenBuffers();
  void UpdateDepthMaxMap();

  gl::FramebufferObject* m_linearHDRFramebuffer;
  gl::Texture2D* m_linearHDRBuffer;
  gl::Texture2D* m_depthBuffer; ///< Scene depth buffer. Contains mipmaps to store maxmaps.

  gl::Texture2D* m_linearHDRBuffer_Half;
  gl::FramebufferObject* m_linearHDRFramebuffer_Half; ///< Has no depth.
  gl::Texture2D* m_depthBufferMaxMaps; ///< Maximum mipmaps for the scene depth buffer, starts with half size depth buffer.
  ezHybridArray<gl::FramebufferObject*, 16> m_depthBufferMaxMapFBOs; ///< A FBO for every miplevel of m_depthBufferMaxMap, from big to small (usual mip order).

  gl::Texture2D* m_lowresScreenColorTexture;
  gl::FramebufferObject* m_lowresScreenColorFBO;

  gl::ShaderObject m_maxMapGenStep;

  gl::ShaderObject m_copyShader;

  gl::UniformBuffer m_CameraUBO;
  gl::UniformBuffer m_TimeUBO;
  gl::UniformBuffer m_GlobalSceneInfo;

  class Terrain* m_terrain;
  class Background* m_pBackground;
  class PostProcessing* m_pPostProcessing;

  ezUniquePtr<gl::TimerQuery> m_pTerrainDrawTimer;
  ezUniquePtr<gl::TimerQuery> m_waterDrawTimer;
  ezUniquePtr<gl::TimerQuery> m_pSimulationTimer;

  ezUniquePtr<class AntTweakBarInterface> m_pUserInterface;

  ezUniquePtr<FreeCamera> m_pCamera;

  ezUniquePtr<gl::Font> m_pFont;
};

