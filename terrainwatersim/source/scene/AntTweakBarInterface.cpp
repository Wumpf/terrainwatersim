#include "PCH.h"
#include "AntTweakBarInterface.h"

#include "config/GlobalCVar.h"

#include <Foundation/Utilities/Stats.h>

ezStatic<ezDynamicArray<ezUInt8>> AntTweakBarInterface::m_callbackFunctionObjectbuffer;

static TwType TwTypeFromCVarType(ezCVarType::Enum type)
{
  TwType twType = TW_TYPE_UNDEF;
  switch (type)
  {
  case ezCVarType::Bool:
    twType = TW_TYPE_BOOLCPP;
    break;
  case ezCVarType::Int:
    twType = TW_TYPE_INT32;
    break;
  case ezCVarType::String:
    twType = TW_TYPE_CDSTRING;
    break;
  case ezCVarType::Float:
    twType = TW_TYPE_FLOAT;
    break;
  }
  return twType;
}

template<typename Type, ezCVarType::Enum CVarType>
static void SetCVarValue(ezTypedCVar<Type, CVarType>& cvar, const void *value)
{
  cvar = *reinterpret_cast<const Type*>(value);
}

template<typename Type, ezCVarType::Enum CVarType>
static void GetCVarValue(ezTypedCVar<Type, CVarType>& cvar, void *value)
{
  *reinterpret_cast<Type*>(value) = cvar;
}
template<typename Type>
static void SetVarValue(Type& var, const void *value)
{
  var = *reinterpret_cast<const Type*>(value);
}

template<typename Type>
static void GetVarValue(Type& var, void *value)
{
  *reinterpret_cast<Type*>(value) = var;
}


#define ADD_CVAR_TO_TWEAKBAR_RW(cvar) \
  do { \
    TwSetVarCallback setFkt = [](const void *value, void *clientData) { \
      SetCVarValue(cvar, value); \
    }; \
    TwGetVarCallback getFkt = [](void *value, void *clientData) { \
      GetCVarValue(cvar, value); \
    }; \
    TwAddVarCB(m_pTweakBar, cvar.GetName(), TwTypeFromCVarType(cvar.GetType()), setFkt, getFkt, NULL, cvar.GetDescription()); \
    const char* errorDesc = TwGetLastError(); \
    if(errorDesc != NULL) ezLog::SeriousWarning("Tw error: %s", errorDesc); \
  } while(false)

/*
#define ADD_VAR_WITH_EVT_TO_TWEAKBAR_RW(var, twType, name, desc) \
  do { \
    TwSetVarCallback setFkt = [](const void *value, void *clientData) { \
      SetVarValue(var, value); \
      var##_changedEvent.GetStatic().Broadcast(var); \
    }; \
    TwGetVarCallback getFkt = [](void *value, void *clientData) { \
      GetVarValue(var, value); \
    }; \
    TwAddVarCB(m_pTweakBar, (name), (twType), setFkt, getFkt, NULL, (desc)); \
    const char* errorDesc = TwGetLastError(); \
    if(errorDesc != NULL) ezLog::SeriousWarning("Tw error: %s", errorDesc); \
  } while(false)
*/  
  
#define ADD_CVAR_TO_TWEAKBAR_RO(cvar) \
  do { \
    TwGetVarCallback getFkt = [](void *value, void *clientData) { \
      GetCVarValue(cvar, value); \
    }; \
    TwAddVarCB(m_pTweakBar, cvar.GetName(), TwTypeFromCVarType(cvar.GetType()), NULL, getFkt, NULL, cvar.GetDescription()); \
    const char* errorDesc = TwGetLastError(); \
    if(errorDesc != NULL) ezLog::SeriousWarning("Tw error: %s", errorDesc); \
  } while(false)

#define ADD_STAT_TO_TWEAKBAR(statname, param) \
  do { \
    TwGetVarCallback getFkt = [](void *value, void *clientData) { \
      *reinterpret_cast<const char**>(value) = ezStats::GetStat(statname); \
    }; \
    TwAddVarCB(m_pTweakBar, statname, TW_TYPE_CDSTRING, NULL, getFkt, NULL, param); \
    const char* errorDesc = TwGetLastError(); \
if(errorDesc != NULL) ezLog::SeriousWarning("Tw error: %s", errorDesc); \
  } while(false)


AntTweakBarInterface::AntTweakBarInterface(void) :
   m_pTweakBar(NULL)
{
  m_szFpsInfo[0] = '\0';
  m_szFrameTimeInfo[0] = '\0';
}

AntTweakBarInterface::~AntTweakBarInterface(void)
{
  if(!m_pTweakBar)
  {
    TwTerminate();
    GlobalEvents::g_pWindowMessage->RemoveEventHandler(ezEvent<const GlobalEvents::Win32Message&>::Handler(&AntTweakBarInterface::WindowMessageEventHandler, this));
  }

  for(auto it = m_entries.GetIterator(); it.IsValid(); ++it)
    EZ_DEFAULT_DELETE(*it);
}

void AntTweakBarInterface::CheckTwError()
{
  const char* errorDesc = TwGetLastError();
  if(errorDesc != NULL)
    ezLog::SeriousWarning("Tw error: %s", errorDesc);
}

void AntTweakBarInterface::AddButton(const ezString& name, ezDelegate<void()>& triggerCallback, const ezString& twDefines)
{
  EntryButton* pEntry = EZ_DEFAULT_NEW(EntryButton)();
  pEntry->name = name;
  pEntry->triggerCallback = triggerCallback;
  m_entries.PushBack(pEntry);

  TwButtonCallback fkt = [](void* entry) {
    static_cast<EntryButton*>(entry)->triggerCallback();
  };

  TwAddButton(m_pTweakBar, name.GetData(), fkt, m_entries.PeekBack(), twDefines.GetData());
  CheckTwError();
}

void AntTweakBarInterface::AddReadOnly(const ezString& name, ezDelegate<ezString()>& getValue, const ezString& twDefines)
{
  EntryReadOnly* pEntry = EZ_DEFAULT_NEW(EntryReadOnly)();
  pEntry->name = name;
  pEntry->getValue = getValue;
  m_entries.PushBack(pEntry);

  TwGetVarCallback getFkt = [](void *value, void *clientData) {
    *static_cast<const char**>(value) = static_cast<EntryReadOnly*>(clientData)->getValue().GetData();
  };

  TwAddVarCB(m_pTweakBar, name.GetData(), TW_TYPE_CDSTRING, NULL, getFkt, m_entries.PeekBack(), twDefines.GetData());
  CheckTwError();
}

void AntTweakBarInterface::AddReadWrite(const ezString& name, ezDelegate<ezVariant()>& getValue, ezDelegate<void(const ezVariant&)>& setValue, const ezString& twDefines)
{
  TwGetVarCallback getFkt;
  TwSetVarCallback setFkt;
  TwType varType;

  ezVariant testValue(getValue());

  switch(testValue.GetType())
  {
  case ezVariant::Type::Bool:
    getFkt = [](void *value, void *clientData) {
      *static_cast<bool*>(value) = static_cast<EntryReadWrite*>(clientData)->getValue().Get<bool>();
    };
    setFkt = [](const void *value, void *clientData) {
      static_cast<EntryReadWrite*>(clientData)->setValue(ezVariant(*static_cast<const bool*>(value)));
    };
    varType = TW_TYPE_BOOLCPP;
    break;

  case ezVariant::Type::Float:
    getFkt = [](void *value, void *clientData) {
      *static_cast<float*>(value) = static_cast<EntryReadWrite*>(clientData)->getValue().Get<float>();
    };
    setFkt = [](const void *value, void *clientData) {
      static_cast<EntryReadWrite*>(clientData)->setValue(ezVariant(*static_cast<const float*>(value)));
    };
    varType = TW_TYPE_FLOAT;
    break;

  default:
    ezLog::Error("AntTweakBarInterface::AddReadWrite: Variables \"%s\" has an unsupported variant type!", name.GetData());
    return;
  }

  EntryReadWrite* pEntry = EZ_DEFAULT_NEW(EntryReadWrite)();
  pEntry->name = name;
  pEntry->getValue = getValue;
  pEntry->setValue = setValue;
  m_entries.PushBack(pEntry);

  TwAddVarCB(m_pTweakBar, name.GetData(), varType, setFkt, getFkt, m_entries.PeekBack(), twDefines.GetData());
  CheckTwError();
}

void AntTweakBarInterface::AddSeperator(const ezString& name, const ezString& twDefines)
{
  TwAddSeparator(m_pTweakBar, name.GetData(), twDefines.GetData());
  CheckTwError();
}

ezResult AntTweakBarInterface::Init()
{
  ezLogBlock("AntTweakBarInit");

  if (!TwInit(TW_OPENGL, NULL))
  {
    ezLog::Error("AntTweakBar initialization failed: %s\n", TwGetLastError());
    return EZ_FAILURE;
  }

  TwWindowSize(GeneralConfig::g_ResolutionWidth.GetValue(), GeneralConfig::g_ResolutionHeight.GetValue());

  // Create a tweak bar
  m_pTweakBar = TwNewBar("TweakBar");

  static const ezSizeU32 tweakBarSize(330, 600);
  ezStringBuilder stringBuilder;
  stringBuilder.Format(" TweakBar size='%i %i' ", tweakBarSize.width, tweakBarSize.height);
  TwDefine(stringBuilder.GetData());
  stringBuilder.Format(" TweakBar position='%i %i' ", GeneralConfig::g_ResolutionWidth - tweakBarSize.width - 20, 20);
  TwDefine(stringBuilder.GetData());

  TwDefine(" TweakBar refresh=0.2 ");
  TwDefine(" TweakBar contained=true "); // TweakBar must be inside the window.
  //TwDefine(" TweakBar alpha=200 ");

  // register eventhandler
  GlobalEvents::g_pWindowMessage->AddEventHandler(ezEvent<const GlobalEvents::Win32Message&>::Handler(&AntTweakBarInterface::WindowMessageEventHandler, this));

  return EZ_SUCCESS;
}

void AntTweakBarInterface::WindowMessageEventHandler(const GlobalEvents::Win32Message& message)
{
  // puffer message to avoid recursive calling 
  m_messageQueue.PushBack(message);
}

void AntTweakBarInterface::Render()
{
  // unwind message buffer - this avoid recursive calls
  while(!m_messageQueue.IsEmpty())
  {
    GlobalEvents::Win32Message message = m_messageQueue.PeekFront();
    TwEventWin(message.wnd, message.msg, message.wParam, message.lParam);
    m_messageQueue.PopFront();
  }

  TwDraw();
}