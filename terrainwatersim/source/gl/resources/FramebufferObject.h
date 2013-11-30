#pragma once

#include "../ShaderDataMetaInfo.h"

namespace gl
{
  class Texture;

  class FramebufferObject
  {
  public:
    struct Attachment
    {
      Attachment(Texture* pTexture, ezUInt32 mipLevel = 0, ezUInt32 layer = 0) : 
        pTexture(pTexture), mipLevel(mipLevel), layer(layer) {}

      Texture* pTexture;
      ezUInt32 mipLevel; 
      ezUInt32 layer;
    };

    FramebufferObject(std::initializer_list<Attachment> colorAttachments, Attachment depthStencil = Attachment(NULL), bool depthWithStencil = false);
    ~FramebufferObject();

    /// Binds the framebuffer object (GL_DRAW_FRAMEBUFFER)
    void Bind();
    /// Resets the binding to zero (GL_DRAW_FRAMEBUFFER)
    static void BindBackBuffer();

    Framebuffer GetInternHandle() { return m_framebuffer; }

  private:
    /// Currently bound draw framebuffer object (NULL means backbuffer)
    static FramebufferObject* s_BoundFrameBufferDraw;
    /// Currently bound read framebuffer object (NULL means backbuffer)
   // static FramebufferObject* s_BoundFrameBufferRead;

    Framebuffer m_framebuffer;

    Attachment m_depthStencil;
    ezDynamicArray<Attachment> m_colorAttachments;
  };
}

