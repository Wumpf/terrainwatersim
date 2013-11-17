#include "PCH.h"
#include "FreeCamera.h"
#include "config/InputConfig.h"

FreeCamera::FreeCamera(ezAngle fov, float aspectRatio) :
  Camera(fov, aspectRatio),
  m_fMouseX(0.0f), m_fMouseY(0.0f)
{
}

FreeCamera::~FreeCamera()
{
}

void FreeCamera::Update(ezTime lastFrameDuration)
{
   
 if(ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szRotationActivated_Camera, NULL) == ezKeyState::Down)
 {
   float fMouseXDeltaPos, fMouseYDeltaPos;
   ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRotateAxisXPos, &fMouseXDeltaPos);
   ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRotateAxisYPos, &fMouseYDeltaPos);
   float fMouseXDeltaNeg, fMouseYDeltaNeg;
   ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRotateAxisXNeg, &fMouseXDeltaNeg);
   ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRotateAxisYNeg, &fMouseYDeltaNeg);
   m_fMouseX -= fMouseXDeltaPos - fMouseXDeltaNeg;
   m_fMouseY += fMouseYDeltaPos - fMouseYDeltaNeg;

   m_ViewDir.x = cosf(m_fMouseX) * sinf(m_fMouseY);
   m_ViewDir.y = cosf(m_fMouseY);
   m_ViewDir.z = sinf(m_fMouseX) * sinf(m_fMouseY);
 }

  /*
  float theta2 = m_fMouseY + ezMath::BasicType<float>::Pi() / 2.0f;
  m_vUp.x = cosf(m_fMouseX) * sinf(theta2);
  m_vUp.y = cosf(theta2);
  m_vUp.z = sinf(m_fMouseX) * sinf(theta2);
  */

  m_vUp = ezVec3(0, 1, 0);
  
  float fForward, fBackward, fLeft, fRight;
  ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraForward, &fForward);
  ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraBackward, &fBackward);
  ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraLeft, &fLeft);
  ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRight, &fRight);

  m_vPosition += (fForward - fBackward) * m_ViewDir;
  m_vPosition += (fRight - fLeft) * m_vUp.Cross(m_ViewDir);

  UpdateMatrices();
}