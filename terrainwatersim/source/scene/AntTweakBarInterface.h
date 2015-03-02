#pragma once

#include <Foundation/Containers/List.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Types/Variant.h>
#include "GlobalEvents.h"

#include <AntTweakBar.h>

class AntTweakBarInterface
{
public:
  AntTweakBarInterface(void);
  ~AntTweakBarInterface(void);

  ezResult Init();
  void Render();

  void AddButton(const ezString& name, ezDelegate<void()>& triggerCallback, const ezString& twDefines= "");
  void AddReadOnly(const ezString& name, ezDelegate<const char*()>& getValue, const ezString& twDefines = "");
  void AddReadWrite(const ezString& name, ezDelegate<ezVariant()> &getValue, ezDelegate<void(const ezVariant&)>& setValue, const ezString& twDefines = "");
  void AddSeperator(const ezString& name, const ezString& twDefines = "");

private:  
  void WindowMessageEventHandler(const GlobalEvents::Win32Message& message);
  void CheckTwError();

  struct CTwBar* m_pTweakBar;
  ezList<GlobalEvents::Win32Message> m_messageQueue;

  ezSet<ezString> m_registeredStatisticKeys;

  static const ezUInt32 m_maxStringLength = 256;
  char m_szFpsInfo[m_maxStringLength];
  char m_szFrameTimeInfo[m_maxStringLength];

  struct EntryBase
  {
    ezString name;
    ezString category;
  };
  struct EntryButton : public EntryBase
  {
    ezDelegate<void()> triggerCallback;
  };
  struct EntryReadOnly : public EntryBase
  {
    ezDelegate<const char*()> getValue;
  };
  struct EntryReadWrite : public EntryBase
  {
    ezDelegate<ezVariant()> getValue;
    ezDelegate<void(const ezVariant&)> setValue;
  };

  ezList<EntryBase*> m_entries;

  /// Buffer for C++ function object callback that will be mapped to C-Function calls for use with TwBar
  /// Needs to be static to be available from C-Calls
  static ezDynamicArray<ezUInt8> m_callbackFunctionObjectbuffer;
};