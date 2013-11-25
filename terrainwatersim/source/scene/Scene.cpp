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

namespace SceneConfig
{
  namespace TerrainRendering
  {
    ezCVarBool g_Wireframe("Wireframe", false, ezCVarFlags::Save, "group=TerrainRendering");
    ezCVarFloat g_PixelPerTriangle("Aimed Pixel/Triangle", 25.0f, ezCVarFlags::Save, "group=TerrainRendering min=3.0 max=200");
      
    ezCVarBool g_UseAnisotropicFilter("Anisotropic Filter on/off", true, ezCVarFlags::Save, "group=TerrainRendering");
  }
}

Scene::Scene(const RenderWindowGL& renderWindow) :
m_pScreenAlignedTriangle(std::make_shared<gl::ScreenAlignedTriangle>()),

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

  // user interface
  m_UserInterface->Init();

  // Callbacks for CVars
  SceneConfig::TerrainRendering::g_PixelPerTriangle.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetPixelPerTriangle(SceneConfig::TerrainRendering::g_PixelPerTriangle.GetValue()); }));
  SceneConfig::TerrainRendering::g_UseAnisotropicFilter.m_CVarEvents.AddEventHandler(ezEvent<const ezCVar::CVarEvent&>::Handler(
    [=](const ezCVar::CVarEvent&) { m_pTerrain->SetAnisotrpicFiltering(SceneConfig::TerrainRendering::g_UseAnisotropicFilter.GetValue()); }));

  // Trigger initial values
  m_pTerrain->SetPixelPerTriangle(SceneConfig::TerrainRendering::g_PixelPerTriangle.GetValue());
  m_pTerrain->SetAnisotrpicFiltering(SceneConfig::TerrainRendering::g_UseAnisotropicFilter.GetValue());
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
  ezStringBuilder statString; statString.Format("%.2f ms", m_DrawTimer->GetLastTimeElapsed().GetMilliSeconds());
  ezStats::SetStat("Terrain Draw Time", statString.GetData());

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

  // Gamma correct & DepthTest
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_FRAMEBUFFER_SRGB);

  // render processed data
  m_DrawTimer->Start();
  if(SceneConfig::TerrainRendering::g_Wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  
  m_pTerrain->Draw(m_pCamera->GetPosition() );
   
  m_DrawTimer->End();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


  // render nice background
  m_pBackground->Draw();


  // disable depth test, give up gamma correctness
  glDisable(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  // and ui
  RenderUI();

  return EZ_SUCCESS;
}

void Scene::RenderUI()
{
  m_UserInterface->Render();
}