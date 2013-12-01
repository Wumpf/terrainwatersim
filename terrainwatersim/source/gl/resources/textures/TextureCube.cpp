#include "PCH.h"
#include "TextureCube.h"
#include "../../GLUtils.h"

#include <Foundation/IO/FileSystem/FileSystem.h>

namespace gl
{
  TextureCube::TextureCube(ezUInt32 size, GLuint format, ezInt32 numMipLevels) :
    Texture(size, size, 1, format, numMipLevels)
  {
    Bind(0);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, m_numMipLevels, format, size, size);
    gl::Utils::CheckError("glTexStorage2D");
  }

  void TextureCube::SetData(CubemapFace face, ezUInt32 uiMipLevel, const ezColor* pData)
  {
    EZ_ASSERT(uiMipLevel < m_numMipLevels, "MipLevel %i does not exist, texture has only %i MipMapLevels", uiMipLevel, m_numMipLevels);

    Bind(0);
    glTexSubImage2D(static_cast<GLenum>(face),
                    uiMipLevel,
                    0, 0,
                    m_width, m_height,
                    GL_RGBA, GL_FLOAT, pData);
  }

  void TextureCube::GenMipMaps()
  {
    Bind(0);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
  }

  void TextureCube::SetData(CubemapFace face, ezUInt32 uiMipLevel, const ezColor8UNorm* pData)
  {
    EZ_ASSERT(uiMipLevel < m_numMipLevels, "MipLevel %i does not exist, texture has only %i MipMapLevels", uiMipLevel, m_numMipLevels);

    Bind(0);
    glTexSubImage2D(static_cast<GLenum>(face),
      uiMipLevel,
      0, 0,
      m_width, m_height,
      GL_RGBA, GL_UNSIGNED_BYTE, pData);
  }
}