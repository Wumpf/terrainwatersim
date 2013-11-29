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
  
  void PerformSimulationStep(ezTime lastFrameDuration);
  void Draw(const ezVec3& cameraPosition);
 
  // Getter & Setter

  const gl::ShaderObject& GetTerrainShader() { return m_terrainRenderShader; }

  float GetTerrainWorldSize() const             { return m_worldSize; }
  //void SetTerrainWorldSize(float worldSize)     { m_worldSize = worldSize; }    // Expected patch count changes!

  float GetMinBlockSizeWorld() const             { return m_minPatchSizeWorld; }
  //void SetMinBlockSizeWorld(float worldSize)     { m_minBlockSizeWorld = worldSize; } // Expected patch count changes!

  float GetPixelPerTriangle() const;
  void SetPixelPerTriangle(float pixelPerTriangle);

  bool GetAnisotropicFiltering() const { return m_anisotropicFiltering; }
  void SetAnisotrpicFiltering(bool anisotropicFiltering) { m_anisotropicFiltering = anisotropicFiltering; }

  float GetSimulationStepsPerSecond() const { return static_cast<float>(1.0f / m_simulationStepLength.GetSeconds() + 0.5f); }
  void SetSimulationStepsPerSecond(float simulationStepsPerSecond);

  float GetFlowDamping() const { return m_flowDamping; }
  void SetFlowDamping(float flowDamping);

  float GetFlowAcceleration() const { return m_flowAcceleration; }
  void SetFlowAcceleration(float flowAcceleration);

private:
  void CreateHeightmap();

  void UpdateSimulationParameters();

  // Settings

  // general
  float m_worldSize;
  float m_minPatchSizeWorld;
  float m_heightScale;
  ezUInt32 m_gridSize;

  // simulation
  ezTime m_simulationStepLength;
  float m_flowDamping;
  float m_flowAcceleration;

  // rendering
  float m_pixelPerTriangle;
  static const float m_maxTesselationFactor;
  bool m_anisotropicFiltering;

  // State
  ezTime m_timeSinceLastSimulationStep;


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
  gl::UniformBuffer m_simulationParametersUBO;

  gl::SamplerId m_texturingSamplerObjectAnisotropic;
  gl::SamplerId m_texturingSamplerObjectTrilinear;

  ezUniquePtr<gl::Texture2D> m_pTextureGrassDiffuseSpec;
  ezUniquePtr<gl::Texture2D> m_pTextureStoneDiffuseSpec;
  ezUniquePtr<gl::Texture2D> m_pTextureGrassNormalHeight;
  ezUniquePtr<gl::Texture2D> m_pTextureStoneNormalHeight;
};

