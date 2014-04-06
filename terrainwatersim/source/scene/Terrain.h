#pragma once

#include "gl/ShaderObject.h"
#include "gl/resources/UniformBuffer.h"

namespace gl
{
  class ScreenAlignedTriangle;
  class Texture2D;
  class TextureCube;
  class FramebufferObject;
  class SamplerObject;
};

class Terrain
{
public:
  Terrain(const ezSizeU32& screenSize);
  ~Terrain();

  /// Updates simulation.
  void PerformSimulationStep(ezTime lastFrameDuration);

  void UpdateVisibilty(const ezVec3& cameraPosition);
  void DrawTerrain();
  
  /// \param lowresSceneCopy       Low resolution version of the framebuffer.
  /// \param depthBufferMaxMaps    Maxmapped depth buffer starting with resolution of lowresSceneCopy.
  void DrawWater(gl::Texture2D& lowresSceneCopy, gl::Texture2D& depthBufferMaxMaps, gl::TextureCube& reflectionCubemap);


  /// Creates heightmap from noise and resets flow.
  void CreateHeightmapFromNoiseAndResetSim();

  // Brush functions

  /// Adds water at a given position with a radial falloff.
  /// \param strength   Intensity scaler to the brush.
  void ApplyRadialWaterBrush(ezVec2 worldPositionXZ, float strength);

  // Getter & Setter

  // General
  const gl::ShaderObject& GetTerrainShader() { return m_terrainRenderShader; }
  const gl::ShaderObject& GetWaterShader() { return m_waterRenderShader; }

  float GetTerrainWorldSize() const             { return m_gridWorldSize; }
  //void SetTerrainWorldSize(float worldSize)     { m_worldSize = worldSize; }    // Expected patch count changes!
  float GetMinBlockSizeWorld() const             { return m_minPatchSizeWorld; }
  //void SetMinBlockSizeWorld(float worldSize)     { m_minBlockSizeWorld = worldSize; } // Expected patch count changes!

  // Rendering
  float GetPixelPerTriangle() const;
  void SetPixelPerTriangle(float pixelPerTriangle);

  bool GetAnisotropicFiltering() const { return m_anisotropicFiltering; }
  void SetAnisotropicFiltering(bool anisotropicFiltering) { m_anisotropicFiltering = anisotropicFiltering; }

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
  void UpdateSimulationParameters();

  // Settings

  // general
  float m_gridWorldSize;
  float m_minPatchSizeWorld;
  float m_heightScale;
  ezUInt32 m_gridResolution;

  // simulation
  ezTime m_simulationStepLength;
  float m_flowDamping;
  float m_flowAcceleration;

  // rendering
  float m_pixelPerTriangle;
  static const float m_maxTesselationFactor;
  bool m_anisotropicFiltering;
    // Waterflow
  ezTime m_waterDistortionLayerBlendInterval;

  // State
  ezTime m_timeSinceLastSimulationStep;


  // Graphics resources.
  class InstancedGeomClipMapping* m_geomClipMaps;

  gl::Texture2D* m_terrainData;
  gl::Texture2D* m_waterOutgoingFlow;
  gl::Texture2D* m_waterFlowMap;

    // Shader
  gl::ShaderObject m_updateFlowShader;
  gl::ShaderObject m_applyFlowShader;
  gl::ShaderObject m_waterRenderShader;
  gl::ShaderObject m_terrainRenderShader;
  gl::ShaderObject m_copyShader;
  gl::ShaderObject m_waterBrushShader;

    // UBO
  gl::UniformBuffer m_landscapeInfoUBO;
  gl::UniformBuffer m_simulationParametersUBO;
  gl::UniformBuffer m_terrainRenderingUBO;
  gl::UniformBuffer m_waterRenderingUBO;
  gl::UniformBuffer m_brushUBO;

    // Samplers
  const gl::SamplerObject* m_texturingSamplerObjectAnisotropic;
  const gl::SamplerObject* m_texturingSamplerObjectTrilinear;
  const gl::SamplerObject* m_texturingSamplerObjectLinearClamp;
  const gl::SamplerObject* m_texturingSamplerObjectNearestRepeat;
  
    // Water Textures
  //gl::Texture2D* m_textureFoam;

    // Terrain Textures
  gl::Texture2D* m_textureGrassDiffuseSpec;
  gl::Texture2D* m_textureStoneDiffuseSpec;
  gl::Texture2D* m_textureGrassNormalHeight;
  gl::Texture2D* m_textureStoneNormalHeight;

  gl::Texture2D* m_waterNormalMap;
  gl::Texture2D* m_lowResNoise;
  gl::Texture2D* m_foamTexture;
};

