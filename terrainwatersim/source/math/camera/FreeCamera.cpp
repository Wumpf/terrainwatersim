#include "PCH.h"
#include "FreeCamera.h"
#include "config/InputConfig.h"

FreeCamera::FreeCamera(ezAngle fov, float aspectRatio) :
  ezCamera(),
  m_mouseX(ezMath::BasicType<float>::Pi()),
  m_mouseY(ezMath::BasicType<float>::Pi())
{
  SetCameraMode(ezCamera::PerspectiveFixedFovY, 85.0f, 0.1f, 1000.0f);
  LookAt(ezVec3::ZeroVector(), ezVec3(1,-0.1f,0));
  UpdateMatrices();
}

void FreeCamera::ChangeAspectRatio(float newAspect)
{
  m_aspectRatio = newAspect;
}

FreeCamera::~FreeCamera()
{
}

void FreeCamera::UpdateMatrices()
{
  m_viewMatrix.SetLookAtMatrix(GetPosition(), GetPosition() + GetDirForwards(), GetDirUp());
  m_projectionMatrix.SetPerspectiveProjectionMatrixFromFovY(GetFovY(m_aspectRatio), m_aspectRatio, GetNearPlane(), GetFarPlane(), ezProjectionDepthRange::MinusOneToOne);
  m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}

void FreeCamera::Update(ezTime lastFrameDuration)
{
  ezVec3 viewDir = GetDirForwards();
  if (ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szRotationActivated_Camera, NULL) == ezKeyState::Down)
  {
    float fMouseXDeltaPos, fMouseYDeltaPos;
    ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRotateAxisXPos, &fMouseXDeltaPos);
    ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRotateAxisYPos, &fMouseYDeltaPos);
    float fMouseXDeltaNeg, fMouseYDeltaNeg;
    ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRotateAxisXNeg, &fMouseXDeltaNeg);
    ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRotateAxisYNeg, &fMouseYDeltaNeg);
    m_mouseX -= fMouseXDeltaPos - fMouseXDeltaNeg;
    m_mouseY -= fMouseYDeltaPos - fMouseYDeltaNeg;

    viewDir.x = cosf(m_mouseX) * sinf(m_mouseY);
    viewDir.y = cosf(m_mouseY);
    viewDir.z = sinf(m_mouseX) * sinf(m_mouseY);
  }

  /*
  float theta2 = m_fMouseY + ezMath::BasicType<float>::Pi() / 2.0f;
  m_vUp.x = cosf(m_fMouseX) * sinf(theta2);
  m_vUp.y = cosf(theta2);
  m_vUp.z = sinf(m_fMouseX) * sinf(theta2);
  */

  ezVec3 upDir = ezVec3(0, 1, 0);
  
  float fForward, fBackward, fLeft, fRight;
  ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraForward, &fForward);
  ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraBackward, &fBackward);
  ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraLeft, &fLeft);
  ezInputManager::GetInputActionState(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRight, &fRight);

  ezVec3 newPosition = GetPosition();
  newPosition += (fForward - fBackward) * viewDir;
  newPosition += (fRight - fLeft) * upDir.Cross(viewDir);

  LookAt(newPosition, newPosition + viewDir, upDir);
  UpdateMatrices();
}