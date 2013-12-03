#include "PCH.h"
#include "Terrain.h"

#include "math/NoiseGenerator.h"

#include "InstancedGeomClipMapping.h"

#include "gl/ScreenAlignedTriangle.h"
#include "gl/resources/textures/Texture2D.h"
#include "gl/resources/textures/TextureCube.h"
#include "gl/resources/FramebufferObject.h"
#include "gl/GLUtils.h"

#include "..\config\GlobalCVar.h"
#include <Foundation\Math\Rect.h>

const float Terrain::m_maxTesselationFactor = 64.0f;
const float Terrain::m_refractionTextureSizeFactor = 1.0f;

Terrain::Terrain(const ezSizeU32& screenSize) :
  m_worldSize(1024.0f),
  m_gridSize(1024),
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
  m_copyShader("copyRefractionShader"),

  m_pTextureGrassDiffuseSpec(NULL),
  m_pTextureStoneDiffuseSpec(NULL),
  m_pTextureGrassNormalHeight(NULL),
  m_pTextureStoneNormalHeight(NULL),
  m_pWaterNormalMap(NULL),
  m_pLowResNoise(NULL),
  m_pFoamTexture(NULL),
  
  m_pRefractionFBO(NULL),
  m_pRefractionTexture(NULL)
{
  EZ_LOG_BLOCK("Terrain");

  m_pGeomClipMaps = EZ_DEFAULT_NEW(InstancedGeomClipMapping)(m_worldSize, m_minPatchSizeWorld);

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
  m_landscapeInfoUBO["HeightmapWorldTexelSize"].Set(1.0f / m_worldSize);
  SetPixelPerTriangle(50.0f);
  m_terrainRenderingUBO["TextureRepeat"].Set(0.05f);

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
  glGenSamplers(1, &m_texturingSamplerObjectDataGrids);
  glSamplerParameteri(m_texturingSamplerObjectDataGrids, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glSamplerParameteri(m_texturingSamplerObjectDataGrids, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glSamplerParameteri(m_texturingSamplerObjectDataGrids, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glSamplerParameteri(m_texturingSamplerObjectDataGrids, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glSamplerParameteri(m_texturingSamplerObjectDataGrids, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);

  glGenSamplers(1, &m_texturingSamplerObjectAnisotropic);
  glSamplerParameteri(m_texturingSamplerObjectAnisotropic, GL_TEXTURE_WRAP_R, GL_REPEAT);
  glSamplerParameteri(m_texturingSamplerObjectAnisotropic, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glSamplerParameteri(m_texturingSamplerObjectAnisotropic, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glSamplerParameteri(m_texturingSamplerObjectAnisotropic, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glSamplerParameteri(m_texturingSamplerObjectAnisotropic, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

  glGenSamplers(1, &m_texturingSamplerObjectTrilinear);
  glSamplerParameteri(m_texturingSamplerObjectTrilinear, GL_TEXTURE_WRAP_R, GL_REPEAT);
  glSamplerParameteri(m_texturingSamplerObjectTrilinear, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glSamplerParameteri(m_texturingSamplerObjectTrilinear, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glSamplerParameteri(m_texturingSamplerObjectTrilinear, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glSamplerParameteri(m_texturingSamplerObjectTrilinear, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);


  // Create heightmap
  CreateHeightmap();

  // Create flow textures
  m_pWaterOutgoingFlow = EZ_DEFAULT_NEW(gl::Texture2D)(m_gridSize, m_gridSize, GL_RGBA32F, 1);
  ezArrayPtr<ezColor> pEmptyBuffer = EZ_DEFAULT_NEW_ARRAY(ezColor, m_gridSize*m_gridSize);
  ezMemoryUtils::ZeroFill(pEmptyBuffer.GetPtr(), pEmptyBuffer.GetCount());
  m_pWaterOutgoingFlow->SetData(0, pEmptyBuffer.GetPtr());
  EZ_DEFAULT_DELETE_ARRAY(pEmptyBuffer);
  m_pWaterFlowMap = EZ_DEFAULT_NEW(gl::Texture2D)(m_gridSize, m_gridSize, GL_RG16F, 1);

  // load textures
  m_pTextureGrassDiffuseSpec = gl::Texture2D::LoadFromFile("grass.tga", true);
  m_pTextureStoneDiffuseSpec = gl::Texture2D::LoadFromFile("rock.tga", true);
  m_pTextureGrassNormalHeight = gl::Texture2D::LoadFromFile("grass_normal.tga");
  m_pTextureStoneNormalHeight = gl::Texture2D::LoadFromFile("rock_normal.tga");

  m_pWaterNormalMap = gl::Texture2D::LoadFromFile("water_normal.png", true);
  m_pLowResNoise = gl::Texture2D::LoadFromFile("noise.png", true);
  m_pFoamTexture = gl::Texture2D::LoadFromFile("foam.png", true);

  RecreateScreenSizeDependentTextures(screenSize);
}

Terrain::~Terrain()
{
  EZ_DEFAULT_DELETE(m_pTerrainData);
  EZ_DEFAULT_DELETE(m_pWaterOutgoingFlow);
  EZ_DEFAULT_DELETE(m_pWaterFlowMap);
  EZ_DEFAULT_DELETE(m_pGeomClipMaps);

  EZ_DEFAULT_DELETE(m_pTextureGrassDiffuseSpec);
  EZ_DEFAULT_DELETE(m_pTextureStoneDiffuseSpec);
  EZ_DEFAULT_DELETE(m_pTextureGrassNormalHeight);
  EZ_DEFAULT_DELETE(m_pTextureStoneNormalHeight);
  EZ_DEFAULT_DELETE(m_pWaterNormalMap);
  EZ_DEFAULT_DELETE(m_pLowResNoise);
  EZ_DEFAULT_DELETE(m_pFoamTexture);

  EZ_DEFAULT_DELETE(m_pRefractionFBO);
  EZ_DEFAULT_DELETE(m_pRefractionTexture);

  glDeleteSamplers(1, &m_texturingSamplerObjectDataGrids);
  glDeleteSamplers(1, &m_texturingSamplerObjectAnisotropic);
  glDeleteSamplers(1, &m_texturingSamplerObjectTrilinear);
}

void Terrain::RecreateScreenSizeDependentTextures(const ezSizeU32& screenSize)
{
  EZ_DEFAULT_DELETE(m_pRefractionFBO);
  EZ_DEFAULT_DELETE(m_pRefractionTexture);

  m_pRefractionTexture = EZ_DEFAULT_NEW(gl::Texture2D)(static_cast<ezUInt32>(screenSize.width * m_refractionTextureSizeFactor),
    static_cast<ezUInt32>(screenSize.height * m_refractionTextureSizeFactor), GL_RGBA16F, 1, 0);
  
  m_pRefractionFBO = EZ_DEFAULT_NEW(gl::FramebufferObject)( { gl::FramebufferObject::Attachment(m_pRefractionTexture) });
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

  float cellDistance = m_worldSize / m_gridSize;
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
  float cellDistance = m_worldSize / m_gridSize;
  m_simulationParametersUBO["WaterAcceleration_perStep"].Set(static_cast<float>(m_simulationStepLength.GetSeconds() * m_flowAcceleration * cellDistance));
}


void Terrain::CreateHeightmap()
{
  m_pTerrainData = EZ_DEFAULT_NEW(gl::Texture2D)(m_gridSize, m_gridSize, GL_RGBA32F, -1);
  ezColor* volumeData = EZ_DEFAULT_NEW_RAW_BUFFER(ezColor, m_gridSize*m_gridSize);

  NoiseGenerator noiseGen;
  float mulitplier = 1.0f / static_cast<float>(m_gridSize - 1);

#pragma omp parallel for // OpenMP parallel for loop.
  for(ezInt32 y = 0; y < static_cast<ezInt32>(m_gridSize); ++y) // Needs to be signed for OpenMP.
  {
    for(ezUInt32 x = 0; x < m_gridSize; ++x)
    {
      volumeData[x + y * m_gridSize].r = (noiseGen.GetValueNoise(ezVec3(mulitplier*x, mulitplier*y, 0.0f), 2, 10, 0.43f, false, NULL) * 0.5f + 0.5f) * m_heightScale;
      volumeData[x + y * m_gridSize].g = 0.3f;
      volumeData[x + y * m_gridSize].b = 0.3f;
      volumeData[x + y * m_gridSize].a = std::max(0.0f, (0.45f - pow(ezVec2(x * mulitplier - 0.5f, y * mulitplier - 0.5f).GetLengthSquared(), 2.0f)*800.0f) *m_heightScale
        - volumeData[x + y * m_gridSize].r);
    }
  }
  m_pTerrainData->SetData(0, volumeData);

  EZ_DEFAULT_DELETE_RAW_BUFFER(volumeData);
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
    m_pTerrainData->BindImage(0, gl::Texture::ImageAccess::READ, GL_RGBA32F);
    m_pWaterOutgoingFlow->BindImage(1, gl::Texture::ImageAccess::READ_WRITE, GL_RGBA32F);
    m_updateFlowShader.Activate();
    glDispatchCompute(m_gridSize / 32, m_gridSize / 32, 1);

    m_pTerrainData->BindImage(0, gl::Texture::ImageAccess::READ_WRITE, GL_RGBA32F);
    m_pWaterOutgoingFlow->BindImage(1, gl::Texture::ImageAccess::READ, GL_RGBA32F);
    m_pWaterFlowMap->BindImage(2, gl::Texture::ImageAccess::WRITE, GL_RG16F);
    m_applyFlowShader.Activate();
    glDispatchCompute(m_gridSize / 32, m_gridSize / 32, 1);
  }

  // Todo: Is this very slow?
  if(anySimStep)
    m_pTerrainData->GenMipMaps();
}

void Terrain::UpdateVisibilty(const ezVec3& cameraPosition)
{
  m_pGeomClipMaps->UpdateInstanceData(cameraPosition);
}

void Terrain::DrawTerrain()
{
  glBindSampler(0, m_texturingSamplerObjectDataGrids);
  m_pTerrainData->Bind(0);

  GLuint variableFilter = m_anisotropicFiltering ? m_texturingSamplerObjectAnisotropic : m_texturingSamplerObjectTrilinear;

  glBindSampler(1, variableFilter);
  glBindSampler(2, variableFilter);
  glBindSampler(3, variableFilter);
  glBindSampler(4, variableFilter);


  m_pTextureGrassDiffuseSpec->Bind(1);
  m_pTextureStoneDiffuseSpec->Bind(2);
  m_pTextureGrassNormalHeight->Bind(3);
  m_pTextureStoneNormalHeight->Bind(4);


  m_landscapeInfoUBO.BindBuffer(5);
  m_terrainRenderingUBO.BindBuffer(6);

  // Terrain
  m_terrainRenderShader.Activate();
  m_pGeomClipMaps->DrawGeometry();

  glBindSampler(0, 0);
  //  glPatchParameteri(GL_PATCH_VERTICES, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Terrain::DrawWater(gl::FramebufferObject& sceneFBO, gl::TextureCube& reflectionCubemap)
{
  GLuint variableFilter = m_anisotropicFiltering ? m_texturingSamplerObjectAnisotropic : m_texturingSamplerObjectTrilinear;

  glBindSampler(0, m_texturingSamplerObjectDataGrids); // heightmap
  glBindSampler(1, variableFilter); // refraction
  glBindSampler(2, variableFilter);  // reflection
  glBindSampler(3, m_texturingSamplerObjectDataGrids); // flowmap
  glBindSampler(4, variableFilter); // normalmap
  glBindSampler(5, m_texturingSamplerObjectTrilinear); // noise
  glBindSampler(6, variableFilter); // normalmap


  // Copy framebuffer to low res refraction (because drawing & reading simultaneously doesn't work!) texture
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  m_pRefractionFBO->Bind(true);
  sceneFBO.GetColorAttachments()[0].pTexture->Bind(0);
  m_copyShader.Activate();
  gl::ScreenAlignedTriangle::Draw();

  // reset
  sceneFBO.Bind(true);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);


  // texture setup
  m_pTerrainData->Bind(0);
  m_pRefractionTexture->Bind(1);
  reflectionCubemap.Bind(2);
  m_pWaterFlowMap->Bind(3);
  m_pWaterNormalMap->Bind(4);
  m_pLowResNoise->Bind(5);
  m_pFoamTexture->Bind(6);

  // UBO setup
  m_waterRenderingUBO["FlowDistortionTimer"].Set(static_cast<float>(ezSystemTime::Now().GetSeconds() / m_waterDistortionLayerBlendInterval.GetSeconds()));


  m_landscapeInfoUBO.BindBuffer(5);
  m_waterRenderingUBO.BindBuffer(6);

  // Water
  m_waterRenderShader.Activate();
  m_pGeomClipMaps->DrawGeometry();

  glBindSampler(0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}