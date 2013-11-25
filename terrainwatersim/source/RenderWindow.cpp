#include "PCH.h"
#include "RenderWindow.h"
#include "config/GlobalCVar.h"
#include "gl/GLUtils.h"
#include "GlobalEvents.h"
#include <Core/Input/InputManager.h>
#include <InputWindows/InputDeviceWindows.h>

namespace GeneralConfig
{
  ezCVarInt g_ResolutionWidth("ResolutionWidth", 1280, ezCVarFlags::Save | ezCVarFlags::RequiresRestart, "Backbuffer resolution in x direction");
  ezCVarInt g_ResolutionHeight("ResolutionHeight", 768, ezCVarFlags::Save | ezCVarFlags::RequiresRestart, "Backbuffer resolution in y direction");

  ezSizeU32 GetScreenResolution()
  {
    return ezSizeU32(g_ResolutionWidth.GetValue(), g_ResolutionHeight.GetValue());
  }
  ezVec2 GetScreenResolutionF()
  {
    return ezVec2(static_cast<float>(g_ResolutionWidth.GetValue()), static_cast<float>(g_ResolutionHeight.GetValue()));
  }
}

RenderWindowGL::RenderWindowGL() :
  ezWindow(),
  m_hDeviceContext(NULL), m_hRC(NULL)
{
  m_CreationDescription.m_Title = "terrainwatersim";
  m_CreationDescription.m_ClientAreaSize.width = GeneralConfig::g_ResolutionWidth.GetValue();
  m_CreationDescription.m_ClientAreaSize.height = GeneralConfig::g_ResolutionHeight.GetValue();
  m_CreationDescription.m_bFullscreenWindow = false;

  Initialize();
  CreateGraphicsContext();
}


RenderWindowGL::~RenderWindowGL()
{
  DestroyGraphicsContext();
  Destroy();
}

void RenderWindowGL::OnWindowMessage(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
  ezInputDeviceWindows::GetDevice()->WindowMessage(hWnd, Msg, WParam, LParam);
  
  GlobalEvents::Win32Message message; 
  message.msg = Msg;
  message.wnd = hWnd;
  message.lParam = LParam;
  message.wParam = WParam;
  GlobalEvents::g_pWindowMessage->Broadcast(message);
}

void RenderWindowGL::DestroyGraphicsContext()
{
  if (m_hRC)
  {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(m_hRC);
    m_hRC = NULL;
  }

  if (m_hDeviceContext)
  {
    ReleaseDC(GetNativeWindowHandle(), m_hDeviceContext);
    m_hDeviceContext = NULL;
  }
}

void RenderWindowGL::CreateGraphicsContext()
{
  // init opengl device
  int iColorBits = 24;
  int iDepthBits = 24;
  int iBPC = 8;

  PIXELFORMATDESCRIPTOR pfd =
  {
    sizeof (PIXELFORMATDESCRIPTOR),
    1, // Version
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE, // Flags
    PFD_TYPE_RGBA, // Pixeltype
    iColorBits, // Color Bits
    iBPC, 0, iBPC, 0, iBPC, 0, iBPC, 0,// Red Bits / Red Shift, Green Bits / Shift, Blue Bits / Shift, Alpha Bits / Shift
    0, 0, 0, 0, 0, // Accum Bits (total), Accum Bits Red, Green, Blue, Alpha
    iDepthBits, 8, // Depth, Stencil Bits
    0, // Aux Buffers
    PFD_MAIN_PLANE, // Layer Type (ignored)
    0, 0, 0, 0 // ignored deprecated flags
  };

  HDC hDC = GetDC (GetNativeWindowHandle());

  if (hDC == NULL)
    return;

  int iPixelformat = ChoosePixelFormat(hDC, &pfd);
  if (iPixelformat == 0)
    return;

  if (!SetPixelFormat(hDC, iPixelformat, &pfd))
    return;

  HGLRC hRC = wglCreateContext(hDC);
  if (hRC == NULL)
    return;

  if (!wglMakeCurrent(hDC, hRC))
    return;

  m_hDeviceContext = hDC;
  m_hRC = hRC;


  // load using glew
  glewExperimental = TRUE;
  GLenum err = glewInit();
  EZ_ASSERT_ALWAYS(err == GLEW_OK, "glewInit failed!");

  // enable debug output
#if defined(_DEBUG)
  gl::Utils::ActivateDebugOutput();
#endif

  // some default gl settings
  glViewport(0,1, GeneralConfig::g_ResolutionWidth, GeneralConfig::g_ResolutionHeight);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
    
}

void RenderWindowGL::SwapBuffers()
{
  ::SwapBuffers(m_hDeviceContext);
}
