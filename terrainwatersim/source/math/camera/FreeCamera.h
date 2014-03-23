#pragma once

#include <CoreUtils/Graphics/Camera.h>

class FreeCamera : public ezCamera
{
public:
  FreeCamera(ezAngle fov, float aspectRatio);
  virtual ~FreeCamera();

  void Update(ezTime lastFrameDuration);

  void ChangeAspectRatio(float newAspect);

  const ezMat4& GetViewMatrix()           { return m_viewMatrix; }
  const ezMat4& GetProjectionMatrix()     { return m_projectionMatrix; }
  const ezMat4& GetViewProjectionMatrix() { return m_viewProjectionMatrix; }

private:
  void UpdateMatrices();

  float m_aspectRatio;

  float m_mouseX, m_mouseY;
  ezMat4 m_viewMatrix;
  ezMat4 m_projectionMatrix;
  ezMat4 m_viewProjectionMatrix;
};

