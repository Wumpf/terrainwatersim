#pragma once

namespace gl
{
  // class for rendering a screen aligned triangle
  class ScreenAlignedTriangle
  {
  public:
    static void Draw();

    ~ScreenAlignedTriangle();

  private:
    ScreenAlignedTriangle();

    static ScreenAlignedTriangle& GetInstance()
    {
      static ScreenAlignedTriangle m_instance;
      return m_instance;
    };

    BufferId vbo;
    VertexArrayObjectId vao;
  };
}