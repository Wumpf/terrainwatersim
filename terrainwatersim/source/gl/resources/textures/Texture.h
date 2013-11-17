#pragma once

namespace gl
{
  class Texture
  {
  public:
    Texture(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth, GLuint format, ezInt32 iNumMipLevels);
    ~Texture();

    virtual void SetData(ezUInt32 uiMipLevel, const ezColor* pData)=0;
    virtual void SetData(ezUInt32 uiMipLevel, const ezColor8UNorm* pData)=0;

    virtual void Bind(GLuint slotIndex)=0;

    enum class ImageAccess
    {
      WRITE = GL_WRITE_ONLY,
      READ = GL_READ_ONLY,
      READ_WRITE = GL_READ_WRITE
    };
      
    void BindImage(GLuint slotIndex, ImageAccess access) { BindImage(0, access, m_Format); }
    void BindImage(GLuint slotIndex, ImageAccess access, GLenum format);

    GLuint GetInternHandle() { return m_TextureHandle; }

  protected:
    GLuint m_TextureHandle;

    const ezUInt32 m_uiWidth;
    const ezUInt32 m_uiHeight;
    const ezUInt32 m_uiDepth;

    const GLuint    m_Format;
    const ezUInt32  m_uiNumMipLevels;

  private:
    static ezUInt32 ConvertMipMapSettingToActualCount(ezInt32 iMipMapSetting, ezUInt32 width, ezUInt32 height, ezUInt32 depth = 0);
  };

}
