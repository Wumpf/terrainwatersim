#include "PCH.h"
#include "Terrain.h"

#include "math/NoiseGenerator.h"

#include "InstancedGeomClipMapping.h"

#include "gl/ScreenAlignedTriangle.h"
#include "gl/resources/textures/Texture3D.h"
#include "gl/resources/textures/Texture2D.h"
#include "gl/GLUtils.h"

#include "..\config\GlobalCVar.h"
#include <Foundation\Math\Rect.h>

const float Terrain::m_maxTesselationFactor = 64.0f;

Terrain::Terrain() :
  m_worldSize(1024.0f),
  m_heightmapSize(1024),
  m_minPatchSizeWorld(16.0f),
  m_heightScale(300.0f),
  m_anisotropicFiltering(false)
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

  // UBO init
  m_landscapeInfoUBO.Init(m_terrainRenderShader, "GlobalLandscapeInfo");
  m_landscapeInfoUBO["GridMinPosition"].Set(ezVec2(0.0f));
  m_landscapeInfoUBO["MaxTesselationFactor"].Set(m_maxTesselationFactor);
  m_landscapeInfoUBO["HeightmapWorldTexelSize"].Set(1.0f / m_worldSize);
  SetPixelPerTriangle(50.0f);
  m_landscapeInfoUBO["TextureRepeat"].Set(0.05f);

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
  m_pWaterFlow = EZ_DEFAULT_NEW(gl::Texture2D)(m_heightmapSize, m_heightmapSize, GL_RGBA32F, 1);
  ezArrayPtr<ezColor> pEmptyBuffer = EZ_DEFAULT_NEW_ARRAY(ezColor, m_heightmapSize*m_heightmapSize);
  ezMemoryUtils::ZeroFill(pEmptyBuffer.GetPtr(), pEmptyBuffer.GetCount());
  m_pWaterFlow->SetData(0, pEmptyBuffer.GetPtr());
  EZ_DEFAULT_DELETE_ARRAY(pEmptyBuffer);

  // load textures
  m_pTextureGrassDiffuseSpec.Swap(gl::Texture2D::LoadFromFile("grass.tga", true));
  m_pTextureStoneDiffuseSpec.Swap(gl::Texture2D::LoadFromFile("rock.tga", true));
  m_pTextureGrassNormalHeight.Swap(gl::Texture2D::LoadFromFile("grass_normal.tga"));
  m_pTextureStoneNormalHeight.Swap(gl::Texture2D::LoadFromFile("rock_normal.tga"));
}

Terrain::~Terrain()
{
  EZ_DEFAULT_DELETE(m_pTerrainData);
  EZ_DEFAULT_DELETE(m_pWaterFlow);
  EZ_DEFAULT_DELETE(m_pGeomClipMaps);

  glDeleteSamplers(1, &m_texturingSamplerObjectAnisotropic);
  glDeleteSamplers(1, &m_texturingSamplerObjectTrilinear);
}

void Terrain::SetPixelPerTriangle(float pixelPerTriangle)
{
  m_landscapeInfoUBO["TrianglesPerClipSpaceUnit"].Set((static_cast<float>(GeneralConfig::g_ResolutionWidth.GetValue()) / pixelPerTriangle) / 2.0f);
}


void Terrain::CreateHeightmap()
{
  m_pTerrainData = EZ_DEFAULT_NEW(gl::Texture2D)(m_heightmapSize, m_heightmapSize, GL_RGBA32F, -1);
  ezColor* volumeData = EZ_DEFAULT_NEW_RAW_BUFFER(ezColor, m_heightmapSize*m_heightmapSize);

  NoiseGenerator noiseGen;
  float mulitplier = 1.0f / static_cast<float>(m_heightmapSize - 1);

#pragma omp parallel for // OpenMP parallel for loop.
  for(ezInt32 y = 0; y < static_cast<ezInt32>(m_heightmapSize); ++y) // Needs to be signed for OpenMP.
  {
    for(ezUInt32 x = 0; x < m_heightmapSize; ++x)
    {
      volumeData[x + y * m_heightmapSize].r = (noiseGen.GetValueNoise(ezVec3(mulitplier*x, mulitplier*y, 0.0f), 2, 10, 0.43f, false, NULL) * 0.5f + 0.5f) * m_heightScale;
      volumeData[x + y * m_heightmapSize].g = 0.3f;
      volumeData[x + y * m_heightmapSize].b = 0.3f;
      volumeData[x + y * m_heightmapSize].a = std::max(0.0f, (0.45f - pow(ezVec2(x * mulitplier - 0.5f, y * mulitplier - 0.5f).GetLengthSquared(), 2.0f)*800.0f) *m_heightScale
        - volumeData[x + y * m_heightmapSize].r);
    }
  }

  m_pTerrainData->SetData(0, volumeData);

  EZ_DEFAULT_DELETE_RAW_BUFFER(volumeData);
}

void Terrain::PerformSimulationStep()
{
  m_pTerrainData->BindImage(0, gl::Texture::ImageAccess::READ, GL_RGBA32F);
  m_pWaterFlow->BindImage(1, gl::Texture::ImageAccess::READ_WRITE, GL_RGBA32F);
  m_updateFlowShader.Activate();
  glDispatchCompute(m_heightmapSize / 32, m_heightmapSize / 32, 1);

  m_pTerrainData->BindImage(0, gl::Texture::ImageAccess::READ_WRITE, GL_RGBA32F);
  m_pWaterFlow->BindImage(1, gl::Texture::ImageAccess::READ, GL_RGBA32F);
  m_applyFlowShader.Activate();
  glDispatchCompute(m_heightmapSize / 32, m_heightmapSize / 32, 1);


  // Todo: Is this very slow?
  m_pTerrainData->GenMipMaps();
}

void Terrain::Draw(const ezVec3& cameraPosition)
{
  m_pGeomClipMaps->UpdateInstanceData(cameraPosition);

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
  // Water
  m_waterRenderShader.Activate();
  m_pGeomClipMaps->DrawGeometry();


  glBindSampler(0, 0);
  //  glPatchParameteri(GL_PATCH_VERTICES, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}