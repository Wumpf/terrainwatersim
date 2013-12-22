layout(binding = 3, shared) uniform PostProcessing
{
  float TonemapExposure;

  float LuminanceInterpolator; //1 - exp(-TimeDelta * Tau);
};