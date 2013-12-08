#pragma once

#include <Foundation/Containers/List.h>
#include <Foundation/Containers/Set.h>
#include "GlobalEvents.h"

#include <AntTweakBar.h>

class AntTweakBarInterface
{
public:
  AntTweakBarInterface(void);
  ~AntTweakBarInterface(void);

  ezResult Init();
  void Render();

  template<typename Function>
  void AddButton(const ezString& name, const ezString& category, Function& triggerCallback);

private:
  void WindowMessageEventHandler(const GlobalEvents::Win32Message& message);

  struct CTwBar* m_pTweakBar;
  ezList<GlobalEvents::Win32Message> m_MessageQueue;

  ezSet<ezString> m_RegisteredStatisticKeys;

  static const ezUInt32 m_maxStringLength = 256;
  char m_szFpsInfo[m_maxStringLength];
  char m_szFrameTimeInfo[m_maxStringLength];

  /// Buffer for C++ function object callback that will be mapped to C-Function calls for use with TwBar
  /// Needs to be static to be available from C-Calls
  static ezStatic<ezDynamicArray<ezUInt8>> m_callbackFunctionObjectbuffer;
};

#include "AntTweakBarInterface.inl"