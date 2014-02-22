#include "PCH.h"
#include "Texture.h"
#include "../../GLUtils.h"

namespace gl
{
  Texture* Texture::s_pBoundTextures[32];

  Texture::Texture(ezUInt32 width, ezUInt32 height, ezUInt32 depth, GLuint format, ezInt32 numMipLevels, ezUInt32 numMSAASamples) :
    m_width(width),
    m_height(height),
    m_depth(depth),

    m_format(format),
    m_numMipLevels(ConvertMipMapSettingToActualCount(numMipLevels, width, height, depth)),

    m_numMSAASamples(numMSAASamples)
  {
    EZ_ASSERT(m_numMipLevels == 1 || numMSAASamples == 0, "Texture must have either zero MSAA samples or only one miplevel!");
    glGenTextures(1, &m_TextureHandle);
  }

  Texture::~Texture()
  {
    glDeleteTextures(1, &m_TextureHandle);
  }

  ezUInt32 Texture::ConvertMipMapSettingToActualCount(ezInt32 iMipMapSetting, ezUInt32 width, ezUInt32 height, ezUInt32 depth)
  {
    if(iMipMapSetting <= 0)
    {
      ezUInt32 uiNumMipLevels = 0;
      while(width > 0 || height > 0 || depth > 0)
      {
        width /= 2;
        height /= 2;
        depth /= 2;
        ++uiNumMipLevels;
      }
      return uiNumMipLevels;
    }

    else
      return iMipMapSetting;
  }

  void Texture::BindImage(GLuint slotIndex, Texture::ImageAccess access, GLenum format)
  {
    glBindImageTexture(slotIndex, m_TextureHandle, 0, GL_TRUE, 0, static_cast<GLenum>(access), format);
    gl::Utils::CheckError("glBindImageTexture");
  }

  void Texture::Bind(GLuint slotIndex)
  {
    EZ_ASSERT(slotIndex < sizeof(s_pBoundTextures) / sizeof(Texture*), "Can't bind texture to slot %i. Maximum number of slots is %i", slotIndex, sizeof(s_pBoundTextures) / sizeof(Texture*));
    if(s_pBoundTextures[slotIndex] != this)
    {
      glActiveTexture(GL_TEXTURE0 + slotIndex);
      glBindTexture(GetOpenGLTextureType(), m_TextureHandle);
      gl::Utils::CheckError("glBindTexture");
      s_pBoundTextures[slotIndex] = this;
    }
  }
}