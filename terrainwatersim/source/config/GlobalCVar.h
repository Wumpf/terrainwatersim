#pragma once 


#define CVarRGBDecl(name) \
  extern ezCVarFloat name##R; \
  extern ezCVarFloat name##G; \
  extern ezCVarFloat name##B;

#define CVarRGBImpl(name, nameString, defaultValue, flags, desc) \
  ezCVarFloat name##R(EZ_CONCAT(nameString, " R"), (defaultValue).x, (flags), (desc)); \
  ezCVarFloat name##G(EZ_CONCAT(nameString, " G"), (defaultValue).y, (flags), (desc)); \
  ezCVarFloat name##B(EZ_CONCAT(nameString, " B"), (defaultValue).z, (flags), (desc));


namespace GeneralConfig
{
  // defined in RenderWindow.cpp
  extern ezCVarInt g_ResolutionWidth;
  extern ezCVarInt g_ResolutionHeight;
  //extern ezCVarInt g_MSAASamples;
  ezSizeU32 GetScreenResolution();
  ezVec2 GetScreenResolutionF();
}

namespace SceneConfig
{
  namespace TerrainRendering
  {
    extern ezCVarBool g_Wireframe;
    extern ezCVarFloat g_PixelPerTriangle;
    extern ezCVarBool g_UseAnisotropicFilter;
  }

  namespace WaterRendering
  {
    CVarRGBDecl(g_waterSurfaceColor);
    CVarRGBDecl(g_waterBigDepthColor);
    CVarRGBDecl(g_waterExtinctionCoefficients);
    extern ezStatic<ezEvent<ezVec3>> g_waterExtinctionCoefficients_changedEvent;

    extern ezCVarFloat g_waterOpaqueness;
  }

  namespace Simulation
  {
    extern ezCVarFloat g_SimulationStepsPerSecond;
    extern ezCVarFloat g_FlowDamping;
    extern ezCVarFloat g_FlowAcceleration;
  }
}