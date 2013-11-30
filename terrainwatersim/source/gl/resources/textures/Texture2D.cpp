#include "PCH.h"
#include "Texture2D.h"
#include "../../GLUtils.h"
#include <stb_image/stb_image.h>

#include <Foundation/IO/FileSystem/FileSystem.h>

namespace gl
{
  Texture2D::Texture2D(ezUInt32 width, ezUInt32 height, GLuint format, ezInt32 numMipLevels, ezUInt32 numMSAASamples) :
    Texture(width, height, 1, format, numMipLevels, numMSAASamples)
  {
    Bind(0);
    if(m_numMSAASamples == 0)
      glTexStorage2D(GL_TEXTURE_2D, m_numMipLevels, format, m_width, m_height);
    else
      glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_numMSAASamples, format, m_width, m_height, GL_FALSE);
    gl::Utils::CheckError("glTexStorage2D");
  }

  ezUniquePtr<Texture2D> Texture2D::LoadFromFile(const ezString& sFilename, bool sRGB, bool generateMipMaps)
  {
    int uiTexSizeX = -1;
    int uiTexSizeY = -1;
    ezString sAbsolutePath;
    if(ezFileSystem::ResolvePath(sFilename.GetData(), false, &sAbsolutePath, NULL) == EZ_FAILURE)
    {
      ezLog::Error("Couldn't find texture \"%s\".", sFilename.GetData());
      return ezUniquePtr<Texture2D>(); // return NULL
    }
    int numComps = -1;
    stbi_uc* TextureData = stbi_load(sAbsolutePath.GetData(), &uiTexSizeX, &uiTexSizeY, &numComps, 4);
    if(!TextureData)
    {
      ezLog::Error("Error loading texture \"%s\".", sAbsolutePath.GetData());
      return ezUniquePtr<Texture2D>(); // return NULL
    }

    ezUniquePtr<Texture2D> out(EZ_DEFAULT_NEW_UNIQUE(Texture2D, static_cast<ezUInt32>(uiTexSizeX), static_cast<ezUInt32>(uiTexSizeY), sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8, generateMipMaps ? -1 : 1));
    out->SetData(0, reinterpret_cast<const ezColor8UNorm*>(TextureData));

    if(generateMipMaps && out->GetNumMipLevels() > 1)
      glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(TextureData);

    return out;
  }

  void Texture2D::SetData(ezUInt32 uiMipLevel, const ezColor* pData)
  {
    EZ_ASSERT(uiMipLevel < m_numMipLevels, "MipLevel %i does not exist, texture has only %i MipMapLevels", uiMipLevel, m_numMipLevels);

    Bind(0);
    glTexSubImage2D(GetNumMSAASamples() > 0 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
                    uiMipLevel,
                    0, 0,
                    m_width, m_height,
                    GL_RGBA, GL_FLOAT, pData);
  }

  void Texture2D::GenMipMaps()
  {
    Bind(0);
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  void Texture2D::SetData(ezUInt32 uiMipLevel, const ezColor8UNorm* pData)
  {
    EZ_ASSERT(uiMipLevel < m_numMipLevels, "MipLevel %i does not exist, texture has only %i MipMapLevels", uiMipLevel, m_numMipLevels);

    Bind(0);
    glTexSubImage2D(GetNumMSAASamples() > 0 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
      uiMipLevel,
      0, 0,
      m_width, m_height,
      GL_RGBA, GL_UNSIGNED_BYTE, pData);
  }

  void Texture2D::Bind(GLuint slotIndex)
  {
    EZ_ASSERT(slotIndex < sizeof(s_pBoundTextures) / sizeof(Texture*), "Can't bind texture to slot %i. Maximum number of slots is %i", slotIndex, sizeof(s_pBoundTextures) / sizeof(Texture*));
    if(s_pBoundTextures[slotIndex] != this)
    {
      glActiveTexture(GL_TEXTURE0 + slotIndex);
      glBindTexture(GetNumMSAASamples() > 0 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, m_TextureHandle);
      gl::Utils::CheckError("glBindTexture");
      s_pBoundTextures[slotIndex] = this;
    }
  }
}