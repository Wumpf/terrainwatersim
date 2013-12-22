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
  //uint NumMSAASamples;
};