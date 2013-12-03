#pragma once

#include "Texture.h"

namespace gl
{
  class Texture2D : public Texture
  {
  public:
    /// \param uiNumMipLevels   -1 for full chain, 0 and 1 have same result
    Texture2D(ezUInt32 width, ezUInt32 height, GLuint format = GL_RGBA8, ezInt32 numMipLevels = 1, ezUInt32 numMSAASamples = 0);
    
    /// \brief Loads texture from file using stb_image
    ///
    /// Uses EZ_DEFAULT_NEW to create an object if file loading is successful. Use EZ_DEFAULT_DELETE to remove the texture.
    /// Will return NULL on error.
    static Texture2D* LoadFromFile(const ezString& sFilename, bool sRGB = false, bool generateMipMaps = true);

    void SetData(ezUInt32 mipLevel, const ezColor* pData);
    void SetData(ezUInt32 mipLevel, const ezColor8UNorm* pData);

    void GenMipMaps();

    GLenum GetOpenGLTextureType() EZ_OVERRIDE { return GetNumMSAASamples() > 0 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D; }
  };

}

