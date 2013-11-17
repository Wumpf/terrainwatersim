#pragma once

class Camera
{
public:
  Camera(ezAngle fov, float fAspectRatio);
  virtual ~Camera();

  virtual void Update(ezTime lastFrameDuration)=0;

  const ezMat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
  const ezMat4& GetViewMatrix() const { return m_ViewMatrix; }
  const ezMat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

  const ezVec3& GetPosition() const { return m_vPosition; }
  const ezVec3& GetUp() const { return m_vUp; }
  const ezVec3& GetViewDir() const { return m_ViewDir; }

  void SetPosition(const ezVec3 vPosition) { m_vPosition = vPosition; }

protected:
  void UpdateMatrices();

  ezVec3 m_vPosition;
  ezVec3 m_vUp;
  ezVec3 m_ViewDir;

private:
  ezMat4 m_ViewProjectionMatrix;
  ezMat4 m_ProjectionMatrix;
  ezMat4 m_ViewMatrix;
};