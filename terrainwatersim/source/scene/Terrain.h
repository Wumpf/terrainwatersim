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
  //void SetTerrainWorldSize(float worldSize)     { m_worldSize = worldSize; }    // Expected patch count changes!

  float GetMinBlockSizeWorld() const             { return m_minPatchSizeWorld; }
  //void SetMinBlockSizeWorld(float worldSize)     { m_minBlockSizeWorld = worldSize; } // Expected patch count changes!

  float GetPixelPerTriangle() const;
  void SetPixelPerTriangle(float pixelPerTriangle);

private:
  void UpdateInstanceData(const ezVec3& cameraPosition);

  // Settings.
  float m_worldSize;
  float m_minPatchSizeWorld;
  float m_pixelPerTriangle;
  static const float m_maxTesselationFactor;
  ezUInt32 m_maxNumRenderedPatchInstances;

  static const ezUInt32 m_ringThinkness = 8;
  static const ezUInt32 m_numRings = 6;

  struct PatchInstanceData
  {
    ezVec2 worldPosition;
    float worldScale;
    ezUInt32 rotationType;
  };

  // Graphics resources.
  gl::ShaderObject m_terrainRenderShader;
  gl::UniformBuffer m_terrainInfoUBO;

  // Contains immutable relative patch positions.
  gl::BufferId m_patchVertexBuffer;
  
  enum class PatchType : ezUInt32
  {
    FULL,
    STITCH1,
    STITCH2,

    NUM_TYPES
  };

  gl::IndexBufferId m_patchIndexBuffer[PatchType::NUM_TYPES];
  gl::BufferId m_patchInstanceBuffer[PatchType::NUM_TYPES];
  ezUInt32 m_maxPatchInstances[PatchType::NUM_TYPES];
  gl::VertexArrayObjectId m_patchVertexArray[PatchType::NUM_TYPES];

  // Serves as CPU buffer for instance data. The element counter will be used to determine how many instances are active at the moment.
  ezDynamicArray<PatchInstanceData> m_currentInstanceData[PatchType::NUM_TYPES];

  gl::SamplerId m_texturingSamplerObjectAnisotropic;
  gl::SamplerId m_texturingSamplerObjectTrilinear;

  ezUniquePtr<gl::Texture2D> m_pTextureY;
  ezUniquePtr<gl::Texture2D> m_pTextureXZ;
};

