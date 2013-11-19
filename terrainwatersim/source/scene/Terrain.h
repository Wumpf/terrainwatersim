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
  
  void Draw(const ezVec3& cameraPosition);
 
  const gl::ShaderObject& GetTerrainShader() { return m_terrainRenderShader; }
  
  float GetTerrainWorldSize() const             { return m_worldSize; }
  //void SetTerrainWorldSize(float worldSize)     { m_worldSize = worldSize; } // patch count changes!

  float GetMinBlockSizeWorld() const             { return m_minBlockSizeWorld; }
  void GetMinBlockSizeWorld(float worldSize)     { m_minBlockSizeWorld = worldSize; }

  float GetPixelPerTriangle() const;
  void SetPixelPerTriangle(float pixelPerTriangle);

private:

  // Settings.
  float m_worldSize;
  float m_minBlockSizeWorld;
  float m_pixelPerTriangle;
  static const float m_maxTesselationFactor;
  ezUInt32 m_maxNumRenderedPatchInstances;
  

  // Graphics resources.
  gl::ShaderObject m_terrainRenderShader;

  gl::UniformBuffer m_terrainInfoUBO;
  gl::UniformBuffer m_patchInfoUBO;

  gl::BufferId m_patchVertexBuffer;
  gl::VertexArrayObjectId m_patchVertexArray;
  gl::IndexBufferId m_patchIndexBuffer_full;
  gl::IndexBufferId m_patchIndexBuffer_stitch1;
  gl::IndexBufferId m_patchIndexBuffer_stitch2;

  gl::SamplerId m_texturingSamplerObjectAnisotropic;
  gl::SamplerId m_texturingSamplerObjectTrilinear;

  ezUniquePtr<gl::Texture2D> m_pTextureY;
  ezUniquePtr<gl::Texture2D> m_pTextureXZ;
};

