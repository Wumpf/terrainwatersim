#include "PCH.h"
#include "AntTweakBarInterface.h"

#include "config/GlobalCVar.h"

#include <AntTweakBar.h>
#include <Foundation/Utilities/Stats.h>


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

  // General
  ADD_STAT_TO_TWEAKBAR("Frame time", "group=General");
  ADD_STAT_TO_TWEAKBAR("Frames per second", "group=General");

    // AntiAliasing
/*  TwEnumVal antialiasingValues[] = { { 0, "None" }, { 2, "2x" }, { 4, "4x" }, { 8, "8x" } };
  auto enumType = TwDefineEnum("AntiAliasing_enum", antialiasingValues, 4);
  TwAddVarCB(m_pTweakBar, GeneralConfig::g_MSAASamples.GetName(), enumType,
              [](const void *value, void *clientData) { SetCVarValue(GeneralConfig::g_MSAASamples, value); },
              [](void *value, void *clientData) { GetCVarValue(GeneralConfig::g_MSAASamples, value); },
              NULL, "group=General");
  const char* errorDesc = TwGetLastError();
  if(errorDesc != NULL) ezLog::SeriousWarning("Tw error: %s", errorDesc);
*/
  // terrain
  //TwAddSeparator(m_pTweakBar, NULL, "group=Rendering");
  ADD_STAT_TO_TWEAKBAR("Terrain Draw Time", "group=\'Terrain Rendering\'");
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::TerrainRendering::g_Wireframe);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::TerrainRendering::g_PixelPerTriangle);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::TerrainRendering::g_UseAnisotropicFilter);  
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::TerrainRendering::g_SpecularPower);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::TerrainRendering::g_FresnelReflection);

//  ADD_VAR_WITH_EVT_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_bigDepthColor, TW_TYPE_COLOR3F, "BigDepth Color", "group=\'Water Rendering\'");

  ADD_STAT_TO_TWEAKBAR("Water Draw Time", "group=\'Water Rendering\'");
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_wireframe);

  TwAddSeparator(m_pTweakBar, "Colors", "group=\'Water Rendering\'");

  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_bigDepthColorR);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_bigDepthColorG);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_bigDepthColorB);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_extinctionCoefficientsR);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_extinctionCoefficientsG);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_extinctionCoefficientsB);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_surfaceColorR);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_surfaceColorG);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_surfaceColorB);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_opaqueness);

  TwAddSeparator(m_pTweakBar, "Flow", "group=\'Water Rendering\'");

  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_normalMapRepeat);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_normalLayerBlendInveral);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_speedToNormalDistortion);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::WaterRendering::g_flowDistortionStrength);

  ADD_STAT_TO_TWEAKBAR("Simulation Time", "group=\'Simulation\'");
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::Simulation::g_SimulationStepsPerSecond);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::Simulation::g_FlowDamping);
  ADD_CVAR_TO_TWEAKBAR_RW(SceneConfig::Simulation::g_FlowAcceleration);

  // register eventhandler
  GlobalEvents::g_pWindowMessage->AddEventHandler(ezEvent<const GlobalEvents::Win32Message&>::Handler(&AntTweakBarInterface::WindowMessageEventHandler, this));

  return EZ_SUCCESS;
}

void AntTweakBarInterface::WindowMessageEventHandler(const GlobalEvents::Win32Message& message)
{
  // puffer message to avoid recursive calling 
  m_MessageQueue.PushBack(message);
}

void AntTweakBarInterface::Render()
{
  // unwind message buffer - this avoid recursive calls
  while(!m_MessageQueue.IsEmpty())
  {
    GlobalEvents::Win32Message message = m_MessageQueue.PeekFront();
    TwEventWin(message.wnd, message.msg, message.wParam, message.lParam);
    m_MessageQueue.PopFront();
  }

  TwDraw();
}