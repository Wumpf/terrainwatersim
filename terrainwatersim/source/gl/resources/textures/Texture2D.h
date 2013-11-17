#pragma once

#include "Texture.h"

namespace gl
{
  class Texture2D : public Texture
  {
  public:
    /// \param uiNumMipLevels   -1 for full chain, 0 and 1 have same result
    Texture2D(ezUInt32 uiWidth, ezUInt32 uiHeight, GLuint format = GL_RGBA8, ezInt32 iNumMipLevels = 1);
    
    /// loads texture from file using stb_image
    static ezUniquePtr<Texture2D> LoadFromFile(const ezString& sFilename, bool generateMipMaps = true);

    void SetData(ezUInt32 uiMipLevel, const ezColor* pData) EZ_OVERRIDE;
    void SetData(ezUInt32 uiMipLevel, const ezColor8UNorm* pData) EZ_OVERRIDE;

    void Bind(GLuint slotIndex) EZ_OVERRIDE;
    void BindImage(GLuint slotIndex, Texture::ImageAccess access, GLenum format);
  };

}

