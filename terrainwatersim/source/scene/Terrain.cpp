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
const float Terrain::m_refractionTextureSizeFactor = 0.5f;

Terrain::Terrain(const ezSizeU32& screenSize) :
  m_worldSize(1024.0f),
  m_gridSize(1024),
  m_minPatchSizeWorld(16.0f),
  m_heightScale(300.0f),
  m_anisotropicFiltering(false),

  m_waterBigDepthColor(ezVec3(0.0039f, 0.00196f, 0.145f)*0.5f),
  m_waterSurfaceColor(ezVec3(0.0078f, 0.5176f, 0.7f) * 0.5f),
  m_waterExtinctionCoefficients(ezVec3(0.478f, 0.435f, 0.5f) * 0.1f),
  m_waterOpaqueness(0.1f),

  m_simulationStepLength(ezTime::Seconds(1.0f / 60.0f)),
  m_flowDamping(0.98f),
  m_flowAcceleration(10.0f)
{
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
  m_landscapeInfoUBO.Init({ &m_terrainRenderShader, &m_applyFlowShader }, "GlobalLandscapeInfo");
  m_simulationParametersUBO.Init({ &m_applyFlowShader, &m_updateFlowShader }, "SimulationParameters");
  m_waterRenderingUBO.Init({ &m_waterRenderShader }, "WaterRendering");


  // set some default values
  m_landscapeInfoUBO["GridMinPosition"].Set(ezVec2(0.0f));
  m_landscapeInfoUBO["MaxTesselationFactor"].Set(m_maxTesselationFactor);
  m_landscapeInfoUBO["HeightmapWorldTexelSize"].Set(1.0f / m_worldSize);
  SetPixelPerTriangle(50.0f);
  m_landscapeInfoUBO["TextureRepeat"].Set(0.05f);
  SetSimulationStepsPerSecond(60.0f);  // Sets implicit all time scaled values
  SetWaterBigDepthColor(m_waterBigDepthColor);
  SetWaterSurfaceColor(m_waterSurfaceColor);
  SetWaterExtinctionCoefficients(m_waterExtinctionCoefficients);
  SetWaterOpaqueness(m_waterOpaqueness);


  // sampler
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
  m_pWaterFlow = EZ_DEFAULT_NEW(gl::Texture2D)(m_gridSize, m_gridSize, GL_RGBA32F, 1);
  ezArrayPtr<ezColor> pEmptyBuffer = EZ_DEFAULT_NEW_ARRAY(ezColor, m_gridSize*m_gridSize);
  ezMemoryUtils::ZeroFill(pEmptyBuffer.GetPtr(), pEmptyBuffer.GetCount());
  m_pWaterFlow->SetData(0, pEmptyBuffer.GetPtr());
  EZ_DEFAULT_DELETE_ARRAY(pEmptyBuffer);

  // load textures
  m_pTextureGrassDiffuseSpec.Swap(gl::Texture2D::LoadFromFile("grass.tga", true));
  m_pTextureStoneDiffuseSpec.Swap(gl::Texture2D::LoadFromFile("rock.tga", true));
  m_pTextureGrassNormalHeight.Swap(gl::Texture2D::LoadFromFile("grass_normal.tga"));
  m_pTextureStoneNormalHeight.Swap(gl::Texture2D::LoadFromFile("rock_normal.tga"));

  RecreateScreenSizeDependentTextures(screenSize);
}

Terrain::~Terrain()
{
  EZ_DEFAULT_DELETE(m_pTerrainData);
  EZ_DEFAULT_DELETE(m_pWaterFlow);
  EZ_DEFAULT_DELETE(m_pGeomClipMaps);

  glDeleteSamplers(1, &m_texturingSamplerObjectAnisotropic);
  glDeleteSamplers(1, &m_texturingSamplerObjectTrilinear);
}

void Terrain::RecreateScreenSizeDependentTextures(const ezSizeU32& screenSize)
{
  m_pRefractionTexture.Swap(EZ_DEFAULT_NEW_UNIQUE(gl::Texture2D, static_cast<ezUInt32>(screenSize.width * m_refractionTextureSizeFactor), 
                                                                 static_cast<ezUInt32>(screenSize.height * m_refractionTextureSizeFactor), GL_RGBA16F, 1, 0));
  m_pRefractionFBO.Swap(EZ_DEFAULT_NEW_UNIQUE(gl::FramebufferObject, { gl::FramebufferObject::Attachment(m_pRefractionTexture.Get()) }));
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

void Terrain::SetWaterBigDepthColor(const ezVec3& waterBigDepthColor)
{
  m_waterBigDepthColor = waterBigDepthColor;
  m_waterRenderingUBO["BigDepthColor"].Set(m_waterBigDepthColor);
}
void Terrain::SetWaterSurfaceColor(const ezVec3& waterSurfaceColor)
{
  m_waterSurfaceColor = waterSurfaceColor;
  m_waterRenderingUBO["SurfaceColor"].Set(m_waterSurfaceColor);
}
void Terrain::SetWaterExtinctionCoefficients(const ezVec3& waterExtinctionCoefficients)
{
  m_waterExtinctionCoefficients = waterExtinctionCoefficients;
  m_waterRenderingUBO["ColorExtinctionCoefficient"].Set(m_waterExtinctionCoefficients);
}
void Terrain::SetWaterOpaqueness(float waterOpaqueness)
{
  m_waterOpaqueness = waterOpaqueness;
  m_waterRenderingUBO["Opaqueness"].Set(m_waterOpaqueness);
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
    m_pWaterFlow->BindImage(1, gl::Texture::ImageAccess::READ_WRITE, GL_RGBA32F);
    m_updateFlowShader.Activate();
    glDispatchCompute(m_gridSize / 32, m_gridSize / 32, 1);

    m_pTerrainData->BindImage(0, gl::Texture::ImageAccess::READ_WRITE, GL_RGBA32F);
    m_pWaterFlow->BindImage(1, gl::Texture::ImageAccess::READ, GL_RGBA32F);
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
  glBindSampler(0, m_texturingSamplerObjectTrilinear);
  m_pTerrainData->Bind(0);

  if(m_anisotropicFiltering)
  {
    glBindSampler(1, m_texturingSamplerObjectAnisotropic);
    glBindSampler(2, m_texturingSamplerObjectAnisotropic);
  }
  else
  {
    glBindSampler(1, m_texturingSamplerObjectTrilinear);
    glBindSampler(2, m_texturingSamplerObjectTrilinear);
  }

  m_pTextureGrassDiffuseSpec->Bind(1);
  m_pTextureStoneDiffuseSpec->Bind(2);
  m_pTextureGrassNormalHeight->Bind(3);
  m_pTextureStoneNormalHeight->Bind(4);


  m_landscapeInfoUBO.BindBuffer(5);

  // Terrain
  m_terrainRenderShader.Activate();
  m_pGeomClipMaps->DrawGeometry();

  glBindSampler(0, 0);
  //  glPatchParameteri(GL_PATCH_VERTICES, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Terrain::DrawWater(gl::FramebufferObject& sceneFBO, gl::TextureCube& reflectionCubemap)
{
  glBindSampler(0, m_texturingSamplerObjectTrilinear);
  glBindSampler(1, m_texturingSamplerObjectTrilinear);

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

  m_landscapeInfoUBO.BindBuffer(5);
  m_waterRenderingUBO.BindBuffer(6);

  // Water
  m_waterRenderShader.Activate();
  m_pGeomClipMaps->DrawGeometry();

  glBindSampler(0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}