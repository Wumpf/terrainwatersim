#include "PCH.h"
#include "Font.h"
#include "..\RenderWindow.h"

namespace gl
{
  Font::Font(const ezString& fontName, int size, const HDC deviceContext)
  {
	  HFONT font;
	  HFONT oldfont;

	  m_DisplayList = glGenLists(96);		

	  font = CreateFontA(	size,
						  0,		
						  0,						
						  0,		
						  FW_REGULAR,	
						  FALSE,					
						  FALSE,	
						  FALSE,
						  ANSI_CHARSET,				
						  OUT_TT_PRECIS,				
						  CLIP_DEFAULT_PRECIS,		
						  ANTIALIASED_QUALITY,		
						  FF_DONTCARE|DEFAULT_PITCH,	
						  fontName.GetData());	

	  oldfont = (HFONT)SelectObject(deviceContext, font);      
	  wglUseFontBitmaps(deviceContext, 32, 96, m_DisplayList);
	  SelectObject(deviceContext, oldfont);		
	  DeleteObject(font);	
  }

  Font::~Font(void)
  {
	  glDeleteLists(m_DisplayList, 96);
  }

  void Font::DrawString(const ezString& sText, const ezVec2& screenPosition, const ezColor& color)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	  glUseProgram(0);
	  glColor4fv(color);

	  float screenPosX = screenPosition.x * 2.0f - 1.0f;
	  float screenPosY = 1.0f - screenPosition.y * 2.0f;
	 // screenPosY -= 20.0f / RenderDevice::Get().GetBackBufferHeight(); // padding due to window frame - the direct3d implementatoin is aware of it
	  glRasterPos2f(screenPosX, screenPosY);

	  glPushAttrib(GL_LIST_BIT);					
	  glListBase(m_DisplayList - 32);
	  glCallLists(static_cast<GLsizei>(sText.GetElementCount()), GL_UNSIGNED_BYTE, sText.GetData());
	  glPopAttrib();

    glDisable(GL_BLEND);
  }
}