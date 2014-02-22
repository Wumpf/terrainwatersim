#include "PCH.h"
#include "Camera.h"

Camera::Camera(ezAngle fov, float aspectRatio, float nearPlane, float farPlane) :
  m_vPosition(0.0f, 200.0f, 0.0f),
  m_vUp(0.0f, 1.0f, 0.0f),
  m_ViewDir(1.0f, 0.0f, 0.0f),

  m_nearPlane(nearPlane),
  m_farPlane(farPlane),
  m_fov(fov),
  m_aspectRatio(aspectRatio)
{
  ChangeAspectRatio(aspectRatio);
}

void Camera::ChangeAspectRatio(float newAspectRatio)
{
  m_aspectRatio = newAspectRatio;
  m_ProjectionMatrix.SetPerspectiveProjectionMatrixFromFovY(m_fov, m_aspectRatio, m_nearPlane, m_farPlane, ezProjectionDepthRange::MinusOneToOne);
  UpdateMatrices();
}

Camera::~Camera(void)
{
}

void Camera::UpdateMatrices()
{
  m_ViewMatrix.SetLookAtMatrix(m_vPosition, m_vPosition + m_ViewDir, m_vUp);
  m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}