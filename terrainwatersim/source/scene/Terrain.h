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
  void SetTerrainWorldSize(float worldSize)     { m_worldSize = worldSize; }

  float GetMinBlockSizeWorld() const             { return m_minBlockSizeWorld; }
  void GetMinBlockSizeWorld(float worldSize)     { m_minBlockSizeWorld = worldSize; }

  float GetPixelPerTriangle() const;
  void SetPixelPerTriangle(float pixelPerTriangle);

private:
  void DrawRecursive(const ezVec2& min, const ezVec2& max, const ezVec2& cameraPos2D);

  // Settings.
  float m_worldSize;
  float m_minBlockSizeWorld;
  float m_pixelPerTriangle;
  static const float m_maxTesselationFactor;
  

  // Graphics resources.
  gl::ShaderObject m_terrainRenderShader;

  gl::UniformBuffer m_terrainInfoUBO;
  gl::UniformBuffer m_patchInfoUBO;

  GLuint m_patchVertexBuffer;
  GLuint m_patchVertexArrayObject;

  GLuint m_texturingSamplerObjectAnisotropic;
  GLuint m_texturingSamplerObjectTrilinear;

  ezUniquePtr<gl::Texture2D> m_pTextureY;
  ezUniquePtr<gl::Texture2D> m_pTextureXZ;
};

