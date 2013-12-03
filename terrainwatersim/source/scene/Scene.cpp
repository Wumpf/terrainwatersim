#include "PCH.h"
#include "Scene.h"
#include "config/GlobalCVar.h"

#include "RenderWindow.h"

#include "gl/ScreenAlignedTriangle.h"
#include "gl/Font.h"
#include "gl/TimerQuery.h"
#include "gl/GLUtils.h"

#include "math/camera/FreeCamera.h"

#include "Terrain.h"
#include "Background.h"

#include "AntTweakBarInterface.h"
#include <Foundation/Utilities/Stats.h>

#include "gl/resources/FramebufferObject.h"
#include "gl/resources/textures/Texture2D.h"

namespace SceneConfig
{
  namespace TerrainRendering
  {
    ezCVarBool g_Wireframe("Wireframe Terrain", false, ezCVarFlags::Save, "group=\'Terrain Rendering\'");
    ezCVarFloat g_PixelPerTriangle("Aimed Pixel/Triangle", 25.0f, ezCVarFlags::Save, "group=\'Terrain Rendering\' min=3.0 max=200");
    ezCVarBool g_UseAnisotropicFilter("Anisotropic Filter on/off", true, ezCVarFlags::Save, "group=\'Terrain Rendering\'");
    ezCVarFloat g_FresnelReflection("Fresnel Reflection Coef", 0.1f, ezCVarFlags::Save, "group=\'Terrain Rendering\' min=0.0 max=2.0 step = 0.01");
    ezCVarFloat g_SpecularPower("Specular Power", 4.0f, ezCVarFlags::Save, "group=\'Terrain Rendering\' min=0.0 max=32.0 step = 0.5");
  }
  namespace WaterRendering
  {
    ezCVarBool g_wireframe("Wireframe Water", false, ezCVarFlags::Save, "group=\'Water Rendering\'");
    CVarRGBImpl(g_surfaceColor, "Surface Color", ezVec3(0.0029f, 0.1788f, 0.27f), ezCVarFlags::Save, "group=\'Water Rendering\' min=0.0 max=1.0 step=0.005");
    CVarRGBImpl(g_bigDepthColor, "Big-Depth Color", ezVec3(0.00195f, 0.00098f, 0.0725f), ezCVarFlags::Save, "group=\'Water Rendering\' min=0.0 max=1.0 step=0.005");
    CVarRGBImpl(g_extinctionCoefficients, "Extinction Coefficients", ezVec3(0.1278f, 0.0735f, 0.5f), ezCVarFlags::Save, "group=\'Water Rendering\' min=0.0 max=1.0 step=0.0025");
    ezCVarFloat g_opaqueness("Opaqueness", 0.01f, ezCVarFlags::Save, "group=\'Water Rendering\' min=0.0 max=0.5 step=0.005");

    ezCVarFloat g_normalMapRepeat("Normalmap repeat", 30.0f, ezCVarFlags::Save, "group=\'Water Rendering\' min=5.0 max=100.0 step=1");
    ezCVarFloat g_speedToNormalDistortion("Flow to normal Distortion", 0.01f, ezCVarFlags::Save, "group=\'Water Rendering\' min=0.0 max=0.3 step=0.001");
    ezCVarFloat g_normalLayerBlendInveral("Normal layer blend interval", 0.01f, ezCVarFlags::Save, "group=\'Water Rendering\' min=0.5 max=50.0 step=0.25");
    ezCVarFloat g_flowDistortionStrength("Flow distortion strength", 0.001f, ezCVarFlags::Save, "group=\'Water Rendering\' min=0.0 max=0.1 step=0.0005");
  }

  namespace Simulation
  {
    ezCVarFloat g_SimulationStepsPerSecond("Simulation steps per second", 60, ezCVarFlags::Save, "group='Simulation' min=30 max=300");
    ezCVarFloat g_FlowDamping("Flow Damping", 0.98f, ezCVarFlags::Save, "group=Simulation min=0.2 max=1.0 step=0.01");
    ezCVarFloat g_FlowAcceleration("Flow Acceleration", 10.0f, ezCVarFlags::Save, "group=Simulation min=0.5 max=100.0 step=0.1");
  }
}

Scene::Scene(const RenderWindowGL& renderWindow) :
  m_pCopyShader(EZ_DEFAULT_NEW_UNIQUE(gl::ShaderObject, "copySceneShader")),

  m_pCamera(EZ_DEFAULT_NEW_UNIQUE(FreeCamera, ezAngle::Degree(70.0f), static_cast<float>(GeneralConfig::g_ResolutionWidth.GetValue()) / GeneralConfig::g_ResolutionHeight.GetValue())),
  m_pFont(EZ_DEFAULT_NEW_UNIQUE(gl::Font, "Arial", 20, renderWindow.GetDeviceContext())),

  m_pWaterDrawTimer(EZ_DEFAULT_NEW_UNIQUE(gl::TimerQuery)),
  m_pTerrainDrawTimer(EZ_DEFAULT_NEW_UNIQUE(gl::TimerQuery)),
  m_pSimulationTimer(EZ_DEFAULT_NEW_UNIQUE(gl::TimerQuery)),

  m_UserInterface(EZ_DEFAULT_NEW_UNIQUE(AntTweakBarInterface))
{
  EZ_LOG_BLOCK("Scene init");

  m_pTerrain = EZ_DEFAULT_NEW(Terrain)(GeneralConfig::GetScreenResolution());
  m_pBackground = EZ_DEFAULT_NEW(Background)(256);

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
  m_UserInterface->Init();
  InitConfig();

}

void Scene::InitGlobalUBO()
{
  std::initializer_list<const gl::ShaderObject*> shaderList =
  { 
    &m_pBackground->GetBackgroundShader(), &m_pBackground->GetScatteringShader(), 
    &m_pTerrain->GetTerrainShader(), &m_pTerrain->GetWaterShader()
  };
  m_CameraUBO.Init(shaderList, "Camera");
  m_GlobalSceneInfo.Init(shaderList, "GlobalSceneInfo");

  /*    ezDynamicArray<const gl::ShaderObject*> timeUBOusingShader;
  cameraUBOusingShader.PushBack(&m_DirectVolVisShader);
  m_TimeUBO.Init(cameraUBOusingShader, "Time");
  */

  m_GlobalSceneInfo["GlobalDirLightDirection"].Set(ezVec3(1.5f, 1.0f, 1.5f).GetNormalized());
  m_GlobalSceneInfo["GlobalDirLightColor"].Set(ezVec3(0.98f, 0.98f, 0.8f));
  m_GlobalSceneInfo["GlobalAmbient"].Set(ezVec3(0.38f, 0.38f, 0.4f));
  //  m_GlobalSceneInfo["NumMSAASamples"].Set(static_cast<ezUInt32>(GeneralConfig::g_MSAASamples.GetValue()));

}

void Scene::InitConfig()
{
#define CONFIG_EVENTHANDLER_LAMBDA(CODE) ezEvent<const ezCVar::CVarEvent&>::Handler([=](const ezCVar::CVarEvent&) { CODE } )

  // Callbacks for CVars
  ezEvent<const ezCVar::CVarEvent&>::Handler onResolutionChange = [=](const ezCVar::CVarEvent&)
  {
    // skip invalid sizes
    if(GeneralConfig::g_ResolutionWidth.GetValue() == 0 || GeneralConfig::g_ResolutionHeight.GetValue() == 0)
      return;

    m_pTerrain->RecreateScreenSizeDependentTextures(GeneralConfig::GetScreenResolution());
    m_pCamera->ChangeAspectRatio(static_cast<float>(GeneralConfig::g_ResolutionWidth.GetValue()) / GeneralConfig::g_ResolutionHeight.GetValue());
    RecreateScreenBuffers();
  };
  GeneralConfig::g_ResolutionWidth.m_CVarEvents.AddEventHandler(onResolutionChange);
  GeneralConfig::g_ResolutionHeight.m_CVarEvents.AddEventHandler(onResolutionChange);
  /*GeneralConfig::g_MSAASamples.m_CVarEvents.AddEventHandler([=](const ezCVar::CVarEvent&){
    RecreateScreenBuffers();
    m_GlobalSceneInfo["NumMSAASamples"].Set(static_cast<ezUInt32>(GeneralConfig::g_MSAASamples.GetValue()));
  });*/

  SceneConfig::TerrainRendering::g_PixelPerTriangle.m_CVarEvents.AddEventHandler(CONFIG_EVENTHANDLER_LAMBDA(m_pTerrain->SetPixelPerTriangle(SceneConfig::TerrainRendering::g_PixelPerTriangle.GetValue());));
  SceneConfig::TerrainRendering::g_UseAnisotropicFilter.m_CVarEvents.AddEventHandler(CONFIG_EVENTHANDLER_LAMBDA(m_pTerrain->SetAnisotrpicFiltering(SceneConfig::TerrainRendering::g_UseAnisotropicFilter.GetValue());));
  SceneConfig::TerrainRendering::g_FresnelReflection.m_CVarEvents.AddEventHandler(CONFIG_EVENTHANDLER_LAMBDA(m_pTerrain->SetTerrainFresnelReflectionCoef(SceneConfig::TerrainRendering::g_FresnelReflection.GetValue());));
  SceneConfig::TerrainRendering::g_SpecularPower.m_CVarEvents.AddEventHandler(CONFIG_EVENTHANDLER_LAMBDA(m_pTerrain->SetTerrainSpecularPower(SceneConfig::TerrainRendering::g_SpecularPower.GetValue());));

  auto setBigDepthColor = [=](const ezCVar::CVarEvent&) { m_pTerrain->SetWaterBigDepthColor(ezVec3(SceneConfig::WaterRendering::g_bigDepthColorR.GetValue(), SceneConfig::WaterRendering::g_bigDepthColorG.GetValue(), SceneConfig::WaterRendering::g_bigDepthColorB.GetValue())); };
  SceneConfig::WaterRendering::g_bigDepthColorR.m_CVarEvents.AddEventHandler(setBigDepthColor);
  SceneConfig::WaterRendering::g_bigDepthColorG.m_CVarEvents.AddEventHandler(setBigDepthColor);
  SceneConfig::WaterRendering::g_bigDepthColorB.m_CVarEvents.AddEventHandler(setBigDepthColor);

  auto setSurfaceColor = [=](const ezCVar::CVarEvent&) { m_pTerrain->SetWaterSurfaceColor(ezVec3(SceneConfig::WaterRendering::g_surfaceColorR.GetValue(), SceneConfig::WaterRendering::g_surfaceColorG.GetValue(), SceneConfig::WaterRendering::g_surfaceColorB.GetValue())); };
  SceneConfig::WaterRendering::g_surfaceColorR.m_CVarEvents.AddEventHandler(setSurfaceColor);
  SceneConfig::WaterRendering::g_surfaceColorG.m_CVarEvents.AddEventHandler(setSurfaceColor);
  SceneConfig::WaterRendering::g_surfaceColorB.m_CVarEvents.AddEventHandler(setSurfaceColor);

  auto setWaterExtinctionCoef = [=](const ezCVar::CVarEvent&) { m_pTerrain->SetWaterExtinctionCoefficients(ezVec3(SceneConfig::WaterRendering::g_extinctionCoefficientsR.GetValue(), SceneConfig::WaterRendering::g_extinctionCoefficientsG.GetValue(), SceneConfig::WaterRendering::g_extinctionCoefficientsB.GetValue())); };
  SceneConfig::WaterRendering::g_extinctionCoefficientsR.m_CVarEvents.AddEventHandler(setWaterExtinctionCoef);
  SceneConfig::WaterRendering::g_extinctionCoefficientsG.m_CVarEvents.AddEventHandler(setWaterExtinctionCoef);
  SceneConfig::WaterRendering::g_extinctionCoefficientsB.m_CVarEvents.AddEventHandler(setWaterExtinctionCoef);


  SceneConfig::WaterRendering::g_opaqueness.m_CVarEvents.AddEventHandler(CONFIG_EVENTHANDLER_LAMBDA(m_pTerrain->SetWaterOpaqueness(SceneConfig::WaterRendering::g_opaqueness.GetValue());));

  SceneConfig::WaterRendering::g_normalMapRepeat.m_CVarEvents.AddEventHandler(CONFIG_EVENTHANDLER_LAMBDA(m_pTerrain->SetWaterNormalMapRepeat(SceneConfig::WaterRendering::g_normalMapRepeat.GetValue());));
  SceneConfig::WaterRendering::g_speedToNormalDistortion.m_CVarEvents.AddEventHandler(CONFIG_EVENTHANDLER_LAMBDA(m_pTerrain->SetWaterSpeedToNormalDistortion(SceneConfig::WaterRendering::g_speedToNormalDistortion.GetValue());));
  SceneConfig::WaterRendering::g_normalLayerBlendInveral.m_CVarEvents.AddEventHandler(CONFIG_EVENTHANDLER_LAMBDA(m_pTerrain->SetWaterDistortionLayerBlendInterval(ezTime::Seconds(SceneConfig::WaterRendering::g_normalLayerBlendInveral.GetValue()));));
  SceneConfig::WaterRendering::g_flowDistortionStrength.m_CVarEvents.AddEventHandler(CONFIG_EVENTHANDLER_LAMBDA(m_pTerrain->SetWaterFlowDistortionStrength(SceneConfig::WaterRendering::g_flowDistortionStrength.GetValue());));


    // Simulation
  SceneConfig::Simulation::g_SimulationStepsPerSecond.m_CVarEvents.AddEventHandler(CONFIG_EVENTHANDLER_LAMBDA(m_pTerrain->SetSimulationStepsPerSecond(SceneConfig::Simulation::g_SimulationStepsPerSecond.GetValue());));
  SceneConfig::Simulation::g_FlowDamping.m_CVarEvents.AddEventHandler(CONFIG_EVENTHANDLER_LAMBDA(m_pTerrain->SetFlowDamping(SceneConfig::Simulation::g_FlowDamping.GetValue());));

  SceneConfig::Simulation::g_FlowAcceleration.m_CVarEvents.AddEventHandler(CONFIG_EVENTHANDLER_LAMBDA(m_pTerrain->SetFlowAcceleration(SceneConfig::Simulation::g_FlowAcceleration.GetValue());));


  // Trigger initial values that may be saved
  m_pTerrain->SetPixelPerTriangle(SceneConfig::TerrainRendering::g_PixelPerTriangle.GetValue());
  m_pTerrain->SetAnisotrpicFiltering(SceneConfig::TerrainRendering::g_UseAnisotropicFilter.GetValue());
  m_pTerrain->SetTerrainSpecularPower(SceneConfig::TerrainRendering::g_SpecularPower.GetValue());
  m_pTerrain->SetTerrainFresnelReflectionCoef(SceneConfig::TerrainRendering::g_FresnelReflection.GetValue());

  setBigDepthColor(ezCVar::CVarEvent(&SceneConfig::WaterRendering::g_bigDepthColorR));
  setSurfaceColor(ezCVar::CVarEvent(&SceneConfig::WaterRendering::g_surfaceColorR));
  setWaterExtinctionCoef(ezCVar::CVarEvent(&SceneConfig::WaterRendering::g_extinctionCoefficientsR));
  m_pTerrain->SetWaterOpaqueness(SceneConfig::WaterRendering::g_opaqueness.GetValue());

  m_pTerrain->SetWaterNormalMapRepeat(SceneConfig::WaterRendering::g_normalMapRepeat.GetValue());
  m_pTerrain->SetWaterSpeedToNormalDistortion(SceneConfig::WaterRendering::g_speedToNormalDistortion.GetValue());
  m_pTerrain->SetWaterDistortionLayerBlendInterval(ezTime::Seconds(SceneConfig::WaterRendering::g_normalLayerBlendInveral.GetValue()));
  m_pTerrain->SetWaterFlowDistortionStrength(SceneConfig::WaterRendering::g_flowDistortionStrength.GetValue());

  m_pTerrain->SetSimulationStepsPerSecond(SceneConfig::Simulation::g_SimulationStepsPerSecond.GetValue());
  m_pTerrain->SetFlowDamping(SceneConfig::Simulation::g_FlowDamping.GetValue());
  m_pTerrain->SetFlowAcceleration(SceneConfig::Simulation::g_FlowAcceleration.GetValue());
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

  return EZ_SUCCESS;
}

ezResult Scene::Render(ezTime lastFrameDuration)
{ 

  // Update background
  m_pBackground->UpdateCubemap();

  // set global ubos to their global binding points
  m_CameraUBO.BindBuffer(0);
  m_TimeUBO.BindBuffer(1);
  m_GlobalSceneInfo.BindBuffer(2);

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
  glEnable(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_DEPTH_TEST); // no more depth needed
  glDepthMask(GL_FALSE);

  // blit is slow according to this http://stackoverflow.com/questions/9209936/copy-texture-to-screen-buffer-without-drawing-quad-opengl
  // But it turned out, that it is still the standard way to resolve textures.. so since the shader is already there, let's choose the best
  if(m_pLinearHDRBuffer->GetNumMSAASamples() > 0)
  {
    ezRectU32 screenRect(0, 0, m_pLinearHDRBuffer->GetWidth(), m_pLinearHDRBuffer->GetHeight());
    m_pLinearHDRFramebuffer->BlitTo(NULL, screenRect, screenRect);
  }
  else
  {
    gl::FramebufferObject::BindBackBuffer();
    m_pLinearHDRBuffer->Bind(0);
    m_pCopyShader->Activate();
    gl::ScreenAlignedTriangle::Draw();
  }



  // Render ui directly to backbuffer
  gl::FramebufferObject::BindBackBuffer();
  glViewport(0, 0, GeneralConfig::g_ResolutionWidth.GetValue(), GeneralConfig::g_ResolutionHeight.GetValue());
//  gl::ShaderObject::ResetShaderBinding();
  glDisable(GL_FRAMEBUFFER_SRGB);
  RenderUI();

  return EZ_SUCCESS;
}

void Scene::RenderUI()
{
  m_UserInterface->Render();
}