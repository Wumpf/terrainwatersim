#pragma once

#include "Texture.h"

namespace gl
{
  class Texture3D : public Texture
  {
  public:
    /// \param uiNumMipLevels   -1 for full chain, 0 and 1 have same result
    Texture3D(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth, GLuint format = GL_RGBA8, ezInt32 iNumMipLevels = 1);
    
    void SetData(ezUInt32 uiMipLevel, const ezColor* pData) EZ_OVERRIDE;
    void SetData(ezUInt32 uiMipLevel, const ezColor8UNorm* pData) EZ_OVERRIDE;

    void Bind(GLuint slotIndex) EZ_OVERRIDE;
  };

}

