#include "PCH.h"
#include "Application.h"

#include "InputConfig.h"

namespace InputConfig
{
  const char* g_szSetName_MiscInput = "Default";
  const char* g_szAction_CloseApplication = "CloseApp";

  const char* g_szSetName_Camera = "Camera";
  const char* g_szRotationActivated_Camera = "RotOn";
  const char* g_szAction_CameraForward = "Forward";
  const char* g_szAction_CameraBackward = "Backward";
  const char* g_szAction_CameraLeft = "Left";
  const char* g_szAction_CameraRight = "Right";
  const char* g_szAction_CameraRotateAxisXPos = "X+";
  const char* g_szAction_CameraRotateAxisYPos = "Y+";
  const char* g_szAction_CameraRotateAxisXNeg = "X-";
  const char* g_szAction_CameraRotateAxisYNeg = "Y-";

  const float g_fCameraMoveScale = 50.0f;
  const float g_fCameraRotationScale = 0.01f;
}

void Application::SetupInput()
{
  ezInputActionConfig inputConfig;
  inputConfig.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
  ezInputManager::SetInputActionConfig(InputConfig::g_szSetName_MiscInput, InputConfig::g_szAction_CloseApplication, inputConfig, false);


  inputConfig.m_bApplyTimeScaling = true;

  inputConfig.m_sInputSlotTrigger[0] = ezInputSlot_MouseButton1;
  ezInputManager::SetInputActionConfig(InputConfig::g_szSetName_Camera, InputConfig::g_szRotationActivated_Camera, inputConfig, false);

  inputConfig.m_fInputSlotScale[0] = InputConfig::g_fCameraMoveScale;
  inputConfig.m_sInputSlotTrigger[0] = ezInputSlot_KeyW;
  ezInputManager::SetInputActionConfig(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraForward, inputConfig, false);
  inputConfig.m_sInputSlotTrigger[0] = ezInputSlot_KeyA;
  ezInputManager::SetInputActionConfig(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraLeft, inputConfig, false);
  inputConfig.m_sInputSlotTrigger[0] = ezInputSlot_KeyS;
  ezInputManager::SetInputActionConfig(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraBackward, inputConfig, false);
  inputConfig.m_sInputSlotTrigger[0] = ezInputSlot_KeyD;
  ezInputManager::SetInputActionConfig(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRight, inputConfig, false);


  inputConfig.m_fInputSlotScale[0]= InputConfig::g_fCameraRotationScale;
  inputConfig.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegX;
  ezInputManager::SetInputActionConfig(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRotateAxisXNeg, inputConfig, false);
  inputConfig.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegY;
  ezInputManager::SetInputActionConfig(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRotateAxisYNeg, inputConfig, false);
  inputConfig.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosX;
  ezInputManager::SetInputActionConfig(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRotateAxisXPos, inputConfig, false);
  inputConfig.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosY;
  ezInputManager::SetInputActionConfig(InputConfig::g_szSetName_Camera, InputConfig::g_szAction_CameraRotateAxisYPos, inputConfig, false);
}

void Application::UpdateInput(ezTime lastFrameDuration)
{
  ezInputManager::Update(lastFrameDuration);

  if (ezInputManager::GetInputActionState("Default", "CloseApp") == ezKeyState::Pressed)
    m_bRunning = false;
}