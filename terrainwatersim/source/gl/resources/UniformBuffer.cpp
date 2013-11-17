#include "PCH.h"
#include "UniformBuffer.h"
#include "../GLUtils.h"
#include "../ShaderObject.h"

namespace gl
{

  UniformBuffer::UniformBuffer() :
    m_BufferObject(9999),
    m_uiBufferSizeBytes(0),
    m_Variables(),
    m_sBufferName(""),
    m_pBufferData(NULL)
  {
  }

  UniformBuffer::~UniformBuffer(void)
  {
    EZ_DEFAULT_DELETE(m_pBufferData);
    glDeleteBuffers(1, &m_BufferObject);
  }

  ezResult UniformBuffer::Init(ezUInt32 uiBufferSizeBytes, const ezString& sBufferName)
  {
    m_sBufferName = sBufferName;
    m_uiBufferSizeBytes = uiBufferSizeBytes;

    EZ_DEFAULT_DELETE(m_pBufferData);
    m_pBufferData = EZ_DEFAULT_NEW_RAW_BUFFER(ezInt8, uiBufferSizeBytes);
    m_uiBufferDirtyRangeEnd = m_uiBufferDirtyRangeStart = 0;

    glGenBuffers(1, &m_BufferObject);
    glBindBuffer(GL_UNIFORM_BUFFER, m_BufferObject);
    // glBufferStorage(GL_UNIFORM_BUFFER, m_uiBufferSizeBytes, NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
    glBufferData(GL_UNIFORM_BUFFER, m_uiBufferSizeBytes, NULL, GL_DYNAMIC_DRAW);

    return gl::Utils::CheckError("glBufferData");
  }

  ezResult UniformBuffer::Init(const gl::ShaderObject& shader, const ezString& sBufferName)
  {
    auto uniformBufferInfoIterator = shader.GetUniformBufferInfo().Find(sBufferName);
    if(!uniformBufferInfoIterator.IsValid())
    {
      ezLog::Error("shader doesn't contain a uniform buffer meta block info with the name \"%s\"!", sBufferName.GetData());
      return EZ_FAILURE;
    }

    for(auto it = uniformBufferInfoIterator.Value().Variables.GetIterator(); it.IsValid(); ++it)
      m_Variables.Insert(it.Key(), Variable(it.Value(), this));

    return Init(uniformBufferInfoIterator.Value().iBufferDataSizeByte, sBufferName);
  }

  ezResult UniformBuffer::Init(const ezDynamicArray<const gl::ShaderObject*>& metaInfos, const ezString& sBufferName)
  {
    EZ_ASSERT(!metaInfos.IsEmpty(), "Meta info list is empty!");
    EZ_ASSERT(metaInfos[0] != NULL, "Shader is NULL");
    ezResult result = Init(*metaInfos[0], sBufferName);
    if(result == EZ_FAILURE)
      return result;

    for(ezUInt32 i=1; i<metaInfos.GetCount(); ++i)
    {
      if(metaInfos[i] == NULL) // the first was fatal, this one is skippable
      {
        ezLog::SeriousWarning("ShaderObject %i in list for uniform buffer \"%s\" initialization doesn't contain the needed meta data! Skipping..", i, sBufferName.GetData());
        continue;
      }
      auto uniformBufferInfoIterator = metaInfos[i]->GetUniformBufferInfo().Find(sBufferName);
      if(!uniformBufferInfoIterator.IsValid()) // the first was fatal, this one is skippable
      {
        ezLog::SeriousWarning("ShaderObject %i in list for uniform buffer \"%s\" initialization doesn't contain the needed meta data! Skipping..", i, sBufferName.GetData());
        continue;
      }

      // sanity check
      if(uniformBufferInfoIterator.Value().iBufferDataSizeByte != m_uiBufferSizeBytes)
      {
        ezLog::SeriousWarning("ShaderObject %i in list for uniform buffer \"%s\" initialization gives size %i, first shader gave size %i! Skipping..", 
                                 i, sBufferName.GetData(), uniformBufferInfoIterator.Value().iBufferDataSizeByte, m_uiBufferSizeBytes);    
        continue;
      }

      for(auto varIt = uniformBufferInfoIterator.Value().Variables.GetIterator(); varIt.IsValid(); ++varIt)
      {
        auto ownVarIt = m_Variables.Find(varIt.Key());
        if(ownVarIt.IsValid())  // overlap
        {
          // sanity check
          const gl::UniformVariableInfo* ownVar =  &ownVarIt.Value().GetMetaInfo();
          const gl::UniformVariableInfo* otherVar = &varIt.Value();
          if(ezMemoryUtils::ByteCompare(ownVar, otherVar) != 0)
          {
            ezLog::Error("ShaderObject %i in list for uniform buffer \"%s\" initialization has a description of variable \"%s\" that doesn't match with the ones before!", 
                     i, sBufferName.GetData(), varIt.Key().GetData());   
          }
        }
        else // new one
        {

          m_Variables.Insert(varIt.Key(), Variable(varIt.Value(), this));
          // todo? check overlaps
        }
      }
    }

    return EZ_SUCCESS;
  }

  ezResult UniformBuffer::UpdateGPUData()
  {
    if(m_uiBufferDirtyRangeEnd <= m_uiBufferDirtyRangeStart)
      return EZ_SUCCESS;

    glBindBuffer(GL_UNIFORM_BUFFER, m_BufferObject);
    glBufferSubData(GL_UNIFORM_BUFFER, m_uiBufferDirtyRangeStart, m_uiBufferDirtyRangeEnd - m_uiBufferDirtyRangeStart, m_pBufferData + m_uiBufferDirtyRangeStart);

    m_uiBufferDirtyRangeEnd = std::numeric_limits<ezUInt32>::min();
    m_uiBufferDirtyRangeStart = std::numeric_limits<ezUInt32>::max();

    return gl::Utils::CheckError("glBufferSubData");
  }

  ezResult UniformBuffer::BindBuffer(GLuint locationIndex)
  {
    if(UpdateGPUData() == EZ_FAILURE)
      return EZ_FAILURE;

    glBindBufferBase(GL_UNIFORM_BUFFER, locationIndex, m_BufferObject);

    return gl::Utils::CheckError("glBindBufferBase");
  }

}