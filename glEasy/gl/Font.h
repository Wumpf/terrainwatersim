#pragma once

namespace gl
{

  /// \brief simple font rendering
  /// \remarks This opengl implementation is oriented on http://nehe.gamedev.net/tutorial/bitmap_fonts/17002/ <br>
  /// <i>Neither fast nor clean, but it works for now</i>
  class Font
  {
  public:
#if EZ_DISABLED(EZ_PLATFORM_WINDOWS)
  #error "No gl::Font class for this platform yet"
#endif

    /// \brief creates a new bitmap font from system font
    /// \param fontName		specify installed system font name
    /// \param window		main window of the application
    Font(const ezString& sFontName, int size, const HDC deviceContext);
    ~Font();

    /// \brief draws a string
    /// \param text				text to be rendered
    /// \param screenPosition	relative screen position (starting in upper left corner, 0-1)
    /// \param color			text's color
    void DrawString(const ezString& text, const ezVec2& screenPosition, const ezColor& color = ezColor::GetWhite());

  private:
    GLuint m_DisplayList;
  };

}