#pragma once 

namespace GeneralConfig
{
  // defined in RenderWindow.cpp
  extern ezCVarInt g_ResolutionWidth;
  extern ezCVarInt g_ResolutionHeight;
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

  namespace Simulation
  {
    extern ezCVarFloat g_SimulationStepsPerSecond;
    extern ezCVarFloat g_FlowDamping;
    extern ezCVarFloat g_FlowAcceleration;
  }
}