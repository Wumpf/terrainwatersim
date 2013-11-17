#pragma once

#include <System/Window/Window.h>

class RenderWindowGL : public ezWindow
{
public:
	RenderWindowGL();
	~RenderWindowGL();

  void CreateGraphicsContext();
  void DestroyGraphicsContext();

  void SwapBuffers();
  virtual void OnWindowMessage(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM LParam) EZ_OVERRIDE;
  
  HDC GetDeviceContext() const { return m_hDeviceContext; }

private:
  HDC m_hDeviceContext;
  HGLRC m_hRC;
};

