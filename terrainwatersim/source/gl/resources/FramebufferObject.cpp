#include "PCH.h"
#include "../GLUtils.h"
#include "FramebufferObject.h"
#include "textures/Texture.h"
#include "textures/Texture2D.h"

namespace gl
{
  FramebufferObject* FramebufferObject::s_BoundFrameBufferDraw = NULL;
  FramebufferObject* FramebufferObject::s_BoundFrameBufferRead = NULL;

  FramebufferObject::FramebufferObject(std::initializer_list<Attachment> colorAttachments, Attachment depthStencil, bool depthWithStencil) :
    m_depthStencil(depthStencil)
  {
    glGenFramebuffers(1, &m_framebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    s_BoundFrameBufferDraw = this;
    s_BoundFrameBufferRead = this;

    if(depthStencil.pTexture)
    {
      GLint attachment = depthWithStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;

      // Sufficient for MSAA?
      if(depthStencil.layer > 0)
        glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, depthStencil.pTexture->GetInternHandle(), depthStencil.mipLevel, depthStencil.layer);
      else
        glFramebufferTexture(GL_FRAMEBUFFER, attachment, depthStencil.pTexture->GetInternHandle(), depthStencil.mipLevel);
    }


    for(auto it = colorAttachments.begin(); it != colorAttachments.end(); ++it)
    {
      GLint attachment = GL_COLOR_ATTACHMENT0 + m_colorAttachments.GetCount();
      if(it->layer > 0)
        glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, it->pTexture->GetInternHandle(), it->mipLevel, it->layer);
      else
        glFramebufferTexture(GL_FRAMEBUFFER, attachment, it->pTexture->GetInternHandle(), it->mipLevel);

      m_colorAttachments.PushBack(*it);
    }

    // setup draw buffers
    ezArrayPtr<GLuint> drawBuffers = EZ_DEFAULT_NEW_ARRAY(GLuint, m_colorAttachments.GetCount());
    for(ezUInt32 i = 0; i < m_colorAttachments.GetCount(); ++i)
      drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
    glDrawBuffers(m_colorAttachments.GetCount(), drawBuffers.GetPtr());
    EZ_DEFAULT_DELETE_ARRAY(drawBuffers);

    // Set read buffers to first color target.
    glReadBuffer(GL_COLOR_ATTACHMENT0);


    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    EZ_ASSERT(framebufferStatus == GL_FRAMEBUFFER_COMPLETE, "Frame buffer creation failed! Error code: %i", framebufferStatus);
  }

  FramebufferObject::~FramebufferObject()
  {
    glDeleteFramebuffers(1, &m_framebuffer);
  }

  void FramebufferObject::Bind()
  {
    if(s_BoundFrameBufferDraw != this)
    {
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer);
      s_BoundFrameBufferDraw = this;
    }
  }

  void FramebufferObject::BindBackBuffer()
  {
    if(s_BoundFrameBufferDraw != NULL)
    {
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      s_BoundFrameBufferDraw = NULL;
    }
  }
 

  void FramebufferObject::BlitTo(FramebufferObject* pDest, const ezRectU32& srcRect, const ezRectU32& dstRect, GLuint mask, GLuint filter)
  {
    if(pDest == NULL)
      BindBackBuffer();
    else
      pDest->Bind();
    if(s_BoundFrameBufferRead != this)
    {
      glBindFramebuffer(GL_READ_BUFFER, m_framebuffer);
      s_BoundFrameBufferRead = this;
    }

    glBlitFramebuffer(srcRect.x, srcRect.y, srcRect.x + srcRect.width, srcRect.y + srcRect.height,
                      dstRect.x, dstRect.y, dstRect.x + dstRect.width, dstRect.y + dstRect.height, mask, filter);
  }
}