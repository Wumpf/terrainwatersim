#pragma once

#include "../ShaderDataMetaInfo.h"

namespace gl
{
  class UniformBuffer
  {
  public:
    UniformBuffer();
    ~UniformBuffer();

    ezResult Init(ezUInt32 uiBufferSizeBytes, const ezString& sBufferName);
    ezResult Init(const gl::ShaderObject& shader, const ezString& sBufferName);
    /// will try to create a uniform buffer that matches all the given meta infos. Performs sanity checks if there's something contradictory
    ezResult Init(const ezDynamicArray<const gl::ShaderObject*>& shaders, const ezString& sBufferName);

    class Variable : public gl::ShaderVariable<UniformVariableInfo>
    {
    public:
      Variable() : ShaderVariable(), m_pUniformBuffer(NULL) { }
      Variable(const UniformVariableInfo& metaInfo, UniformBuffer* pUniformBuffer) :
        ShaderVariable(metaInfo), m_pUniformBuffer(pUniformBuffer) {}

      void Set(const void* pData, ezUInt32 uiSizeInBytes) EZ_OVERRIDE;

      using gl::ShaderVariable<UniformVariableInfo>::Set;
    private:
      UniformBuffer* m_pUniformBuffer;
    };


    bool ContainsVariable(const ezString& sVariableName) const       { return m_Variables.Find(sVariableName).IsValid(); }
    UniformBuffer::Variable& operator[] (const ezString& sVariableName);

    /// \brief Sets data in buffer directly.
    /// Given data block will be set dirty and copied with the next BindBuffer/UpdateGPUData call
    void SetData(const void* pData, ezUInt32 uiDataSize, ezUInt32 uiOffset);

    /// Updates gpu data if necessary and binds buffer
    ezResult BindBuffer(GLuint locationIndex);

    const ezString& GetBufferName() const { return m_sBufferName; }


    /// \brief Updates gpu UBO with dirty marked data
    /// Buffer should be already binded. Will be performed by BindBuffer by default.
    ezResult UpdateGPUData();

  private:

    BufferId    m_BufferObject;
    ezUInt32    m_uiBufferSizeBytes;
    ezString    m_sBufferName;

    /// where the currently dirty range of the buffer starts (bytes)
    ezUInt32 m_uiBufferDirtyRangeStart;
    /// where the currently dirty range of the buffer ends (bytes)
    ezUInt32 m_uiBufferDirtyRangeEnd;
    /// local copy of the buffer data
    ezInt8* m_pBufferData;

    /// meta information
    ezMap<ezString, Variable> m_Variables;  /// \todo no ezHashTable possible?

  };

  #include "UniformBuffer.inl"
}

