#pragma once

#include "Camera.h"

class FreeCamera : public Camera
{
public:
  FreeCamera(ezAngle fov, float aspectRatio);
  virtual ~FreeCamera();

  virtual void Update(ezTime lastFrameDuration) EZ_OVERRIDE;

private:
  float m_fMouseX, m_fMouseY;
};

