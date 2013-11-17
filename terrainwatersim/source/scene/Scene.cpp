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
  ezCVarBool g_Wireframe("Wireframe", false, ezCVarFlags::Save, "group=TerrainRendering");
}

GLuint vertexArray;

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

  // global ubo init
  ezDynamicArray<const gl::ShaderObject*> cameraUBOusingShader;
  cameraUBOusingShader.PushBack(&m_pBackground->GetShader());
  cameraUBOusingShader.PushBack(&m_pTerrain->GetTerrainShader());
  m_CameraUBO.Init(cameraUBOusingShader, "Camera");

 /* ezDynamicArray<const gl::ShaderObject*> globalSceneInfoUBOusingShader;
  globalSceneInfoUBOusingShader.PushBack(&m_pTerrain->GetShaderTerrainRenderShader());
  m_GlobalSceneInfo.Init(globalSceneInfoUBOusingShader, "GlobalSceneInfo");
  */
/*    ezDynamicArray<const gl::ShaderObject*> timeUBOusingShader;
  cameraUBOusingShader.PushBack(&m_DirectVolVisShader);
  m_TimeUBO.Init(cameraUBOusingShader, "Time");
*/
  /*
  m_GlobalSceneInfo["GlobalDirLightDirection"].Set(ezVec3(1,-2.0f,1).GetNormalized());
  m_GlobalSceneInfo["GlobalDirLightColor"].Set(ezVec3(0.98f, 0.98f, 0.8f));
  m_GlobalSceneInfo["GlobalAmbient"].Set(ezVec3(0.3f, 0.3f, 0.3f));
  */
  ezVec3 vCameraPos(m_pTerrain->GetTerrainWorldSize() / 2, 5, m_pTerrain->GetTerrainWorldSize() / 2);
  m_pCamera->SetPosition(vCameraPos);

  // user interface
  m_UserInterface->Init();
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
  //m_CameraUBO["CameraPosition"].Set(m_pCamera->GetPosition());
  
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

  // no depth test needed so far
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  // render nice background
  //m_pBackground->Draw();

  // activate depth test
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);

  glDisable(GL_CULL_FACE);

  // render processed data
  m_DrawTimer->Start();
  if(SceneConfig::g_Wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  
  m_pTerrain->Draw(m_pCamera->GetPosition() );
   
  m_DrawTimer->End();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  // disable depth test
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