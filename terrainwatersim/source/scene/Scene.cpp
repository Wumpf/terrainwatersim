#include "PCH.h"
#include "Scene.h"
#include "config/GlobalCVar.h"

#include "RenderWindow.h"

#include <gl/ScreenAlignedTriangle.h>
#include <gl/Font.h>
#include <gl/TimerQuery.h>
#include <gl/GLUtils.h>
#include <gl/resources/FramebufferObject.h>
#include <gl/resources/textures/Texture2D.h>

#include "math/camera/FreeCamera.h"

#include "Terrain.h"
#include "Background.h"
#include "PostProcessing.h"

#include "AntTweakBarInterface.h"

#include <Foundation/Utilities/Stats.h>
#include <Foundation/Basics/Types/Variant.h>



namespace SceneConfig
{
  namespace TerrainRendering
  {
    ezCVarBool g_Wireframe("Wireframe Terrain", false, ezCVarFlags::Save, "group='Terrain Rendering'");
    ezCVarFloat g_PixelPerTriangle("Aimed Pixel/Triangle", 25.0f, ezCVarFlags::Save, "group='Terrain Rendering' min=3.0 max=200");
    ezCVarBool g_UseAnisotropicFilter("Anisotropic Filter on/off", true, ezCVarFlags::Save, "group='Terrain Rendering'");
    ezCVarFloat g_FresnelReflection("Fresnel Reflection Coef", 0.1f, ezCVarFlags::Save, "group='Terrain Rendering' min=0.0 max=2.0 step = 0.01");
    ezCVarFloat g_SpecularPower("Specular Power", 4.0f, ezCVarFlags::Save, "group='Terrain Rendering' min=0.0 max=32.0 step = 0.5");
  }
  namespace WaterRendering
  {
    ezCVarBool g_wireframe("Wireframe Water", false, ezCVarFlags::Save, "group='Water Rendering'");
    CVarRGBImpl(g_surfaceColor, "Surface Color", ezVec3(0.0029f, 0.1788f, 0.27f), ezCVarFlags::Save, "group='Water Rendering' min=0.0 max=1.0 step=0.005");
    CVarRGBImpl(g_bigDepthColor, "Big-Depth Color", ezVec3(0.00195f, 0.00098f, 0.0725f), ezCVarFlags::Save, "group='Water Rendering' min=0.0 max=1.0 step=0.005");
    CVarRGBImpl(g_extinctionCoefficients, "Extinction Coefficients", ezVec3(0.1278f, 0.0735f, 0.5f), ezCVarFlags::Save, "group='Water Rendering' min=0.0 max=1.0 step=0.0025");
    ezCVarFloat g_opaqueness("Opaqueness", 0.01f, ezCVarFlags::Save, "group='Water Rendering' min=0.0 max=0.5 step=0.005");

    ezCVarFloat g_normalMapRepeat("Normalmap repeat", 30.0f, ezCVarFlags::Save, "group='Water Rendering' min=5.0 max=100.0 step=1");
    ezCVarFloat g_speedToNormalDistortion("Flow to normal Distortion", 0.01f, ezCVarFlags::Save, "group='Water Rendering' min=0.0 max=0.3 step=0.001");
    ezCVarFloat g_normalLayerBlendInveral("Normal layer blend interval", 0.01f, ezCVarFlags::Save, "group='Water Rendering' min=0.5 max=50.0 step=0.25");
    ezCVarFloat g_flowDistortionStrength("Flow distortion strength", 0.001f, ezCVarFlags::Save, "group='Water Rendering' min=0.0 max=0.1 step=0.0005");
  }

  namespace Simulation
  {
    ezCVarFloat g_simulationStepsPerSecond("Simulation steps per second", 60, ezCVarFlags::Save, "group='Simulation' min=30 max=300");
    ezCVarFloat g_flowDamping("Flow Damping", 0.98f, ezCVarFlags::Save, "group='Simulation' min=0.2 max=1.0 step=0.01");
    ezCVarFloat g_flowAcceleration("Flow Acceleration", 10.0f, ezCVarFlags::Save, "group='Simulation' min=0.5 max=100.0 step=0.1");
  }

  namespace PostPro
  {
    ezCVarFloat g_exposure("Exposure", 0.3f, ezCVarFlags::Save, "group='PostProcessing' min=0.0 max=2.0 step = 0.05");
    ezCVarFloat g_adaptationSpeed("Adaptation Speed", 0.5f, ezCVarFlags::Save, "group='PostProcessing' min=0 max=4 step=0.1");
  }
}

Scene::Scene(const RenderWindowGL& renderWindow) :
  m_pCopyShader(EZ_DEFAULT_NEW_UNIQUE(gl::ShaderObject, "copySceneShader")),

  m_pCamera(EZ_DEFAULT_NEW_UNIQUE(FreeCamera, ezAngle::Degree(70.0f), static_cast<float>(GeneralConfig::g_ResolutionWidth.GetValue()) / GeneralConfig::g_ResolutionHeight.GetValue())),
  m_pFont(EZ_DEFAULT_NEW_UNIQUE(gl::Font, "Arial", 20, renderWindow.GetWindowDC())),

  m_pWaterDrawTimer(EZ_DEFAULT_NEW_UNIQUE(gl::TimerQuery)),
  m_pTerrainDrawTimer(EZ_DEFAULT_NEW_UNIQUE(gl::TimerQuery)),
  m_pSimulationTimer(EZ_DEFAULT_NEW_UNIQUE(gl::TimerQuery)),

  m_pUserInterface(EZ_DEFAULT_NEW_UNIQUE(AntTweakBarInterface))
{
  EZ_LOG_BLOCK("Scene init");

  m_pTerrain = EZ_DEFAULT_NEW(Terrain)(GeneralConfig::GetScreenResolution());
  m_pBackground = EZ_DEFAULT_NEW(Background)(128);
  m_pPostProcessing = EZ_DEFAULT_NEW(PostProcessing)(GeneralConfig::GetScreenResolution());

  // global ubo inits
  InitGlobalUBO();

  ezVec3 vCameraPos(m_pTerrain->GetTerrainWorldSize() / 2, 200, m_pTerrain->GetTerrainWorldSize() / 2);
  m_pCamera->SetPosition(vCameraPos);

  // Shader for buffer copy operations
  m_pCopyShader->AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "screenTri.vert");
  m_pCopyShader->AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "textureOutput.frag");
  m_pCopyShader->CreateProgram();

  // Buffer setup
  RecreateScreenBuffers();

  // user interface
  m_pUserInterface->Init();
  InitConfig();

}

void Scene::InitGlobalUBO()
{
  m_CameraUBO.Init({ &m_pTerrain->GetTerrainShader(), &m_pTerrain->GetWaterShader(), &m_pBackground->GetBackgroundShader() }, "Camera");
  m_GlobalSceneInfo.Init({ &m_pTerrain->GetTerrainShader(), &m_pBackground->GetScatteringShader() }, "GlobalSceneInfo");

  /*    ezDynamicArray<const gl::ShaderObject*> timeUBOusingShader;
  cameraUBOusingShader.PushBack(&m_DirectVolVisShader);
  m_TimeUBO.Init(cameraUBOusingShader, "Time");
  */

  m_GlobalSceneInfo["GlobalDirLightDirection"].Set(ezVec3(1.5f, 1.0f, 1.5f).GetNormalized());
  m_GlobalSceneInfo["GlobalDirLightColor"].Set(ezVec3(0.98f, 0.98f, 0.8f));
  m_GlobalSceneInfo["GlobalAmbient"].Set(ezVec3(0.38f, 0.38f, 0.4f));
  //  m_GlobalSceneInfo["NumMSAASamples"].Set(static_cast<ezUInt32>(GeneralConfig::g_MSAASamples.GetValue()));

}

// CVar type conversions
template<typename T> struct CVarType {};
template<> struct CVarType<ezCVarFloat> { typedef float type; };
template<> struct CVarType<ezCVarBool> { typedef bool type; };
template<> struct CVarType<ezCVarInt> { typedef int type; };
template<> struct CVarType<ezCVarString> { typedef char* type; };


// Macro for adding a change handler to a arbitrary cvar and add it to the interface
#define CreateCVarInterfaceEntry(cvar, changeHandler) \
  (cvar).m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler([=](const ezCVar::CVarEvent&) { (changeHandler)((cvar).GetValue()); })); \
  m_pUserInterface->AddReadWrite((cvar).GetName(), \
  ezDelegate<ezVariant(void)>([]{ return ezVariant((cvar).GetValue()); }), \
  ezDelegate<void(const ezVariant &)>([](const ezVariant& val){ (cvar) = val.Get<CVarType<decltype(cvar)>::type>(); }), \
  (cvar).GetDescription());

// Macro for adding a stat value from ezStats to the interface
#define CreateStatInterfaceEntry(statname, twDef) \
  m_pUserInterface->AddReadOnly((statname), ezDelegate<const char*()>([](){ return ezStats::GetStat((statname)); }), (twDef));


void Scene::InitConfig()
{
  // Resolution change events
  ezEvent<const ezCVar::CVarEvent&>::Handler onResolutionChange = [=](const ezCVar::CVarEvent&)
  {
    // skip invalid sizes
    if(GeneralConfig::g_ResolutionWidth.GetValue() == 0 || GeneralConfig::g_ResolutionHeight.GetValue() == 0)
      return;

    m_pTerrain->RecreateScreenSizeDependentTextures(GeneralConfig::GetScreenResolution());
    m_pPostProcessing->RecreateScreenSizeDependentTextures(GeneralConfig::GetScreenResolution());
    m_pCamera->ChangeAspectRatio(static_cast<float>(GeneralConfig::g_ResolutionWidth.GetValue()) / GeneralConfig::g_ResolutionHeight.GetValue());
    RecreateScreenBuffers();
  };
  GeneralConfig::g_ResolutionWidth.m_CVarEvents.AddEventHandler(onResolutionChange);
  GeneralConfig::g_ResolutionHeight.m_CVarEvents.AddEventHandler(onResolutionChange);

  // General
  CreateStatInterfaceEntry("Frame time", "group=General");
  CreateStatInterfaceEntry("Frames per second", "group=General");

  // Terrain Rendering
  CreateStatInterfaceEntry("Terrain Draw Time", "group='Terrain Rendering'");
  CreateCVarInterfaceEntry(SceneConfig::TerrainRendering::g_Wireframe, [](bool) {});
  CreateCVarInterfaceEntry(SceneConfig::TerrainRendering::g_PixelPerTriangle, ezDelegate<void(float)>(&Terrain::SetPixelPerTriangle, m_pTerrain));
  CreateCVarInterfaceEntry(SceneConfig::TerrainRendering::g_UseAnisotropicFilter, ezDelegate<void(bool)>(&Terrain::SetAnisotropicFiltering, m_pTerrain));
  CreateCVarInterfaceEntry(SceneConfig::TerrainRendering::g_FresnelReflection, ezDelegate<void(float)>(&Terrain::SetTerrainFresnelReflectionCoef, m_pTerrain));
  CreateCVarInterfaceEntry(SceneConfig::TerrainRendering::g_SpecularPower, ezDelegate<void(float)>(&Terrain::SetTerrainSpecularPower, m_pTerrain));

  // Water Rendering
  CreateStatInterfaceEntry("Water Draw Time", "group='Water Rendering'");
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_wireframe, [](bool) {});

  m_pUserInterface->AddSeperator("Colors", "group='Water Rendering'");

  auto setBigDepthColor = [=](float) { m_pTerrain->SetWaterBigDepthColor(ezVec3(SceneConfig::WaterRendering::g_bigDepthColorR.GetValue(), SceneConfig::WaterRendering::g_bigDepthColorG.GetValue(), SceneConfig::WaterRendering::g_bigDepthColorB.GetValue())); };
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_bigDepthColorR, setBigDepthColor);
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_bigDepthColorG, setBigDepthColor);
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_bigDepthColorB, setBigDepthColor);
  auto setWaterExtinctionCoef = [=](const float) { m_pTerrain->SetWaterExtinctionCoefficients(ezVec3(SceneConfig::WaterRendering::g_extinctionCoefficientsR.GetValue(), SceneConfig::WaterRendering::g_extinctionCoefficientsG.GetValue(), SceneConfig::WaterRendering::g_extinctionCoefficientsB.GetValue())); };
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_extinctionCoefficientsR, setWaterExtinctionCoef);
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_extinctionCoefficientsG, setWaterExtinctionCoef);
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_extinctionCoefficientsB, setWaterExtinctionCoef);
  auto setSurfaceColor = [=](float) { m_pTerrain->SetWaterSurfaceColor(ezVec3(SceneConfig::WaterRendering::g_surfaceColorR.GetValue(), SceneConfig::WaterRendering::g_surfaceColorG.GetValue(), SceneConfig::WaterRendering::g_surfaceColorB.GetValue())); };
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_surfaceColorR, setSurfaceColor);
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_surfaceColorG, setSurfaceColor);
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_surfaceColorB, setSurfaceColor);
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_opaqueness, ezDelegate<void(float)>(&Terrain::SetWaterOpaqueness, m_pTerrain));

  m_pUserInterface->AddSeperator("Flow", "group='Water Rendering'");

  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_normalMapRepeat, ezDelegate<void(float)>(&Terrain::SetWaterNormalMapRepeat, m_pTerrain));
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_speedToNormalDistortion, ezDelegate<void(float)>(&Terrain::SetWaterSpeedToNormalDistortion, m_pTerrain));
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_normalLayerBlendInveral, [&](float seconds) { m_pTerrain->Terrain::SetWaterDistortionLayerBlendInterval(ezTime::Seconds(seconds)); });
  CreateCVarInterfaceEntry(SceneConfig::WaterRendering::g_flowDistortionStrength, ezDelegate<void(float)>(&Terrain::SetWaterFlowDistortionStrength, m_pTerrain));


  // Simulation
  CreateStatInterfaceEntry("Simulation Time", "group='Simulation'");
  CreateCVarInterfaceEntry(SceneConfig::Simulation::g_simulationStepsPerSecond, ezDelegate<void(float)>(&Terrain::SetSimulationStepsPerSecond, m_pTerrain));
  CreateCVarInterfaceEntry(SceneConfig::Simulation::g_flowDamping, ezDelegate<void(float)>(&Terrain::SetFlowDamping, m_pTerrain));
  CreateCVarInterfaceEntry(SceneConfig::Simulation::g_flowAcceleration, ezDelegate<void(float)>(&Terrain::SetFlowAcceleration, m_pTerrain));
  m_pUserInterface->AddButton("Reset Simulation", ezDelegate<void()>([&]() { m_pTerrain->CreateHeightmapFromNoiseAndResetSim(); }), "group='Simulation'");


  // Postprocessing
  CreateCVarInterfaceEntry(SceneConfig::PostPro::g_exposure, ezDelegate<void(float)>(&PostProcessing::SetExposure, m_pPostProcessing));
  CreateCVarInterfaceEntry(SceneConfig::PostPro::g_adaptationSpeed, ezDelegate<void(float)>(&PostProcessing::SetLuminanceAdaptationSpeed, m_pPostProcessing));


  // Trigger all cvar changed since CVar's default values are not necessarily setting's status
  for(ezCVar* pCVarInst = ezCVar::GetFirstInstance(); pCVarInst; pCVarInst = pCVarInst->GetNextInstance())
    pCVarInst->m_CVarEvents.Broadcast(ezCVar::CVarEvent(pCVarInst));
}

void Scene::RecreateScreenBuffers()
{
  m_pLinearHDRBuffer.Swap(EZ_DEFAULT_NEW_UNIQUE(gl::Texture2D, GeneralConfig::g_ResolutionWidth.GetValue(), GeneralConfig::g_ResolutionHeight.GetValue(),
          GL_RGBA16F, 1/*, GeneralConfig::g_MSAASamples.GetValue()*/));
  m_pDepthBuffer.Swap(EZ_DEFAULT_NEW_UNIQUE(gl::Texture2D, GeneralConfig::g_ResolutionWidth.GetValue(), GeneralConfig::g_ResolutionHeight.GetValue(),
          GL_DEPTH_COMPONENT32, 1/*, GeneralConfig::g_MSAASamples.GetValue()*/));
  m_pLinearHDRFramebuffer.Swap(EZ_DEFAULT_NEW_UNIQUE(gl::FramebufferObject, { gl::FramebufferObject::Attachment(m_pLinearHDRBuffer.Get()) }, gl::FramebufferObject::Attachment(m_pDepthBuffer.Get())));
}

Scene::~Scene(void)
{
  EZ_DEFAULT_DELETE(m_pTerrain);
  EZ_DEFAULT_DELETE(m_pBackground);
  EZ_DEFAULT_DELETE(m_pPostProcessing);
}

ezResult Scene::Update(ezTime lastFrameDuration)
{
  m_pCamera->Update(lastFrameDuration);

  m_CameraUBO["ViewMatrix"].Set(m_pCamera->GetViewMatrix());
  m_CameraUBO["ViewProjection"].Set(m_pCamera->GetViewProjectionMatrix());
  ezMat4 inverseViewProjection = m_pCamera->GetViewProjectionMatrix();
  inverseViewProjection.Invert();
  m_CameraUBO["InverseViewProjection"].Set(inverseViewProjection);
  m_CameraUBO["CameraPosition"].Set(m_pCamera->GetPosition());
  
  // update stats vars
  ezStringBuilder statString; statString.Format("%.3f ms", m_pTerrainDrawTimer->GetLastTimeElapsed(true).GetMilliseconds());
  ezStats::SetStat("Terrain Draw Time", statString.GetData());
  statString.Format("%.3f ms", m_pWaterDrawTimer->GetLastTimeElapsed(true).GetMilliseconds());
  ezStats::SetStat("Water Draw Time", statString.GetData());
  statString.Format("%.3f ms", m_pSimulationTimer->GetLastTimeElapsed(true).GetMilliseconds());
  ezStats::SetStat("Simulation Time", statString.GetData());

  // simulate
  m_pSimulationTimer->Start();
  m_pTerrain->PerformSimulationStep(lastFrameDuration);
  m_pSimulationTimer->End();

  // visibility
  m_pTerrain->UpdateVisibilty(m_pCamera->GetPosition());


  //ezAngle angle = ezAngle::Radian(ezSystemTime::Now().GetSeconds() / 4);
  //m_GlobalSceneInfo["GlobalDirLightDirection"].Set(ezVec3((float)ezMath::Sin(angle), ezMath::Abs(ezMath::Cos(angle)), 0.0f).GetNormalized());

  return EZ_SUCCESS;
}

ezResult Scene::Render(ezTime lastFrameDuration)
{ 
  // set global ubos to their global binding points
  m_CameraUBO.BindBuffer(0);
  m_TimeUBO.BindBuffer(1);
  m_GlobalSceneInfo.BindBuffer(2);

  // Update background
  m_pBackground->UpdateCubemap();

  // DepthTest on.
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  // HDR on.
  m_pLinearHDRFramebuffer->Bind(true);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

 
  
  // render terrain
  if(SceneConfig::TerrainRendering::g_Wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  m_pTerrainDrawTimer->Start();
  m_pTerrain->DrawTerrain();
  m_pTerrainDrawTimer->End();
  if(SceneConfig::TerrainRendering::g_Wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
  // render water
  if(SceneConfig::WaterRendering::g_wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  m_pWaterDrawTimer->Start();
  m_pTerrain->DrawWater(*m_pLinearHDRFramebuffer.Get(), m_pBackground->GetSkyboxCubemap());
  m_pWaterDrawTimer->End();
  if(SceneConfig::WaterRendering::g_wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


  // render nice background
  m_pBackground->Draw();

  // Resolve rendertarget to backbuffer
  m_pPostProcessing->ApplyAndRenderToBackBuffer(lastFrameDuration, *m_pLinearHDRFramebuffer.Get());



  // Render ui directly to backbuffer
  RenderUI();

  return EZ_SUCCESS;
}

void Scene::RenderUI()
{
  glBindSampler(0, 0);
  m_pUserInterface->Render();
}