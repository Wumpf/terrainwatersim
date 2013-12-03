#pragma once

#include "Texture.h"

namespace gl
{
  class TextureCube : public Texture
  {
  public:
    /// \param uiNumMipLevels   -1 for full chain, 0 and 1 have same result
    TextureCube(ezUInt32 size, GLuint format = GL_RGBA8, ezInt32 numMipLevels = 1);

    enum class CubemapFace : GLenum
    {
      X_POS = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
      X_NEG = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
      Y_POS = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
      Y_NEG = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
      Z_POS = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
      Z_NEG = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    void SetData(CubemapFace face, ezUInt32 mipLevel, const ezColor* pData);
    void SetData(CubemapFace face, ezUInt32 mipLevel, const ezColor8UNorm* pData);

    void GenMipMaps();

    GLenum GetOpenGLTextureType() EZ_OVERRIDE { return GL_TEXTURE_CUBE_MAP; }
  };

}

