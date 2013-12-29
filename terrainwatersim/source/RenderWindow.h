#pragma once

#include <System/Window/Window.h>

class RenderWindowGL : public ezWindow
{
public:
	RenderWindowGL();
	~RenderWindowGL();

  void SwapBuffers();
  virtual void OnWindowMessage(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM LParam) EZ_OVERRIDE;
  virtual void OnResizeMessage(const ezSizeU32& newWindowSize) EZ_OVERRIDE;

private:
  ezResult CreateGraphicsContext() EZ_OVERRIDE;
};

