#include "PCH.h"
#include "SamplerObject.h"

namespace gl
{
  ezHashTable<SamplerObject::Desc, SamplerObject> SamplerObject::s_existingSamplerObjects;
  const SamplerObject* SamplerObject::s_pSamplerBindings[32];

  SamplerObject::Desc::Desc(Filter minFilter, Filter magFilter, Filter mipFilter,
    Border borderHandling, ezUInt32 maxAnisotropy, ezColor borderColor) :
    Desc(minFilter, magFilter, mipFilter, borderHandling, borderHandling, borderHandling, maxAnisotropy, borderColor)
  {}

  SamplerObject::Desc::Desc(Filter minFilter, Filter magFilter, Filter mipFilter,
    Border borderHandlingU, Border borderHandlingV, Border m_borderHandlingW,
    ezUInt32 maxAnisotropy, ezColor borderColor) :
    minFilter(minFilter),
    magFilter(magFilter),
    mipFilter(mipFilter),
    borderHandlingU(borderHandlingU),
    borderHandlingV(borderHandlingV),
    borderHandlingW(m_borderHandlingW),
    maxAnisotropy(maxAnisotropy),
    borderColor(borderColor)
  {
  }

  SamplerObject::SamplerObject(SamplerObject&& cpy) :
    m_samplerId(cpy.m_samplerId)
  {
    cpy.m_samplerId = std::numeric_limits<GLuint>::max();
  }


  SamplerObject::SamplerObject(const Desc& desc)
  {
    EZ_ASSERT(desc.maxAnisotropy > 0, "Anisotropy level of 0 is invalid! Must be between 1 and GPU's max.");

    glGenSamplers(1, &m_samplerId);
    glSamplerParameteri(m_samplerId, GL_TEXTURE_WRAP_S, static_cast<GLint>(desc.borderHandlingU));
    glSamplerParameteri(m_samplerId, GL_TEXTURE_WRAP_T, static_cast<GLint>(desc.borderHandlingV));
    glSamplerParameteri(m_samplerId, GL_TEXTURE_WRAP_R, static_cast<GLint>(desc.borderHandlingW));

    GLint minFilterGl;
    if(desc.minFilter == Filter::NEAREST)
      minFilterGl = desc.mipFilter == Filter::NEAREST ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR;
    else
      minFilterGl = desc.mipFilter == Filter::NEAREST ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;

    glSamplerParameteri(m_samplerId, GL_TEXTURE_MIN_FILTER, minFilterGl);
    glSamplerParameteri(m_samplerId, GL_TEXTURE_MAG_FILTER, desc.magFilter == Filter::NEAREST ? GL_NEAREST : GL_LINEAR);

    glSamplerParameteri(m_samplerId, GL_TEXTURE_MAX_ANISOTROPY_EXT, desc.maxAnisotropy);

    glSamplerParameterfv(m_samplerId, GL_TEXTURE_BORDER_COLOR, desc.borderColor);
  }

  SamplerObject::~SamplerObject()
  {
    if(m_samplerId != std::numeric_limits<GLuint>::max())
      glDeleteSamplers(1, &m_samplerId);
  }

  const SamplerObject& SamplerObject::GetSamplerObject(const Desc& desc)
  {
    gl::SamplerObject* pSamplerObject;

    bool exists = s_existingSamplerObjects.TryGetValue(desc, pSamplerObject);
    if(!exists)
    {
      gl::SamplerObject samplerObject(desc);
      s_existingSamplerObjects.Insert(desc, samplerObject);

      // prevent deletion (since it's not guaranteed that the rvalue ctor already did that event if the object scope is further shrinked to the insert method)
      samplerObject.m_samplerId = std::numeric_limits<GLuint>::max();

      // additional search necessary to avoid copies
      s_existingSamplerObjects.TryGetValue(desc, pSamplerObject);
    }

    return *pSamplerObject;
  }

  void SamplerObject::BindSampler(ezUInt32 textureStage) const
  {
    EZ_ASSERT(textureStage < sizeof(s_pSamplerBindings) / sizeof(SamplerObject*), "Can't bind sampler to slot %i. Maximum number of slots is %i", textureStage, sizeof(s_pSamplerBindings) / sizeof(SamplerObject*));
    if(s_pSamplerBindings[textureStage] != this)
    {
      glBindSampler(textureStage, m_samplerId);
      s_pSamplerBindings[textureStage] = this;
    }
  }
}