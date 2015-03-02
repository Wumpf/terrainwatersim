

inline void UniformBuffer::Variable::Set(const void* pData, ezUInt32 sizeInBytes)
{
  EZ_ASSERT(m_pUniformBuffer != NULL, "Uniform buffer variable is not assigned to an Uniform Buffer!");
  EZ_ASSERT(sizeInBytes != 0, "Given size to set for uniform variable is 0.");
  EZ_ASSERT(pData != NULL, "Data to set for uniform variable is NULL.");

  m_pUniformBuffer->SetData(pData, sizeInBytes, m_MetaInfo.iBlockOffset);
}

inline void UniformBuffer::SetData(const void* pData, ezUInt32 dataSize, ezUInt32 offset)
{
  EZ_ASSERT(dataSize != 0, "Given size to set for uniform data is 0.");
  EZ_ASSERT(offset + dataSize <= m_uiBufferSizeBytes, "Data block doesn't fit into uniform buffer.");
  EZ_ASSERT(pData != NULL, "Data to copy into uniform is NULL.");

  m_uiBufferDirtyRangeStart = std::min(m_uiBufferDirtyRangeStart, offset);
  m_uiBufferDirtyRangeEnd = std::max(m_uiBufferDirtyRangeEnd, offset + dataSize);
  ezMemoryUtils::Copy(m_pBufferData + offset, reinterpret_cast<const char*>(pData), dataSize);
}

inline UniformBuffer::Variable& UniformBuffer::operator[] (const ezString& sVariableName)             
{
  EZ_ASSERT(m_Variables.KeyExists(sVariableName), "There is no variable named %s in the uniform buffer \"%s\"", sVariableName.GetData(), m_sBufferName.GetData());
  return m_Variables[sVariableName];
}