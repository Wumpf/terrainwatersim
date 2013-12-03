#include "PCH.h"
#include "Texture3D.h"
#include "../../GLUtils.h"

namespace gl
{
  Texture3D::Texture3D(ezUInt32 width, ezUInt32 height, ezUInt32 depth, GLuint format, ezInt32 iNumMipLevels) :
    Texture( width, height, depth, format, iNumMipLevels)
  {
    Bind(0);
    glTexStorage3D(GL_TEXTURE_3D, m_numMipLevels, format, m_width, m_height, m_depth);
    gl::Utils::CheckError("glTexStorage3D");
  }

  void Texture3D::SetData(ezUInt32 mipLevel, const ezColor* pData)
  {
    EZ_ASSERT(mipLevel < m_numMipLevels, "MipLevel %i does not exist, texture has only %i MipMapLevels", mipLevel, m_numMipLevels);

    Bind(0);
    glTexSubImage3D(GL_TEXTURE_3D, 
                    mipLevel,
                    0, 0, 0,
                    m_width, m_height, m_depth,
                    GL_RGBA, GL_FLOAT, pData);
  }
  
  void Texture3D::SetData(ezUInt32 mipLevel, const ezColor8UNorm* pData)
  {
    EZ_ASSERT(mipLevel < m_numMipLevels, "MipLevel %i does not exist, texture has only %i MipMapLevels", mipLevel, m_numMipLevels);

    Bind(0);
    glTexSubImage3D(GL_TEXTURE_3D, 
                    mipLevel,
                    0, 0, 0,
                    m_width, m_height, m_depth,
                    GL_RGBA, GL_UNSIGNED_BYTE, pData);
  }
}