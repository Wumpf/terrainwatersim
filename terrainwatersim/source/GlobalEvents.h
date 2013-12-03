#pragma once

namespace GlobalEvents
{

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  struct Win32Message
  {
    HWND wnd;
    INT msg;
    WPARAM wParam;
    LPARAM lParam;
  };
  extern ezEvent<const Win32Message&>* g_pWindowMessage;
#endif
};