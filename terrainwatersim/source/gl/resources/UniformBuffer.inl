

inline void UniformBuffer::Variable::Set(const void* pData, ezUInt32 uiSizeInBytes)
{
  EZ_ASSERT(m_pUniformBuffer != NULL, "Uniform buffer variable is not assigned to an Uniform Buffer!");
  EZ_ASSERT(uiSizeInBytes != 0, "Given size to set for uniform variable is 0.");
  EZ_ASSERT(pData != NULL, "Data to set for uniform variable is NULL.");

  m_pUniformBuffer->SetData(pData, uiSizeInBytes, m_MetaInfo.iBlockOffset);
}

inline void UniformBuffer::SetData(const void* pData, ezUInt32 uiDataSize, ezUInt32 uiOffset)
{
  EZ_ASSERT(uiDataSize != 0, "Given size to set for uniform data is 0.");
  EZ_ASSERT(uiOffset + uiDataSize <= m_uiBufferSizeBytes, "Data block doesn't fit into uniform buffer.");
  EZ_ASSERT(pData != NULL, "Data to copy into uniform is NULL.");

  m_uiBufferDirtyRangeStart = std::min(m_uiBufferDirtyRangeStart, uiOffset);
  m_uiBufferDirtyRangeEnd = std::max(m_uiBufferDirtyRangeEnd, uiOffset + uiDataSize);
  ezMemoryUtils::Copy(m_pBufferData + uiOffset, reinterpret_cast<const char*>(pData), uiDataSize);
}

inline UniformBuffer::Variable& UniformBuffer::operator[] (const ezString& sVariableName)             
{
  EZ_ASSERT(m_Variables.Find(sVariableName).IsValid(), "There is no variable named %s in the uniform buffer \"%s\"", sVariableName.GetData(), m_sBufferName.GetData());
  return m_Variables[sVariableName];
}