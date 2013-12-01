#pragma once

namespace gl
{
  class Texture
  {
  public:
    Texture(ezUInt32 width, ezUInt32 height, ezUInt32 depth, GLuint format, ezInt32 numMipLevels, ezUInt32 numMSAASamples = 0);
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
      
    /// Binds as image, currently without redundancy checking!
    void BindImage(GLuint slotIndex, ImageAccess access) { BindImage(0, access, m_format); }
    void BindImage(GLuint slotIndex, ImageAccess access, GLenum format);

    TextureId GetInternHandle() { return m_TextureHandle; }

    ezUInt32 GetWidth() const           { return m_width; }
    ezUInt32 GetHeight() const          { return m_height; }
    ezUInt32 GetDepth() const           { return m_depth; }
    ezUInt32 GetNumMipLevels() const    { return m_numMipLevels; }
    ezUInt32 GetNumMSAASamples() const  { return m_numMSAASamples; }
    ezUInt32 GetFormat() const          { return m_format; }

  protected:
    /// Currently bound textures - number is arbitrary!
    /// Not used for image binding
    static Texture* s_pBoundTextures[32];

    TextureId m_TextureHandle;

    const ezUInt32 m_width;
    const ezUInt32 m_height;
    const ezUInt32 m_depth;

    const GLuint    m_format;
    const ezUInt32  m_numMipLevels;
    const ezUInt32  m_numMSAASamples;

  private:
    static ezUInt32 ConvertMipMapSettingToActualCount(ezInt32 iMipMapSetting, ezUInt32 width, ezUInt32 height, ezUInt32 depth = 0);
  };

}
