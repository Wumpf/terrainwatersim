#pragma once

#include <Core/Input/InputManager.h>

namespace InputConfig
{
  extern const char* g_szSetName_MiscInput;
  extern const char* g_szAction_CloseApplication;
  
  extern const char* g_szSetName_Camera;
  extern const char* g_szRotationActivated_Camera;
  extern const char* g_szAction_CameraForward;
  extern const char* g_szAction_CameraBackward;
  extern const char* g_szAction_CameraLeft;
  extern const char* g_szAction_CameraRight;
  extern const char* g_szAction_CameraRotateAxisXPos;
  extern const char* g_szAction_CameraRotateAxisYPos;
  extern const char* g_szAction_CameraRotateAxisXNeg;
  extern const char* g_szAction_CameraRotateAxisYNeg;

  extern const float g_fCameraMoveScale;
  extern const float g_fCameraRotationScale;
}