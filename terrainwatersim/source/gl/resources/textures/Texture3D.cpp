#include "PCH.h"
#include "Texture3D.h"
#include "../../GLUtils.h"

namespace gl
{
  Texture3D::Texture3D(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth, GLuint format, ezInt32 iNumMipLevels) :
    Texture( uiWidth, uiHeight, uiDepth, format, iNumMipLevels)
  {
    Bind(0);
    glTexStorage3D(GL_TEXTURE_3D, m_uiNumMipLevels, format, m_uiWidth, m_uiHeight, m_uiDepth);
    gl::Utils::CheckError("glTexStorage3D");
  }

  void Texture3D::SetData(ezUInt32 uiMipLevel, const ezColor* pData)
  {
    EZ_ASSERT(uiMipLevel < m_uiNumMipLevels, "MipLevel %i does not exist, texture has only %i MipMapLevels", uiMipLevel, m_uiNumMipLevels);

    Bind(0);
    glTexSubImage3D(GL_TEXTURE_3D, 
                    uiMipLevel,
                    0, 0, 0,
                    m_uiWidth, m_uiHeight, m_uiDepth,
                    GL_RGBA, GL_FLOAT, pData);
  }
  
  void Texture3D::SetData(ezUInt32 uiMipLevel, const ezColor8UNorm* pData)
  {
    EZ_ASSERT(uiMipLevel < m_uiNumMipLevels, "MipLevel %i does not exist, texture has only %i MipMapLevels", uiMipLevel, m_uiNumMipLevels);

    Bind(0);
    glTexSubImage3D(GL_TEXTURE_3D, 
                    uiMipLevel,
                    0, 0, 0,
                    m_uiWidth, m_uiHeight, m_uiDepth,
                    GL_RGBA, GL_UNSIGNED_BYTE, pData);
  }

  void Texture3D::Bind(GLuint slotIndex)
  {
    glActiveTexture(GL_TEXTURE0 + slotIndex);
    glBindTexture(GL_TEXTURE_3D, m_TextureHandle);
    gl::Utils::CheckError("glBindTexture");
  }
}