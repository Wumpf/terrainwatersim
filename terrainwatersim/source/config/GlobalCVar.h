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

}