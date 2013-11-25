#include "PCH.h"
#include "InstancedGeomClipMapping.h"


InstancedGeomClipMapping::InstancedGeomClipMapping(float maxGridSize, float minPatchSizeWorld) :
m_maxGridSize(maxGridSize),
m_minPatchSizeWorld(minPatchSizeWorld)
{
  // Patch vertex buffer
  ezVec2 patchVertices[9];
  for(int x = 0; x < 3; ++x)
  {
    for(int y = 0; y < 3; ++y)
      patchVertices[x + y * 3] = ezVec2(x * 0.5f, y * 0.5f);
  }
  glGenBuffers(1, &m_patchVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_patchVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(patchVertices), patchVertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Instance buffers
  // Try to guess max number of patches:
  m_maxPatchInstances[(ezUInt32)PatchType::FULL] = static_cast<ezUInt32>(ezMath::Pow(m_maxGridSize / m_minPatchSizeWorld / 2, 2.0f));
  m_maxPatchInstances[(ezUInt32)PatchType::STITCH1] = static_cast<ezUInt32>(m_maxPatchInstances[(ezUInt32)PatchType::FULL] * (static_cast<float>(4 * 2 * m_ringThinkness) / (2 * m_ringThinkness * 2 * m_ringThinkness)));
  m_maxPatchInstances[(ezUInt32)PatchType::STITCH2] = m_maxPatchInstances[(ezUInt32)PatchType::STITCH1] / 4;

  // Full patch
  glGenBuffers((ezUInt32)PatchType::NUM_TYPES, m_patchInstanceBuffer);
  for(int i = 0; i < (ezUInt32)PatchType::NUM_TYPES; ++i)
  {
    glBindBuffer(GL_ARRAY_BUFFER, m_patchInstanceBuffer[i]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(PatchInstanceData)* m_maxPatchInstances[i], NULL, GL_DYNAMIC_DRAW);
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);


  // Patch index buffer
  glGenBuffers((ezUInt32)PatchType::NUM_TYPES, m_patchIndexBuffer);
  // Full patch
  ezUInt8 indicesFull[] = { 0, 1, 4, 4, 1, 2, 0, 4, 3, 4, 2, 5, 3, 4, 6, 6, 4, 7, 7, 4, 8, 8, 4, 5 };  // optimize?
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchIndexBuffer[(ezUInt32)PatchType::FULL]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesFull), indicesFull, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  // First stitch: Only one triangle at bottom
  ezUInt8 indicesStitch1[] = { 0, 1, 4, 4, 1, 2, 0, 4, 3, 4, 2, 5, 3, 4, 6, 6, 4, 8, 8, 4, 5 };  // optimize?
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchIndexBuffer[(ezUInt32)PatchType::STITCH1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesStitch1), indicesStitch1, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  // Second stitch: Only one triangle at bottom and right
  ezUInt8 indicesStitch2[] = { 0, 1, 4, 4, 1, 2, 0, 4, 3, 3, 4, 6, 6, 4, 8, 8, 4, 2 };  // optimize?
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchIndexBuffer[(ezUInt32)PatchType::STITCH2]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesStitch2), indicesStitch2, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


  // patch vertex array
  glGenVertexArrays((ezUInt32)PatchType::NUM_TYPES, m_patchVertexArray);
  for(int i = 0; i < (ezUInt32)PatchType::NUM_TYPES; ++i)
  {
    glBindVertexArray(m_patchVertexArray[i]);

    glBindBuffer(GL_ARRAY_BUFFER, m_patchVertexBuffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)* 2, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_patchInstanceBuffer[i]);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(PatchInstanceData), 0);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(PatchInstanceData), reinterpret_cast<void*>(sizeof(float)* 2));
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(PatchInstanceData), reinterpret_cast<void*>(sizeof(float)* 3));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);


    glBindVertexArray(0);
  }
}


InstancedGeomClipMapping::~InstancedGeomClipMapping()
{
  glDeleteVertexArrays((ezUInt32)PatchType::NUM_TYPES, m_patchVertexArray);
  glDeleteBuffers(1, &m_patchVertexBuffer);
  glDeleteBuffers((ezUInt32)PatchType::NUM_TYPES, m_patchIndexBuffer);
  glDeleteBuffers((ezUInt32)PatchType::NUM_TYPES, m_patchInstanceBuffer);
}

void InstancedGeomClipMapping::UpdateInstanceData(const ezVec3& cameraPosition)
{
  for(int i = 0; i < (ezUInt32)PatchType::NUM_TYPES; ++i)
    m_currentInstanceData[i].Clear();

  ezVec2 minBefore(0.0f);
  ezVec2 maxBefore(0.0f);

  PatchInstanceData currentPatch;
  currentPatch.worldScale = m_minPatchSizeWorld;

  for(int ring = 0; ring < m_numRings; ++ring)
  {
    // snap to next grid
    ezVec2 cameraBlockPosition = ezVec2(ezMath::Floor(cameraPosition.x / currentPatch.worldScale / 2) * currentPatch.worldScale * 2,
      ezMath::Floor(cameraPosition.z / currentPatch.worldScale / 2) * currentPatch.worldScale * 2);
    ezVec2 positionMin = cameraBlockPosition - ezVec2(currentPatch.worldScale * m_ringThinkness);
    ezVec2 positionMax = cameraBlockPosition + ezVec2(currentPatch.worldScale * m_ringThinkness);

    // World is not infinite!
    positionMin.x = ezMath::Clamp(positionMin.x, 0.0f, m_maxGridSize);
    positionMin.y = ezMath::Clamp(positionMin.y, 0.0f, m_maxGridSize);
    positionMax.x = ezMath::Clamp(positionMax.x, 0.0f, m_maxGridSize);
    positionMax.y = ezMath::Clamp(positionMax.y, 0.0f, m_maxGridSize);

    for(currentPatch.worldPosition.x = positionMin.x; currentPatch.worldPosition.x < positionMax.x; currentPatch.worldPosition.x += currentPatch.worldScale)
    {
      for(currentPatch.worldPosition.y = positionMin.y; currentPatch.worldPosition.y < positionMax.y; currentPatch.worldPosition.y += currentPatch.worldScale)
      {
        // Skip tile position if it is within last ring. Since size doubles every time, these are not many.
        if(!(currentPatch.worldPosition.x < minBefore.x || currentPatch.worldPosition.y < minBefore.y ||
          currentPatch.worldPosition.x >= maxBefore.x || currentPatch.worldPosition.y >= maxBefore.y))
          continue;

        int xBorder = 0;
        int yBorder = 0;
        if(currentPatch.worldPosition.y == positionMin.y)
          yBorder = -1;
        else if(currentPatch.worldPosition.y + currentPatch.worldScale >= positionMax.y)
          yBorder = 1;
        if(currentPatch.worldPosition.x == positionMin.x)
          xBorder = -1;
        else if(currentPatch.worldPosition.x + currentPatch.worldScale >= positionMax.x)
          xBorder = 1;

        if(yBorder == -1)
          currentPatch.rotationType = xBorder == 1 ? 4 : 1;
        else if(xBorder == -1)
          currentPatch.rotationType = 3;
        else if(yBorder == 1)
          currentPatch.rotationType = 0;
        else// if(xBorder == 1)
          currentPatch.rotationType = 2;

        if(xBorder == 0 && yBorder == 0)
          m_currentInstanceData[(ezUInt32)PatchType::FULL].PushBack(currentPatch);
        else
        {
          if(xBorder == 0 || yBorder == 0)
            m_currentInstanceData[(ezUInt32)PatchType::STITCH1].PushBack(currentPatch);
          else
            m_currentInstanceData[(ezUInt32)PatchType::STITCH2].PushBack(currentPatch);
        }
      }
    }

    minBefore = positionMin;
    maxBefore = positionMax;
    currentPatch.worldScale *= 2;
  }

  // Upload to gpu.
  for(int i = 0; i < (ezUInt32)PatchType::NUM_TYPES; ++i)
  {
    EZ_ASSERT(m_currentInstanceData[i].GetCount() <= m_maxPatchInstances[i], "Too many patch instances!");
    glBindBuffer(GL_ARRAY_BUFFER, m_patchInstanceBuffer[i]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(PatchInstanceData)* m_currentInstanceData[i].GetCount(),
      static_cast<ezArrayPtr<PatchInstanceData>>(m_currentInstanceData[i]).GetPtr());
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void InstancedGeomClipMapping::DrawGeometry()
{
  glPatchParameteri(GL_PATCH_VERTICES, 3);

  for(int i = 0; i < (ezUInt32)PatchType::NUM_TYPES; ++i)
  {
    glBindVertexArray(m_patchVertexArray[i]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchIndexBuffer[i]);
    glDrawElementsInstanced(GL_PATCHES, 8 * 3, GL_UNSIGNED_BYTE, NULL, m_currentInstanceData[i].GetCount());
  }
}