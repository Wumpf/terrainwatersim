#pragma once

#include "gl/ShaderObject.h"
#include "gl/resources/UniformBuffer.h"

namespace gl
{
  class ScreenAlignedTriangle;
  class Texture2D;
};

class Terrain
{
public:
  Terrain();
  ~Terrain();
  
  void PerformSimulationStep();
  void Draw(const ezVec3& cameraPosition);
 
  const gl::ShaderObject& GetTerrainShader() { return m_terrainRenderShader; }
  
  float GetTerrainWorldSize() const             { return m_worldSize; }
  //void SetTerrainWorldSize(float worldSize)     { m_worldSize = worldSize; }    // Expected patch count changes!

  float GetMinBlockSizeWorld() const             { return m_minPatchSizeWorld; }
  //void SetMinBlockSizeWorld(float worldSize)     { m_minBlockSizeWorld = worldSize; } // Expected patch count changes!

  float GetPixelPerTriangle() const;
  void SetPixelPerTriangle(float pixelPerTriangle);

  bool GetAnisotropicFiltering() const { return m_anisotropicFiltering; }
  void SetAnisotrpicFiltering(bool anisotropicFiltering) { m_anisotropicFiltering = anisotropicFiltering; }

private:
  void CreateHeightmap();

  // Settings.
  float m_worldSize;
  float m_minPatchSizeWorld;
  float m_pixelPerTriangle;
  float m_heightScale;
  static const float m_maxTesselationFactor;

  ezUInt32 m_heightmapSize;

  bool m_anisotropicFiltering;


  // Graphics resources.
  class InstancedGeomClipMapping* m_pGeomClipMaps;

  gl::Texture2D* m_pTerrainData;
  gl::Texture2D* m_pWaterFlow;

    // Shader
  gl::ShaderObject m_updateFlowShader;
  gl::ShaderObject m_applyFlowShader;

  gl::ShaderObject m_waterRenderShader;
  gl::ShaderObject m_terrainRenderShader;

  gl::UniformBuffer m_landscapeInfoUBO;

  gl::SamplerId m_texturingSamplerObjectAnisotropic;
  gl::SamplerId m_texturingSamplerObjectTrilinear;

  ezUniquePtr<gl::Texture2D> m_pTextureGrassDiffuseSpec;
  ezUniquePtr<gl::Texture2D> m_pTextureStoneDiffuseSpec;
  ezUniquePtr<gl::Texture2D> m_pTextureGrassNormalHeight;
  ezUniquePtr<gl::Texture2D> m_pTextureStoneNormalHeight;
};

