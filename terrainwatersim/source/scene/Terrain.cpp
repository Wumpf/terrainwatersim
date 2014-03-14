#include "PCH.h"
#include "Terrain.h"

#include "math/NoiseGenerator.h"

#include "InstancedGeomClipMapping.h"

#include "gl/ScreenAlignedTriangle.h"
#include "gl/SamplerObject.h"
#include "gl/resources/textures/Texture2D.h"
#include "gl/resources/textures/TextureCube.h"
#include "gl/resources/FramebufferObject.h"
#include "gl/GLUtils.h"

#include "..\config\GlobalCVar.h"
#include <Foundation\Math\Rect.h>
#include <Foundation\Time\Time.h>

const float Terrain::m_maxTesselationFactor = 64.0f;

Terrain::Terrain(const ezSizeU32& screenSize) :
  m_gridWorldSize(1024.0f),
  m_gridResolution(1024),
  m_minPatchSizeWorld(16.0f),
  m_heightScale(300.0f),
  m_anisotropicFiltering(false),

  m_simulationStepLength(ezTime::Seconds(1.0f / 60.0f)),
  m_flowDamping(0.98f),
  m_flowAcceleration(10.0f),

  m_terrainRenderShader("terrainRender"),
  m_waterRenderShader("waterRender"),
  m_applyFlowShader("applyFlow"),
  m_updateFlowShader("updateFlow"),
  m_copyShader("copyRefraction"),

  m_terrainData(NULL),
  m_waterOutgoingFlow(NULL),
  m_waterFlowMap(NULL),

  m_textureGrassDiffuseSpec(NULL),
  m_textureStoneDiffuseSpec(NULL),
  m_textureGrassNormalHeight(NULL),
  m_textureStoneNormalHeight(NULL),
  m_waterNormalMap(NULL),
  m_lowResNoise(NULL),
  m_foamTexture(NULL)
{
  EZ_LOG_BLOCK("Terrain");

  m_geomClipMaps = EZ_DEFAULT_NEW(InstancedGeomClipMapping)(m_minPatchSizeWorld, 8, 5);

  // shader init
  m_terrainRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "terrainRender.vert");
  m_terrainRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::CONTROL, "terrainRender.cont");
  m_terrainRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::EVALUATION, "terrainRender.eval");
  m_terrainRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "terrainRender.frag");
  m_terrainRenderShader.CreateProgram();

  m_waterRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "waterRender.vert");
  m_waterRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::CONTROL, "waterRender.cont");
  m_waterRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::EVALUATION, "waterRender.eval");
  m_waterRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "waterRender.frag");
  m_waterRenderShader.CreateProgram();
  
  m_applyFlowShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "flowApply.comp");
  m_applyFlowShader.CreateProgram();
  m_updateFlowShader.AddShaderFromFile(gl::ShaderObject::ShaderType::COMPUTE, "flowUpdate.comp");
  m_updateFlowShader.CreateProgram();
  
  m_copyShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "screenTri.vert");
  m_copyShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "textureOutput.frag");
  m_copyShader.CreateProgram();

  // UBO init
  m_landscapeInfoUBO.Init({ &m_terrainRenderShader, &m_waterRenderShader }, "GlobalLandscapeInfo");
  m_simulationParametersUBO.Init({ &m_applyFlowShader, &m_updateFlowShader }, "SimulationParameters");
  m_waterRenderingUBO.Init({ &m_waterRenderShader }, "WaterRendering");
  m_terrainRenderingUBO.Init({ &m_terrainRenderShader }, "TerrainRendering");


  // set some default values
  m_landscapeInfoUBO["GridMinPosition"].Set(ezVec2(0.0f));
  m_landscapeInfoUBO["MaxTesselationFactor"].Set(m_maxTesselationFactor);
  m_landscapeInfoUBO["HeightmapWorldTexelSize"].Set(1.0f / m_gridWorldSize);
  SetPixelPerTriangle(50.0f);
  m_terrainRenderingUBO["TextureRepeat"].Set(0.1f);

  SetSimulationStepsPerSecond(60.0f);  // Sets implicit all time scaled values

  SetTerrainFresnelReflectionCoef(0.2f);
  SetTerrainSpecularPower(4.0f);

  SetWaterBigDepthColor(ezVec3(0.0039f, 0.00196f, 0.145f)*0.5f);
  SetWaterSurfaceColor(ezVec3(0.0078f, 0.5176f, 0.7f) * 0.5f);
  SetWaterExtinctionCoefficients(ezVec3(0.478f, 0.435f, 0.5f) * 0.1f);
  SetWaterOpaqueness(0.1f);
  SetWaterSpeedToNormalDistortion(0.01f);
  SetWaterDistortionLayerBlendInterval(ezTime::Seconds(2.0f));
  SetWaterFlowDistortionStrength(0.01f);
  SetWaterNormalMapRepeat(30.0f);

  // sampler
  m_texturingSamplerObjectAnisotropic = &gl::SamplerObject::GetSamplerObject(
    gl::SamplerObject::Desc(gl::SamplerObject::Filter::LINEAR, gl::SamplerObject::Filter::LINEAR, gl::SamplerObject::Filter::LINEAR, gl::SamplerObject::Border::REPEAT, 16));
  m_texturingSamplerObjectTrilinear = &gl::SamplerObject::GetSamplerObject(
    gl::SamplerObject::Desc(gl::SamplerObject::Filter::LINEAR, gl::SamplerObject::Filter::LINEAR, gl::SamplerObject::Filter::LINEAR, gl::SamplerObject::Border::REPEAT, 1));
  m_texturingSamplerObjectLinearClamp = &gl::SamplerObject::GetSamplerObject(
    gl::SamplerObject::Desc(gl::SamplerObject::Filter::LINEAR, gl::SamplerObject::Filter::LINEAR, gl::SamplerObject::Filter::NEAREST, gl::SamplerObject::Border::CLAMP, 1));
  m_texturingSamplerObjectNearestRepeat = &gl::SamplerObject::GetSamplerObject(
    gl::SamplerObject::Desc(gl::SamplerObject::Filter::NEAREST, gl::SamplerObject::Filter::NEAREST, gl::SamplerObject::Filter::NEAREST, gl::SamplerObject::Border::REPEAT, 1));


  // Create heightmap
  CreateHeightmapFromNoiseAndResetSim();

  // load textures
  m_textureGrassDiffuseSpec = gl::Texture2D::LoadFromFile("grass.tga", true);
  m_textureStoneDiffuseSpec = gl::Texture2D::LoadFromFile("rock.tga", true);
  m_textureGrassNormalHeight = gl::Texture2D::LoadFromFile("grass_normal.tga");
  m_textureStoneNormalHeight = gl::Texture2D::LoadFromFile("rock_normal.tga");

  m_waterNormalMap = gl::Texture2D::LoadFromFile("water_normal.png", true);
  m_lowResNoise = gl::Texture2D::LoadFromFile("noise.png", true);
  m_foamTexture = gl::Texture2D::LoadFromFile("foam.png", true);
}

Terrain::~Terrain()
{
  EZ_DEFAULT_DELETE(m_terrainData);
  EZ_DEFAULT_DELETE(m_waterOutgoingFlow);
  EZ_DEFAULT_DELETE(m_waterFlowMap);
  EZ_DEFAULT_DELETE(m_geomClipMaps);

  EZ_DEFAULT_DELETE(m_textureGrassDiffuseSpec);
  EZ_DEFAULT_DELETE(m_textureStoneDiffuseSpec);
  EZ_DEFAULT_DELETE(m_textureGrassNormalHeight);
  EZ_DEFAULT_DELETE(m_textureStoneNormalHeight);
  EZ_DEFAULT_DELETE(m_waterNormalMap);
  EZ_DEFAULT_DELETE(m_lowResNoise);
  EZ_DEFAULT_DELETE(m_foamTexture);
}

void Terrain::SetPixelPerTriangle(float pixelPerTriangle)
{
  m_landscapeInfoUBO["TrianglesPerClipSpaceUnit"].Set((static_cast<float>(GeneralConfig::g_ResolutionWidth.GetValue()) / pixelPerTriangle) / 2.0f);
}

void Terrain::SetSimulationStepsPerSecond(float simulationStepsPerSecond)
{
  m_simulationStepLength = ezTime::Seconds(1.0f / simulationStepsPerSecond);

  // Reset all timescaled values.
  SetFlowDamping(m_flowDamping);
  SetFlowAcceleration(m_flowAcceleration);

  float cellDistance = m_gridWorldSize / m_gridResolution;
  m_simulationParametersUBO["CellAreaInv_timeScaled"].Set(static_cast<float>(m_simulationStepLength.GetSeconds() / (cellDistance * cellDistance)));
}

void Terrain::SetFlowDamping(float flowDamping)
{
  m_flowDamping = flowDamping;
  m_simulationParametersUBO["FlowFriction_perStep"].Set(ezMath::Pow(m_flowDamping, static_cast<float>(m_simulationStepLength.GetSeconds())));
}

void Terrain::SetFlowAcceleration(float flowAcceleration)
{
  m_flowAcceleration = flowAcceleration;
  float cellDistance = m_gridWorldSize / m_gridResolution;
  m_simulationParametersUBO["WaterAcceleration_perStep"].Set(static_cast<float>(m_simulationStepLength.GetSeconds() * m_flowAcceleration * cellDistance));
}

void Terrain::CreateHeightmapFromNoiseAndResetSim()
{
  if(m_terrainData != NULL)
    EZ_DEFAULT_DELETE(m_terrainData);

  m_terrainData = EZ_DEFAULT_NEW(gl::Texture2D)(m_gridResolution, m_gridResolution, GL_RGBA32F, -1);
  ezColor* volumeData = EZ_DEFAULT_NEW_RAW_BUFFER(ezColor, m_gridResolution*m_gridResolution);

  NoiseGenerator noiseGen;
  float mulitplier = 1.0f / static_cast<float>(m_gridResolution - 1);

#pragma omp parallel for // OpenMP parallel for loop.
  for(ezInt32 y = 0; y < static_cast<ezInt32>(m_gridResolution); ++y) // Needs to be signed for OpenMP.
  {
    for(ezUInt32 x = 0; x < m_gridResolution; ++x)
    {
      volumeData[x + y * m_gridResolution].r = (noiseGen.GetValueNoise(ezVec3(mulitplier*x, mulitplier*y, 0.0f), 2, 10, 0.43f, true, NULL) * 0.5f + 0.5f) * m_heightScale;
      volumeData[x + y * m_gridResolution].g = 0.3f;
      volumeData[x + y * m_gridResolution].b = 0.3f;
      volumeData[x + y * m_gridResolution].a = std::max(0.0f, (0.45f - pow(ezVec2(x * mulitplier - 0.5f, y * mulitplier - 0.5f).GetLengthSquared(), 2.0f)*800.0f) *m_heightScale
        - volumeData[x + y * m_gridResolution].r);
    }
  }
  m_terrainData->SetData(0, volumeData);

  EZ_DEFAULT_DELETE_RAW_BUFFER(volumeData);

  // Create flow textures
  if(m_waterOutgoingFlow != NULL)
    EZ_DEFAULT_DELETE(m_waterOutgoingFlow);
  m_waterOutgoingFlow = EZ_DEFAULT_NEW(gl::Texture2D)(m_gridResolution, m_gridResolution, GL_RGBA32F, 1);
  ezArrayPtr<ezColor> pEmptyBuffer = EZ_DEFAULT_NEW_ARRAY(ezColor, m_gridResolution*m_gridResolution);
  ezMemoryUtils::ZeroFill(pEmptyBuffer.GetPtr(), pEmptyBuffer.GetCount());
  m_waterOutgoingFlow->SetData(0, pEmptyBuffer.GetPtr());
  EZ_DEFAULT_DELETE_ARRAY(pEmptyBuffer);

  if(m_waterFlowMap == NULL)
    m_waterFlowMap = EZ_DEFAULT_NEW(gl::Texture2D)(m_gridResolution, m_gridResolution, GL_RG16F, 1);
}

void Terrain::PerformSimulationStep(ezTime lastFrameDuration)
{
  m_timeSinceLastSimulationStep += lastFrameDuration;
  ezUInt32 numSimulationSteps = static_cast<ezUInt32>(m_timeSinceLastSimulationStep.GetSeconds() / m_simulationStepLength.GetSeconds());
  m_timeSinceLastSimulationStep -= m_simulationStepLength * numSimulationSteps;

  // clamp simulation step count, otherwise we could get stuck here under certain circumstances.
  numSimulationSteps = ezMath::Min(numSimulationSteps, (ezUInt32)10);

  bool anySimStep = numSimulationSteps > 0;
  if(anySimStep)
    m_simulationParametersUBO.BindBuffer(5);

  for(; numSimulationSteps > 0; --numSimulationSteps)
  {
    m_terrainData->BindImage(0, gl::Texture::ImageAccess::READ, GL_RGBA32F);
    m_waterOutgoingFlow->BindImage(1, gl::Texture::ImageAccess::READ_WRITE, GL_RGBA32F);
    m_updateFlowShader.Activate();
    glDispatchCompute(m_gridResolution / 16, m_gridResolution / 16, 1);

    m_terrainData->BindImage(0, gl::Texture::ImageAccess::READ_WRITE, GL_RGBA32F);
    m_waterOutgoingFlow->BindImage(1, gl::Texture::ImageAccess::READ, GL_RGBA32F);
    m_waterFlowMap->BindImage(2, gl::Texture::ImageAccess::WRITE, GL_RG16F);
    m_applyFlowShader.Activate();
    glDispatchCompute(m_gridResolution / 16, m_gridResolution / 16, 1);
  }

  // Unbind to be assure the gpu that no more reads will happen
  gl::Texture::ResetImageBinding(0);
  gl::Texture::ResetImageBinding(1);
  gl::Texture::ResetImageBinding(2);

  // Update mip and max maps
  if (anySimStep)
  {
    m_terrainData->GenMipMaps();
  }
}

void Terrain::UpdateVisibilty(const ezVec3& cameraPosition)
{
  m_geomClipMaps->UpdateInstanceData(cameraPosition);
}

void Terrain::DrawTerrain()
{
  const gl::SamplerObject* pTextureFilter = m_anisotropicFiltering ? m_texturingSamplerObjectAnisotropic : m_texturingSamplerObjectTrilinear;

  m_terrainData->Bind(0);
  m_textureGrassDiffuseSpec->Bind(1);
  m_textureStoneDiffuseSpec->Bind(2);
  m_textureGrassNormalHeight->Bind(3);
  m_textureStoneNormalHeight->Bind(4);

  m_texturingSamplerObjectTrilinear->BindSampler(0); // TerrainInfo
  pTextureFilter->BindSampler(1); // GrassDiffuse
  pTextureFilter->BindSampler(2); // StoneDiffuse
  pTextureFilter->BindSampler(3); // GrassNormal
  pTextureFilter->BindSampler(4); // StoneNormal

  m_landscapeInfoUBO.BindBuffer(5);
  m_terrainRenderingUBO.BindBuffer(6);

  // Terrain
  m_terrainRenderShader.Activate();
  m_geomClipMaps->DrawGeometry();
}

void Terrain::DrawWater(gl::Texture2D& lowresSceneCopy, gl::Texture2D& depthBufferMaxMaps, gl::TextureCube& reflectionCubemap)
{
  const gl::SamplerObject* pTextureFilter = m_anisotropicFiltering ? m_texturingSamplerObjectAnisotropic : m_texturingSamplerObjectTrilinear;

  // texture setup
  m_texturingSamplerObjectTrilinear->BindSampler(0); // TerrainInfo
  m_texturingSamplerObjectLinearClamp->BindSampler(1); // refraction/reflection
  m_texturingSamplerObjectTrilinear->BindSampler(2); // reflection
  m_texturingSamplerObjectTrilinear->BindSampler(3); // flowmap
  pTextureFilter->BindSampler(4);// normalmap
  m_texturingSamplerObjectTrilinear->BindSampler(5); // noise
  pTextureFilter->BindSampler(6); // Foam
  m_texturingSamplerObjectTrilinear->BindSampler(7); // TerrainInfo again, but filtered
  m_texturingSamplerObjectNearestRepeat->BindSampler(8); // Maxmapped Depth
  
  m_terrainData->Bind(0);
  lowresSceneCopy.Bind(1);
  reflectionCubemap.Bind(2);
  m_waterFlowMap->Bind(3);
  m_waterNormalMap->Bind(4);
  m_lowResNoise->Bind(5);
  m_foamTexture->Bind(6);
  m_terrainData->Bind(7);
  depthBufferMaxMaps.Bind(8);

  // UBO setup
  double flowDistortionTimer = ezTime::Now().GetSeconds() / m_waterDistortionLayerBlendInterval.GetSeconds();
  flowDistortionTimer = ezMath::Mod(flowDistortionTimer, 16000.0);// Keeps accuracy high and avoids flickering. Trading it against ab big flicker every x seconds
  m_waterRenderingUBO["FlowDistortionTimer"].Set(static_cast<float>(flowDistortionTimer));

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);

  m_landscapeInfoUBO.BindBuffer(5);
  m_waterRenderingUBO.BindBuffer(6);

  // Water
  m_waterRenderShader.Activate();
  m_geomClipMaps->DrawGeometry();
}