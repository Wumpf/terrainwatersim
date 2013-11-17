#include "PCH.h"
#include "Terrain.h"

#include "math/NoiseGenerator.h"

#include "gl/ScreenAlignedTriangle.h"
#include "gl/resources/textures/Texture3D.h"
#include "gl/resources/textures/Texture2D.h"
#include "gl/GLUtils.h"

#include "..\config\GlobalCVar.h"

namespace SceneConfig
{
  ezCVarBool g_UseAnisotropicFilter("Anisotropic Filter on/off", true, ezCVarFlags::Save, "group=TerrainRendering");
}

const float Terrain::m_maxTesselationFactor = 32.0f;

Terrain::Terrain() :
  m_worldSize(1024.0f),
  m_minBlockSizeWorld(32.0f)
{
  // shader init
  m_terrainRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "terrainRender.vert");
  m_terrainRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::CONTROL, "terrainRender.cont");
  m_terrainRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::EVALUATION, "terrainRender.eval");
  //m_terrainRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::GEOMETRY, "terrainRender.geom");
  m_terrainRenderShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "terrainRender.frag");
  m_terrainRenderShader.CreateProgram();

  // UBO init
  m_terrainInfoUBO.Init(m_terrainRenderShader, "GlobalTerrainInfo");
  m_terrainInfoUBO["MaxTesselationFactor"].Set(m_maxTesselationFactor);
  //m_terrainInfoUBO["HeightmapHeightScale"].Set(0.0f);
  //m_terrainInfoUBO["HeightmapTexelSize"].Set();
  //m_terrainInfoUBO["HeightmapTexelSizeWorld_doubled"].Set();	// size of a texel in worldcoordinates doubled
  SetPixelPerTriangle(50.0f);
  /*m_terrainInfoUBO["DetailHeightScale"].Set();
  m_terrainInfoUBO["DetailHeightmapTexcoordFactor"].Set();
  m_terrainInfoUBO["DetailHeightmapTexelSize"].Set();	
  m_terrainInfoUBO["DetailHeightmapTexelSizeWorld_doubled"].Set();	// size of a texel in worldcoordinates doubled
  m_terrainInfoUBO["TextureRepeat"].Set(); */

  m_patchInfoUBO.Init(m_terrainRenderShader, "TerrainPatchInfo");

  m_patchInfoUBO["PatchWorldPosition"].Set(ezVec2(0.0f));
  //m_patchInfoUBO["PatchHeightmapTexcoordPosition"]
  m_patchInfoUBO["PatchWorldScale"].Set(40.0f);
  //m_patchInfoUBO["PatchHeightmapTexcoordScale"]



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
  

  // patch vertex buffer
  ezVec2 patchVertices[] = {ezVec2(0.0f, 1.0f), ezVec2(1.0f, 1.0f), ezVec2(1.0f, 0.0f), ezVec2(0.0f, 0.0f) };
  glGenBuffers(1, &m_patchVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_patchVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(patchVertices), patchVertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // vertex array
  glGenVertexArrays(1, &m_patchVertexArrayObject);
  glBindVertexArray(m_patchVertexArrayObject);
  glBindBuffer(GL_ARRAY_BUFFER, m_patchVertexBuffer);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
  
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);

  // load textures
  m_pTextureY.Swap(gl::Texture2D::LoadFromFile("grass.png"));
  m_pTextureXZ.Swap(gl::Texture2D::LoadFromFile("rock.png"));
}

Terrain::~Terrain()
{
  glDeleteVertexArrays(1, &m_patchVertexArrayObject);
  glDeleteBuffers(1, &m_patchVertexBuffer);
  glDeleteSamplers(1, &m_texturingSamplerObjectAnisotropic);
  glDeleteSamplers(1, &m_texturingSamplerObjectTrilinear);
}

void Terrain::SetPixelPerTriangle(float pixelPerTriangle)
{
  m_terrainInfoUBO["TrianglesPerClipSpaceUnit"].Set((static_cast<float>(GeneralConfig::g_ResolutionWidth.GetValue()) / pixelPerTriangle) / 2.0f);
}

void Terrain::DrawRecursive(const ezVec2& min, const ezVec2& max, const ezVec2& cameraPos2D)
{
  ezVec2 center = (min + max) / 2.0f;
  float blockSize = max.x - min.x;
  float distanceToCamSq = (center - cameraPos2D).GetLengthSquared();
  if(blockSize > distanceToCamSq)
    distanceToCamSq = 0.0f;

  if(false)	// culling
    return;


  // Camera is more than two times the of the current block-size away? - far enough away to render NOW
  if(distanceToCamSq / (blockSize * blockSize) > 6.0f ||
    blockSize <= m_minBlockSizeWorld)	// minimum size
  {

    // todo: Push into instancing buffer rather than draw immediately!

    m_patchInfoUBO["PatchWorldPosition"].Set(min); // 2D Position in world
    m_patchInfoUBO["PatchWorldScale"].Set(blockSize); // Scale of the Patch

    m_patchInfoUBO.UpdateGPUData();

    glDrawArrays(GL_PATCHES, 0, 4);
  }
  else // subdivide
  {
    DrawRecursive(min, center, cameraPos2D);
    DrawRecursive(ezVec2(center.x, min.y), ezVec2(max.x, center.y) , cameraPos2D);
    DrawRecursive(center, max, cameraPos2D);
    DrawRecursive(ezVec2(min.x, center.y), ezVec2(center.x, max.y) , cameraPos2D);
  }
}

void Terrain::Draw(const ezVec3& cameraPosition)
{
  m_terrainRenderShader.Activate();

  m_terrainInfoUBO.BindBuffer(5);
  m_patchInfoUBO.BindBuffer(6);

  glBindVertexArray(m_patchVertexArrayObject);
  glPatchParameteri(GL_PATCH_VERTICES, 4);
 
  DrawRecursive(ezVec2(0.0f), ezVec2(m_worldSize), ezVec2(cameraPosition.x, cameraPosition.z));
 
//  glPatchParameteri(GL_PATCH_VERTICES, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}