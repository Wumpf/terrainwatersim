template<typename Function>
void AntTweakBarInterface::AddButton(const ezString& name, const ezString& category, Function& triggerCallback)
{
  ezStringBuilder def(category.GetData());
  def.Prepend("group=\'");
  def.Append("\'");

  ezUInt32 fktObjDataOffset = m_callbackFunctionObjectbuffer.GetStatic().GetCount();

  ezArrayPtr<ezUInt8> pFktObjBuffer = EZ_DEFAULT_NEW_ARRAY(ezUInt8, sizeof(Function));
  ezMemoryUtils::Copy(reinterpret_cast<Function*>(pFktObjBuffer.GetPtr()), &triggerCallback, 1);
  m_callbackFunctionObjectbuffer.GetStatic().PushBackRange(pFktObjBuffer);
  EZ_DEFAULT_DELETE_ARRAY(pFktObjBuffer);

  auto fkt = [](void* functionObjectIdx) {
    ezUInt32 fktObjDataOffset = reinterpret_cast<ezUInt32>(functionObjectIdx);
    (*reinterpret_cast<Function*>(&m_callbackFunctionObjectbuffer.GetStatic()[fktObjDataOffset]))();
  };

  TwAddButton(m_pTweakBar, name.GetData(), fkt, reinterpret_cast<void*>(fktObjDataOffset), def.GetData());
}
