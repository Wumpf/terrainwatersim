layout(binding = 0, shared) uniform Camera
{
  mat4 ViewProjection;
  mat4 InverseViewProjection;
  mat4 ViewMatrix;
  vec3 CameraPosition;
};

layout(binding = 1, shared) uniform Time
{
  float CurrentTime;
  float LastFrameDuration;
};

layout(binding = 2, shared) uniform GlobalSceneInfo
{
  vec3 GlobalDirLightDirection;
  vec3 GlobalDirLightColor;
  vec3 GlobalAmbient;
};

layout(binding = 3, shared) uniform VolumeDataInfo
{
  ivec3 VolumeMaxTextureLoad;
  vec3 VolumeWorldSize;
  vec3 VolumePosToTexcoord;

  float GradientDescendStepMultiplier;
  int GradientDescendStepCount;
};

const float IsoValue = 0.5;