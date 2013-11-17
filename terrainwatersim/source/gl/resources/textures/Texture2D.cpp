#include "PCH.h"
#include "Texture2D.h"
#include "../../GLUtils.h"
#include <stb_image/stb_image.h>

#include <Foundation/IO/FileSystem/FileSystem.h>

namespace gl
{
  Texture2D::Texture2D(ezUInt32 uiWidth, ezUInt32 uiHeight, GLuint format, ezInt32 iNumMipLevels) :
    Texture(uiWidth, uiHeight, 1, format, iNumMipLevels)
  {
    Bind(0);
    glTexStorage2D(GL_TEXTURE_2D, m_uiNumMipLevels, format, m_uiWidth, m_uiHeight);
    gl::Utils::CheckError("glTexStorage3D");
  }

  ezUniquePtr<Texture2D> Texture2D::LoadFromFile(const ezString& sFilename, bool generateMipMaps)
  {
    int uiTexSizeX = -1;
    int uiTexSizeY = -1;
    ezString sAbsolutePath;
    if(ezFileSystem::ResolvePath(sFilename.GetData(), false, &sAbsolutePath, NULL) == EZ_FAILURE)
    {
      ezLog::Error("Couldn't find texture \"%s\".", sFilename.GetData());
      return ezUniquePtr<Texture2D>(); // return NULL
    }
    stbi_uc* TextureData = stbi_load(sAbsolutePath.GetData(), &uiTexSizeX, &uiTexSizeY, NULL, 4);
    if(!TextureData)
    {
      ezLog::Error("Error loading texture \"%s\".", sAbsolutePath.GetData());
      return ezUniquePtr<Texture2D>(); // return NULL
    }

    ezUniquePtr<Texture2D> out(EZ_DEFAULT_NEW_UNIQUE(Texture2D, static_cast<ezUInt32>(uiTexSizeX), static_cast<ezUInt32>(uiTexSizeY), GL_RGBA8, generateMipMaps ? -1 : 1));
    out->SetData(0, reinterpret_cast<const ezColor8UNorm*>(TextureData));

    if(generateMipMaps)
      glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(TextureData);

    return out;
  }

  void Texture2D::SetData(ezUInt32 uiMipLevel, const ezColor* pData)
  {
    EZ_ASSERT(uiMipLevel < m_uiNumMipLevels, "MipLevel %i does not exist, texture has only %i MipMapLevels", uiMipLevel, m_uiNumMipLevels);

    Bind(0);
    glTexSubImage2D(GL_TEXTURE_2D, 
                    uiMipLevel,
                    0, 0,
                    m_uiWidth, m_uiHeight,
                    GL_RGBA, GL_FLOAT, pData);
  }

  void Texture2D::SetData(ezUInt32 uiMipLevel, const ezColor8UNorm* pData)
  {
    EZ_ASSERT(uiMipLevel < m_uiNumMipLevels, "MipLevel %i does not exist, texture has only %i MipMapLevels", uiMipLevel, m_uiNumMipLevels);

    Bind(0);
    glTexSubImage2D(GL_TEXTURE_2D, 
      uiMipLevel,
      0, 0,
      m_uiWidth, m_uiHeight,
      GL_RGBA, GL_UNSIGNED_BYTE, pData);
  }

  void Texture2D::Bind(GLuint slotIndex)
  {
    glActiveTexture(GL_TEXTURE0 + slotIndex);
    glBindTexture(GL_TEXTURE_2D, m_TextureHandle);
    gl::Utils::CheckError("glBindTexture");
  }
}