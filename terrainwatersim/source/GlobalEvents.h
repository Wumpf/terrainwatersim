#pragma once

namespace GlobalEvents
{
  extern ezEvent<const ezString&>* g_pShaderFileChanged;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  struct Win32Message
  {
    HWND wnd;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
  };
  extern ezEvent<const Win32Message&>* g_pWindowMessage;
#endif
};