#include "PCH.h"
#include "Terrain.h"

#include "math/NoiseGenerator.h"

#include "gl/ScreenAlignedTriangle.h"
#include "gl/resources/textures/Texture3D.h"
#include "gl/resources/textures/Texture2D.h"
#include "gl/GLUtils.h"

#include "..\config\GlobalCVar.h"
#include <Foundation\Math\Rect.h>

namespace SceneConfig
{
  ezCVarBool g_UseAnisotropicFilter("Anisotropic Filter on/off", true, ezCVarFlags::Save, "group=TerrainRendering");
}

const float Terrain::m_maxTesselationFactor = 32.0f;

Terrain::Terrain() :
  m_worldSize(1024.0f),
  m_minBlockSizeWorld(8.0f)
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
  

  // Patch vertex buffer
  ezVec2 patchVertices[9];
  for(int x=0; x<3; ++x)
  {
    for(int y=0; y<3; ++y)
      patchVertices[x + y * 3] = ezVec2(x * 0.5f, y * 0.5f);
  }
  glGenBuffers(1, &m_patchVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_patchVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(patchVertices), patchVertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Patch index buffer
    // Full patch
  ezUInt8 indicesFull[] = { 0,1,4, 4,1,2, 0,4,3, 4,2,5, 3,4,6,  6,4,7, 7,4,8, 8,4,5 };  // optimize?
  glGenBuffers(1, &m_patchIndexBuffer_full);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchIndexBuffer_full);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesFull), indicesFull, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    // First stitch: Only one triangle at bottom
  ezUInt8 indicesStitch1[] = { 0,1,4, 4,1,2, 0,4,3, 4,2,5, 3,4,6, 6,4,8, 8,4,5 };  // optimize?
  glGenBuffers(1, &m_patchIndexBuffer_stitch1);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchIndexBuffer_stitch1);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesStitch1), indicesStitch1, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    // Second stitch: Only one triangle at bottom and right
  ezUInt8 indicesStitch2[] = { 0,1,4, 4,1,2, 0,4,3, 3,4,6, 6,4,8, 8,4,2 };  // optimize?
  glGenBuffers(1, &m_patchIndexBuffer_stitch2);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchIndexBuffer_stitch2);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesStitch2), indicesStitch2, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  

  // patch vertex array
  glGenVertexArrays(1, &m_patchVertexArray);
  glBindVertexArray(m_patchVertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, m_patchVertexBuffer);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

  // patch instance vertex array

  glEnableVertexAttribArray(0);
  glBindVertexArray(0);

  // load textures
  m_pTextureY.Swap(gl::Texture2D::LoadFromFile("grass.png"));
  m_pTextureXZ.Swap(gl::Texture2D::LoadFromFile("rock.png"));
}

Terrain::~Terrain()
{
  glDeleteVertexArrays(1, &m_patchVertexArray);
  glDeleteBuffers(1, &m_patchVertexBuffer);
  glDeleteBuffers(1, &m_patchIndexBuffer_full);
  glDeleteBuffers(1, &m_patchIndexBuffer_stitch1);
  glDeleteBuffers(1, &m_patchIndexBuffer_stitch2);
  glDeleteSamplers(1, &m_texturingSamplerObjectAnisotropic);
  glDeleteSamplers(1, &m_texturingSamplerObjectTrilinear);
}

void Terrain::SetPixelPerTriangle(float pixelPerTriangle)
{
  m_terrainInfoUBO["TrianglesPerClipSpaceUnit"].Set((static_cast<float>(GeneralConfig::g_ResolutionWidth.GetValue()) / pixelPerTriangle) / 2.0f);
}


void Terrain::Draw(const ezVec3& cameraPosition)
{
  // Render.
  m_terrainRenderShader.Activate();

  m_terrainInfoUBO.BindBuffer(5);
  m_patchInfoUBO.BindBuffer(6);

  
  glBindVertexArray(m_patchVertexArray);
 
  glPatchParameteri(GL_PATCH_VERTICES, 3);
 
  static const ezUInt32 ringThinkness = 8;
  static const ezUInt32 numRings = 6;

  float blockSize = m_minBlockSizeWorld;
  

  ezVec2 minBefore(0.0f);
  ezVec2 maxBefore(0.0f);

  for(int ring=0; ring<numRings; ++ring)
  {
    m_patchInfoUBO["PatchWorldScale"].Set(blockSize);

    // snap to next grid
    ezVec2 cameraBlockPosition = ezVec2(ezMath::Floor(cameraPosition.x / blockSize/2) * blockSize*2, ezMath::Floor(cameraPosition.z / blockSize/2) * blockSize*2);
    ezVec2 positionMin = cameraBlockPosition - ezVec2(blockSize * ringThinkness);
    ezVec2 positionMax = cameraBlockPosition + ezVec2(blockSize * ringThinkness);

    // World is not infinite!
    positionMin.x = ezMath::Clamp(positionMin.x, 0.0f, m_worldSize);
    positionMin.y = ezMath::Clamp(positionMin.y, 0.0f, m_worldSize);
    positionMax.x = ezMath::Clamp(positionMax.x, 0.0f, m_worldSize);
    positionMax.y = ezMath::Clamp(positionMax.y, 0.0f, m_worldSize);

    ezVec2 position;
    for(position.x = positionMin.x; position.x < positionMax.x; position.x += blockSize)
    {
      for(position.y = positionMin.y; position.y < positionMax.y; position.y += blockSize)
      {
        // Skip tile position is within last ring. Since size doubles every time, these are not many.
        if(!(position.x < minBefore.x || position.y < minBefore.y || position.x >= maxBefore.x || position.y >= maxBefore.y))
          continue;

        int xBorder = 0;
        int yBorder = 0;
        if(position.y == positionMin.y)
          yBorder = -1;
        else if(position.y + blockSize >= positionMax.y)
          yBorder =  1;
        if(position.x == positionMin.x)
          xBorder = -1;
        else if(position.x + blockSize >= positionMax.x)
          xBorder =  1;


        if(yBorder == -1)
          m_patchInfoUBO["PatchType"].Set(xBorder == -1 ? 4 : 1);
        else if(xBorder == -1)
          m_patchInfoUBO["PatchType"].Set(3);
        else if(yBorder == 1)
          m_patchInfoUBO["PatchType"].Set(0);
        else if(xBorder == 1)
          m_patchInfoUBO["PatchType"].Set(2);

        if(xBorder == 0 && yBorder == 0)
        {
           glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchIndexBuffer_full);
        }
        else
        {
          if(xBorder == 0 || yBorder == 0)
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchIndexBuffer_stitch1);
          else
          {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchIndexBuffer_stitch2);
          }
        }

        // Draw.
        m_patchInfoUBO["PatchWorldPosition"].Set(position); // 2D Position in world
        m_patchInfoUBO.UpdateGPUData();

        glDrawElements(GL_PATCHES, 8 * 3, GL_UNSIGNED_BYTE, NULL);
      }
    }

    minBefore = positionMin;
    maxBefore = positionMax;
    blockSize *= 2;
  }

 
//  glPatchParameteri(GL_PATCH_VERTICES, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}