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
    ezCVarBool g_Wireframe("Wireframe", false, ezCVarFlags::Save, "group=\'Terrain Rendering\'");
    ezCVarFloat g_PixelPerTriangle("Aimed Pixel/Triangle", 25.0f, ezCVarFlags::Save, "group=\'Terrain Rendering\' min=3.0 max=200");

    ezCVarBool g_UseAnisotropicFilter("Anisotropic Filter on/off", true, ezCVarFlags::Save, "group=\'Terrain Rendering\'");
  }
  namespace WaterRendering
  {
    CVarRGBImpl(g_waterSurfaceColor, "Surface Color", ezVec3(0.0078f, 0.5176f, 0.7f) * 0.5f, ezCVarFlags::Save, "group=\'Water Rendering\' min=0.0 max=1.0 step=0.01");
    CVarRGBImpl(g_waterBigDepthColor, "Big-Depth Color", ezVec3(0.0039f, 0.00196f, 0.145f)*0.5f, ezCVarFlags::Save, "group=\'Water Rendering\' min=0.0 max=1.0 step=0.01");
    CVarRGBImpl(g_waterExtinctionCoefficients, "Extinction Coefficients", ezVec3(0.478f, 0.435f, 0.5f) * 0.1f, ezCVarFlags::Save, "group=\'Water Rendering\' min=0.0 max=1.0 step=0.01");
    ezCVarFloat g_waterOpaqueness("Opaqueness", 0.01f, ezCVarFlags::Save, "group=\'Water Rendering\' min=0.0 max=0.5 step=0.005");
  }

  namespace Simulation
  {
    ezCVarFloat g_SimulationStepsPerSecond("Simulation steps per second", 60, ezCVarFlags::Save, "group='Simulation' min=30 max=300");
    ezCVarFloat g_FlowDamping("Flow Damping", 0.98f, ezCVarFlags::Save, "group=Simulation min=0.2 max=1.0 step=0.01");
    ezCVarFloat g_FlowAcceleration("Flow Acceleration", 10.0f, ezCVarFlags::Save, "group=Simulation min=0.5 max=100.0 step=0.1");
  }
}

Scene::Scene(const RenderWindowGL& renderWindow) :
  m_pCopyShader(EZ_DEFAULT_NEW_UNIQUE(gl::ShaderObject)),

  m_pCamera(EZ_DEFAULT_NEW_UNIQUE(FreeCamera, ezAngle::Degree(70.0f), static_cast<float>(GeneralConfig::g_ResolutionWidth.GetValue()) / GeneralConfig::g_ResolutionHeight.GetValue())),
  m_pFont(EZ_DEFAULT_NEW_UNIQUE(gl::Font, "Arial", 20, renderWindow.GetDeviceContext())),

  m_ExtractGeometryTimer(EZ_DEFAULT_NEW_UNIQUE(gl::TimerQuery)),
  m_DrawTimer(EZ_DEFAULT_NEW_UNIQUE(gl::TimerQuery)),

  m_UserInterface(EZ_DEFAULT_NEW_UNIQUE(AntTweakBarInterface))
{
  EZ_LOG_BLOCK("Scene init");

  m_pTerrain = EZ_DEFAULT_NEW(Terrain)(GeneralConfig::GetScreenResolution());
  m_pBackground = EZ_DEFAULT_NEW(Background)();

  // global ubo inits
  m_CameraUBO.Init({ &m_pBackground->GetShader(), &m_pTerrain->GetTerrainShader() }, "Camera");
  m_GlobalSceneInfo.Init({ &m_pTerrain->GetTerrainShader(), }, "GlobalSceneInfo");
  
/*    ezDynamicArray<const gl::ShaderObject*> timeUBOusingShader;
  cameraUBOusingShader.PushBack(&m_DirectVolVisShader);
  m_TimeUBO.Init(cameraUBOusingShader, "Time");
*/
  
  m_GlobalSceneInfo["GlobalDirLightDirection"].Set(ezVec3(1.5f, 1.0f, 1.5f).GetNormalized());
  m_GlobalSceneInfo["GlobalDirLightColor"].Set(ezVec3(0.98f, 0.98f, 0.8f));
  m_GlobalSceneInfo["GlobalAmbient"].Set(ezVec3(0.38f, 0.38f, 0.4f));
//  m_GlobalSceneInfo["NumMSAASamples"].Set(static_cast<ezUInt32>(GeneralConfig::g_MSAASamples.GetValue()));
  
  ezVec3 vCameraPos(m_pTerrain->GetTerrainWorldSize() / 2, 100, m_pTerrain->GetTerrainWorldSize() / 2);
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

void Scene::InitConfig()
{
  // Callbacks for CVars
  ezEvent<const ezCVar::CVarEvent&>::Handler onResolutionChange = [=](const ezCVar::CVarEvent&)
  {
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

  SceneConfig::TerrainRendering::g_PixelPerTriangle.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetPixelPerTriangle(SceneConfig::TerrainRendering::g_PixelPerTriangle.GetValue()); }));
  SceneConfig::TerrainRendering::g_UseAnisotropicFilter.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetAnisotrpicFiltering(SceneConfig::TerrainRendering::g_UseAnisotropicFilter.GetValue()); }));

  auto setWaterBigDepthColor = [=](const ezCVar::CVarEvent&) { m_pTerrain->SetWaterBigDepthColor(ezVec3(SceneConfig::WaterRendering::g_waterBigDepthColorR.GetValue(), SceneConfig::WaterRendering::g_waterBigDepthColorG.GetValue(), SceneConfig::WaterRendering::g_waterBigDepthColorB.GetValue())); };
  SceneConfig::WaterRendering::g_waterBigDepthColorR.m_CVarEvents.AddEventHandler(setWaterBigDepthColor);
  SceneConfig::WaterRendering::g_waterBigDepthColorG.m_CVarEvents.AddEventHandler(setWaterBigDepthColor);
  SceneConfig::WaterRendering::g_waterBigDepthColorB.m_CVarEvents.AddEventHandler(setWaterBigDepthColor);

  auto setWaterSurfaceColor = [=](const ezCVar::CVarEvent&) { m_pTerrain->SetWaterSurfaceColor(ezVec3(SceneConfig::WaterRendering::g_waterSurfaceColorR.GetValue(), SceneConfig::WaterRendering::g_waterSurfaceColorG.GetValue(), SceneConfig::WaterRendering::g_waterSurfaceColorB.GetValue())); };
  SceneConfig::WaterRendering::g_waterSurfaceColorR.m_CVarEvents.AddEventHandler(setWaterSurfaceColor);
  SceneConfig::WaterRendering::g_waterSurfaceColorG.m_CVarEvents.AddEventHandler(setWaterSurfaceColor);
  SceneConfig::WaterRendering::g_waterSurfaceColorB.m_CVarEvents.AddEventHandler(setWaterSurfaceColor);

  auto setWaterExtinctionCoef = [=](const ezCVar::CVarEvent&) { m_pTerrain->SetWaterExtinctionCoefficients(ezVec3(SceneConfig::WaterRendering::g_waterExtinctionCoefficientsR.GetValue(), SceneConfig::WaterRendering::g_waterExtinctionCoefficientsG.GetValue(), SceneConfig::WaterRendering::g_waterExtinctionCoefficientsB.GetValue())); };
  SceneConfig::WaterRendering::g_waterExtinctionCoefficientsR.m_CVarEvents.AddEventHandler(setWaterExtinctionCoef);
  SceneConfig::WaterRendering::g_waterExtinctionCoefficientsG.m_CVarEvents.AddEventHandler(setWaterExtinctionCoef);
  SceneConfig::WaterRendering::g_waterExtinctionCoefficientsB.m_CVarEvents.AddEventHandler(setWaterExtinctionCoef);

  SceneConfig::WaterRendering::g_waterOpaqueness.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetWaterOpaqueness(SceneConfig::WaterRendering::g_waterOpaqueness.GetValue()); }));

  SceneConfig::Simulation::g_SimulationStepsPerSecond.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetSimulationStepsPerSecond(SceneConfig::Simulation::g_SimulationStepsPerSecond.GetValue()); }));
  SceneConfig::Simulation::g_FlowDamping.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetFlowDamping(SceneConfig::Simulation::g_FlowDamping.GetValue()); }));
  SceneConfig::Simulation::g_FlowAcceleration.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetFlowAcceleration(SceneConfig::Simulation::g_FlowAcceleration.GetValue()); }));


  // Trigger initial values that may be saved
  m_pTerrain->SetPixelPerTriangle(SceneConfig::TerrainRendering::g_PixelPerTriangle.GetValue());
  m_pTerrain->SetAnisotrpicFiltering(SceneConfig::TerrainRendering::g_UseAnisotropicFilter.GetValue());

  setWaterBigDepthColor(ezCVar::CVarEvent(&SceneConfig::WaterRendering::g_waterBigDepthColorR));
  setWaterSurfaceColor(ezCVar::CVarEvent(&SceneConfig::WaterRendering::g_waterSurfaceColorR));
  setWaterExtinctionCoef(ezCVar::CVarEvent(&SceneConfig::WaterRendering::g_waterExtinctionCoefficientsR));
  m_pTerrain->SetWaterOpaqueness(SceneConfig::WaterRendering::g_waterOpaqueness.GetValue());

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
  ezStringBuilder statString; statString.Format("%.2f ms", m_DrawTimer->GetLastTimeElapsed().GetMilliseconds());
  ezStats::SetStat(
    "Terrain Draw Time", statString.GetData());

  // simulate
  m_pTerrain->PerformSimulationStep(lastFrameDuration);

  // visibility
  m_pTerrain->UpdateVisibilty(m_pCamera->GetPosition());

  return EZ_SUCCESS;
}

ezResult Scene::Render(ezTime lastFrameDuration)
{ 
  // set global ubos to their global binding points
  m_CameraUBO.BindBuffer(0);
  m_TimeUBO.BindBuffer(1);
  m_GlobalSceneInfo.BindBuffer(2);

  m_ExtractGeometryTimer->Start();
  m_ExtractGeometryTimer->End();
  
  // DepthTest on.
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  // HDR on.
  m_pLinearHDRFramebuffer->Bind();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // render terrain
  m_DrawTimer->Start();
  if(SceneConfig::TerrainRendering::g_Wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  
  m_pTerrain->DrawTerrain();
  m_pTerrain->DrawWater(*m_pLinearHDRFramebuffer.Get());
   
  m_DrawTimer->End();
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
//  gl::ShaderObject::ResetShaderBinding();
  glDisable(GL_FRAMEBUFFER_SRGB);
  RenderUI();

  return EZ_SUCCESS;
}

void Scene::RenderUI()
{
  m_UserInterface->Render();
}