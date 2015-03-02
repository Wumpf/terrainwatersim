#pragma once

#include <System/Window/Window.h>

class RenderWindowGL : public ezWindow
{
public:
	RenderWindowGL();
	~RenderWindowGL();

  void SwapBuffers();
  virtual void OnWindowMessage(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM LParam) override;
  virtual void OnResizeMessage(const ezSizeU32& newWindowSize) override;
  virtual void OnClickCloseMessage();

  HDC GetWindowDC() const { return m_hDC; }

private:
  ezResult CreateGraphicsContext();

  HDC m_hDC;
  HGLRC m_hRC;
};

