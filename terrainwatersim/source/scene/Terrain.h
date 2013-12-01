#pragma once

#include "gl/ShaderObject.h"
#include "gl/resources/UniformBuffer.h"

namespace gl
{
  class ScreenAlignedTriangle;
  class Texture2D;
  class TextureCube;
  class FramebufferObject;
};

class Terrain
{
public:
  Terrain(const ezSizeU32& screenSize);
  ~Terrain();
    
  void RecreateScreenSizeDependentTextures(const ezSizeU32& screenSize);

  void PerformSimulationStep(ezTime lastFrameDuration);

  void UpdateVisibilty(const ezVec3& cameraPosition);
  void DrawTerrain();
  /// Draws water. Needs to take a low resolution resolved copy of current framebuffer.
  void DrawWater(gl::FramebufferObject& sceneFBO, gl::TextureCube& reflectionCubemap);

  // Getter & Setter

  // General
  const gl::ShaderObject& GetTerrainShader() { return m_terrainRenderShader; }
  const gl::ShaderObject& GetWaterShader() { return m_waterRenderShader; }

  float GetTerrainWorldSize() const             { return m_worldSize; }
  //void SetTerrainWorldSize(float worldSize)     { m_worldSize = worldSize; }    // Expected patch count changes!
  float GetMinBlockSizeWorld() const             { return m_minPatchSizeWorld; }
  //void SetMinBlockSizeWorld(float worldSize)     { m_minBlockSizeWorld = worldSize; } // Expected patch count changes!

  // Rendering
  float GetPixelPerTriangle() const;
  void SetPixelPerTriangle(float pixelPerTriangle);

  bool GetAnisotropicFiltering() const { return m_anisotropicFiltering; }
  void SetAnisotrpicFiltering(bool anisotropicFiltering) { m_anisotropicFiltering = anisotropicFiltering; }

    // Terrain
  void SetTerrainFresnelReflectionCoef(float terrainFresnelReflectionCoef) { m_terrainRenderingUBO["FresnelReflectionCoefficient"].Set(terrainFresnelReflectionCoef); }
  void SetTerrainSpecularPower(float terrainSpecularPower) { m_terrainRenderingUBO["SpecularPower"].Set(terrainSpecularPower); }

    // Water
      // Appearance
  void SetWaterBigDepthColor(const ezVec3& bigDepthColor)    { m_waterRenderingUBO["BigDepthColor"].Set(bigDepthColor); }
  void SetWaterSurfaceColor(const ezVec3& surfaceColor)      { m_waterRenderingUBO["SurfaceColor"].Set(surfaceColor); }
  void SetWaterExtinctionCoefficients(const ezVec3& extinctionCoefficients) { m_waterRenderingUBO["ColorExtinctionCoefficient"].Set(extinctionCoefficients); }
  void SetWaterOpaqueness(float waterOpaqueness)             { m_waterRenderingUBO["Opaqueness"].Set(waterOpaqueness); }
      // Flow
  void SetWaterSpeedToNormalDistortion(float speedToNormalDistortion) { m_waterRenderingUBO["SpeedToNormalDistortion"].Set(speedToNormalDistortion); }
  void SetWaterDistortionLayerBlendInterval(ezTime interval)    { m_waterDistortionLayerBlendInterval = interval; }
  void SetWaterFlowDistortionStrength(float distortionStrength) { m_waterRenderingUBO["FlowDistortionStrength"].Set(distortionStrength); }
  void SetWaterNormalMapRepeat(float normalMapRepeat)           { m_waterRenderingUBO["NormalMapRepeat"].Set(normalMapRepeat); }

  // Simulation
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
  static const float m_refractionTextureSizeFactor;
    // Waterflow
  ezTime m_waterDistortionLayerBlendInterval;

  // State
  ezTime m_timeSinceLastSimulationStep;


  // Graphics resources.
  class InstancedGeomClipMapping* m_pGeomClipMaps;

  gl::Texture2D* m_pTerrainData;
  gl::Texture2D* m_pWaterOutgoingFlow;
  gl::Texture2D* m_pWaterFlowMap;

    // Shader
  gl::ShaderObject m_updateFlowShader;
  gl::ShaderObject m_applyFlowShader;
  gl::ShaderObject m_waterRenderShader;
  gl::ShaderObject m_terrainRenderShader;
  gl::ShaderObject m_copyShader;

    // UBO
  gl::UniformBuffer m_landscapeInfoUBO;
  gl::UniformBuffer m_simulationParametersUBO;
  gl::UniformBuffer m_terrainRenderingUBO;
  gl::UniformBuffer m_waterRenderingUBO;

    // Samplers
  gl::SamplerId m_texturingSamplerObjectDataGrids;
  gl::SamplerId m_texturingSamplerObjectAnisotropic;
  gl::SamplerId m_texturingSamplerObjectTrilinear;

    // Water Textures
  //ezUniquePtr<gl::Texture2D> m_pTextureFoam;

    // Terrain Textures
  ezUniquePtr<gl::Texture2D> m_pTextureGrassDiffuseSpec;
  ezUniquePtr<gl::Texture2D> m_pTextureStoneDiffuseSpec;
  ezUniquePtr<gl::Texture2D> m_pTextureGrassNormalHeight;
  ezUniquePtr<gl::Texture2D> m_pTextureStoneNormalHeight;

  ezUniquePtr<gl::Texture2D> m_pWaterNormalMap;
  ezUniquePtr<gl::Texture2D> m_pLowResNoise;
  ezUniquePtr<gl::Texture2D> m_pFoamTexture;

  ezUniquePtr<gl::Texture2D> m_pRefractionTexture;
  ezUniquePtr<gl::FramebufferObject> m_pRefractionFBO;  // needed for drawing to (resolve)
};

