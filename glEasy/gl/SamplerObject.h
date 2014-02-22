#pragma once

#include <Foundation\Containers\HashTable.h>

namespace gl
{

  /// \brief Easy to use OpenGL Sampler Object
  ///
  /// Will store all runtime create sampler objects in a intern static list. List will be cleared on program shutdown or on explicit call.
  /// Also binds sampler only if not already bound.
  class SamplerObject
  {
  public:
    enum class Filter
    {
      NEAREST,
      LINEAR
    };
    enum class Border
    {
      REPEAT = GL_REPEAT,
      MIRROR = GL_MIRRORED_REPEAT,
      CLAMP = GL_CLAMP_TO_EDGE,
      BORDER = GL_CLAMP_TO_BORDER
    };

    struct Desc
    {
      Desc(Filter minFilter, Filter magFilter, Filter mipFilter, Border borderHandling,
        ezUInt32 maxAnisotropy = 1, ezColor borderColor = ezColor::GetWhite());

      Desc(Filter minFilter, Filter magFilter, Filter mipFilter, Border borderHandlingU, Border borderHandlingV, Border m_borderHandlingW,
        ezUInt32 maxAnisotropy = 1, ezColor borderColor = ezColor::GetWhite());

      bool operator == (const Desc& lft) { return ezMemoryUtils::ByteCompare(this, &lft) == 0;  }

      Filter minFilter;
      Filter magFilter;
      Filter mipFilter;
      Border borderHandlingU;
      Border borderHandlingV;
      Border borderHandlingW;
      ezUInt32 maxAnisotropy;
      ezColor borderColor;

      EZ_DECLARE_POD_TYPE();
    };

    /// Will create or return a existing SamplerObject.
    static const SamplerObject& GetSamplerObject(const Desc& desc);

    /// Binds sampler to given texture stage if not already bound.
    void BindSampler(ezUInt32 textureStage) const;

    /// Removes all existing sampler objects.
    void ClearAllCachedSamplerObjects() { s_existingSamplerObjects.Clear(); }

    SamplerId GetInternSamplerId() const { return m_samplerId; }

    ~SamplerObject();

  private:
    SamplerObject(const Desc& samplerDesc);
    SamplerObject(SamplerObject&& cpy);
 

    static const SamplerObject* s_pSamplerBindings[32];
    static ezHashTable<Desc, SamplerObject> s_existingSamplerObjects;

    SamplerId m_samplerId;
  };

};
