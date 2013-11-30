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
  namespace Simulation
  {
    ezCVarFloat g_SimulationStepsPerSecond("Simulation steps per second", 60, ezCVarFlags::Save, "group='Simulation' min=30 max=300");
    ezCVarFloat g_FlowDamping("Flow Damping", 0.98f, ezCVarFlags::Save, "group=Simulation min=0.2 max=1.0 step=0.01");
    ezCVarFloat g_FlowAcceleration("Flow Acceleration", 10.0f, ezCVarFlags::Save, "group=Simulation min=0.5 max=100.0 step=0.1");
  }
}

Scene::Scene(const RenderWindowGL& renderWindow) :
  m_pScreenAlignedTriangle(std::make_shared<gl::ScreenAlignedTriangle>()),

  m_pCopyShader(EZ_DEFAULT_NEW_UNIQUE(gl::ShaderObject)),

  m_pCamera(EZ_DEFAULT_NEW_UNIQUE(FreeCamera, ezAngle::Degree(70.0f), static_cast<float>(GeneralConfig::g_ResolutionWidth.GetValue()) / GeneralConfig::g_ResolutionHeight.GetValue())),
  m_pFont(EZ_DEFAULT_NEW_UNIQUE(gl::Font, "Arial", 20, renderWindow.GetDeviceContext())),

  m_ExtractGeometryTimer(EZ_DEFAULT_NEW_UNIQUE(gl::TimerQuery)),
  m_DrawTimer(EZ_DEFAULT_NEW_UNIQUE(gl::TimerQuery)),

  m_UserInterface(EZ_DEFAULT_NEW_UNIQUE(AntTweakBarInterface))
{
  EZ_LOG_BLOCK("Scene init");

  m_pTerrain = EZ_DEFAULT_NEW(Terrain)();
  m_pBackground = EZ_DEFAULT_NEW(Background)(m_pScreenAlignedTriangle);

  // global ubo inits
  m_CameraUBO.Init({ &m_pBackground->GetShader(), &m_pTerrain->GetTerrainShader() }, "Camera");
  m_GlobalSceneInfo.Init({ &m_pTerrain->GetTerrainShader() }, "GlobalSceneInfo");
  
/*    ezDynamicArray<const gl::ShaderObject*> timeUBOusingShader;
  cameraUBOusingShader.PushBack(&m_DirectVolVisShader);
  m_TimeUBO.Init(cameraUBOusingShader, "Time");
*/
  
  m_GlobalSceneInfo["GlobalDirLightDirection"].Set(ezVec3(1.5f, 1.0f, 1.5f).GetNormalized());
  m_GlobalSceneInfo["GlobalDirLightColor"].Set(ezVec3(0.98f, 0.98f, 0.8f));
  m_GlobalSceneInfo["GlobalAmbient"].Set(ezVec3(0.38f, 0.38f, 0.4f));
  
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
    m_pCamera->ChangeAspectRatio(static_cast<float>(GeneralConfig::g_ResolutionWidth.GetValue()) / GeneralConfig::g_ResolutionHeight.GetValue());
    RecreateScreenBuffers();
  };
  GeneralConfig::g_ResolutionWidth.m_CVarEvents.AddEventHandler(onResolutionChange);
  GeneralConfig::g_ResolutionHeight.m_CVarEvents.AddEventHandler(onResolutionChange);

  SceneConfig::TerrainRendering::g_PixelPerTriangle.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetPixelPerTriangle(SceneConfig::TerrainRendering::g_PixelPerTriangle.GetValue()); }));
  SceneConfig::TerrainRendering::g_UseAnisotropicFilter.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetAnisotrpicFiltering(SceneConfig::TerrainRendering::g_UseAnisotropicFilter.GetValue()); }));

  SceneConfig::Simulation::g_SimulationStepsPerSecond.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetSimulationStepsPerSecond(SceneConfig::Simulation::g_SimulationStepsPerSecond.GetValue()); }));
  SceneConfig::Simulation::g_FlowDamping.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetFlowDamping(SceneConfig::Simulation::g_FlowDamping.GetValue()); }));
  SceneConfig::Simulation::g_FlowAcceleration.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetFlowAcceleration(SceneConfig::Simulation::g_FlowAcceleration.GetValue()); }));


  // Trigger initial values that may be saved
  m_pTerrain->SetPixelPerTriangle(SceneConfig::TerrainRendering::g_PixelPerTriangle.GetValue());
  m_pTerrain->SetAnisotrpicFiltering(SceneConfig::TerrainRendering::g_UseAnisotropicFilter.GetValue());

  m_pTerrain->SetSimulationStepsPerSecond(SceneConfig::Simulation::g_SimulationStepsPerSecond.GetValue());
  m_pTerrain->SetFlowDamping(SceneConfig::Simulation::g_FlowDamping.GetValue());
  m_pTerrain->SetFlowAcceleration(SceneConfig::Simulation::g_FlowAcceleration.GetValue());
}

void Scene::RecreateScreenBuffers()
{
  m_pLinearHDRBuffer.Swap(EZ_DEFAULT_NEW_UNIQUE(gl::Texture2D, GeneralConfig::g_ResolutionWidth.GetValue(), GeneralConfig::g_ResolutionHeight.GetValue(), GL_RGBA16F, 1));
  m_pDepthBuffer.Swap(EZ_DEFAULT_NEW_UNIQUE(gl::Texture2D, GeneralConfig::g_ResolutionWidth.GetValue(), GeneralConfig::g_ResolutionHeight.GetValue(), GL_DEPTH_COMPONENT32, 1));
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
  
  m_pTerrain->Draw(m_pCamera->GetPosition() );
   
  m_DrawTimer->End();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


  // render nice background
  m_pBackground->Draw();



  // Resolve rendertarget to backbuffer (blit is slow according to this http://stackoverflow.com/questions/9209936/copy-texture-to-screen-buffer-without-drawing-quad-opengl)
  glEnable(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_DEPTH_TEST); // no more depth needed
  glDepthMask(GL_FALSE);
  gl::FramebufferObject::BindBackBuffer();
  m_pLinearHDRBuffer->Bind(0);
  m_pCopyShader->Activate();
  m_pScreenAlignedTriangle->Draw();



  // Render ui directly to backbuffer
  glDisable(GL_FRAMEBUFFER_SRGB);
  RenderUI();

  return EZ_SUCCESS;
}

void Scene::RenderUI()
{
  m_UserInterface->Render();
}