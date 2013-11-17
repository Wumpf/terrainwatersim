template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(float f)            
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::FLOAT, "Variable type does not match!");
  Set(&f, sizeof(f));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec2& v)    
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::FLOAT_VEC2, "Variable type does not match!");
  Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec3& v)    
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::FLOAT_VEC3, "Variable type does not match!");
  Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec4& v)    
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::FLOAT_VEC4, "Variable type does not match!");
  Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezMat3& m)    
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::FLOAT_MAT3, "Variable type does not match!");
  Set(&m, sizeof(m));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezMat4& m)    
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::FLOAT_MAT4, "Variable type does not match!");
  Set(&m, sizeof(m));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(double f)           
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::DOUBLE, "Variable type does not match!");
  Set(&f, sizeof(f));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec2d& v)   
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::DOUBLE_VEC2, "Variable type does not match!");
  Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec3d& v)   
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::DOUBLE_VEC3, "Variable type does not match!");
  Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec4d& v)   
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::DOUBLE_VEC4, "Variable type does not match!");
  Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezMat3d& m)   
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::FLOAT_MAT3, "Variable type does not match!");
  Set(&m, sizeof(m));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezMat4d& m)   
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::FLOAT_MAT4, "Variable type does not match!");
  Set(&m, sizeof(m));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(ezUInt32 ui)         
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::UNSIGNED_INT, "Variable type does not match!");
  Set(&ui, sizeof(ui));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec2U32& v) 
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::UNSIGNED_INT_VEC2, "Variable type does not match!");
  Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec3Template<ezUInt32>& v) 
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::UNSIGNED_INT_VEC3, "Variable type does not match!");
  Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec4Template<ezUInt32>& v) 
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::UNSIGNED_INT_VEC4, "Variable type does not match!");
  Set(&v, sizeof(v));
}

template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(ezInt32 i)         
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::INT, "Variable type does not match!");
  Set(&i, sizeof(i));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec2I32& v) 
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::INT_VEC2, "Variable type does not match!");
  Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec3Template<ezInt32>& v) 
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::INT_VEC3, "Variable type does not match!");
  Set(&v, sizeof(v));
}
template<typename VariableType>
inline void ShaderVariable<VariableType>::Set(const ezVec4Template<ezInt32>& v) 
{
  EZ_ASSERT(m_MetaInfo.Type == ShaderVariableType::INT_VEC4, "Variable type does not match!");
  Set(&v, sizeof(v));
}