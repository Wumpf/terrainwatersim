#pragma once

#include "Texture.h"

namespace gl
{
  class Texture3D : public Texture
  {
  public:
    /// \param numMipLevels   -1 for full chain, 0 and 1 have same result
    Texture3D(ezUInt32 width, ezUInt32 height, ezUInt32 depth, GLuint format = GL_RGBA8, ezInt32 numMipLevels = 1);
    
    void SetData(ezUInt32 uiMipLevel, const ezColor* pData);
    void SetData(ezUInt32 uiMipLevel, const ezColor8UNorm* pData);

    GLenum GetOpenGLTextureType() EZ_OVERRIDE { return GL_TEXTURE_3D; }
  };

}

