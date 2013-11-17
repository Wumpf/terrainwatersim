#include "PCH.h"
#include "GlobalEvents.h"

namespace GlobalEvents
{
  ezEvent<const ezString&>* g_pShaderFileChanged;
  ezEvent<const Win32Message&>* g_pWindowMessage;
};