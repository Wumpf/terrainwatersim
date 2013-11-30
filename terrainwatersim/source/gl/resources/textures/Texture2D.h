#pragma once

#include "Texture.h"

namespace gl
{
  class Texture2D : public Texture
  {
  public:
    /// \param uiNumMipLevels   -1 for full chain, 0 and 1 have same result
    Texture2D(ezUInt32 width, ezUInt32 height, GLuint format = GL_RGBA8, ezInt32 numMipLevels = 1, ezUInt32 numMSAASamples = 0);
    
    /// loads texture from file using stb_image
    static ezUniquePtr<Texture2D> LoadFromFile(const ezString& sFilename, bool sRGB = false, bool generateMipMaps = true);

    void SetData(ezUInt32 mipLevel, const ezColor* pData) EZ_OVERRIDE;
    void SetData(ezUInt32 mipLevel, const ezColor8UNorm* pData) EZ_OVERRIDE;

    void GenMipMaps();

    void Bind(GLuint slotIndex) EZ_OVERRIDE;
  };

}

