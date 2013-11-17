#include "PCH.h"
#include "GLUtils.h"
#include "ScreenAlignedTriangle.h"

namespace gl
{

  struct ScreenTriVertex
  {
    float position[2];
  };

  ScreenAlignedTriangle::ScreenAlignedTriangle()
  {
    ScreenTriVertex screenTriangle[3];
    screenTriangle[0].position[0] = -1.0f;
    screenTriangle[0].position[1] = -1.0f;
    screenTriangle[1].position[0] = 3.0f;
    screenTriangle[1].position[1] = -1.0f;
    screenTriangle[2].position[0] = -1.0f;
    screenTriangle[2].position[1] = 3.0f;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenTriangle), screenTriangle, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl::Utils::CheckError("screenTri_vbo");


    // generate vao for cloud particles (2 for ping/pong)
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    gl::Utils::CheckError("screenTri_vao");
  }


  ScreenAlignedTriangle::~ScreenAlignedTriangle(void)
  {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
  }

  void ScreenAlignedTriangle::Draw() const
  {
    //glDisable(GL_DEPTH_TEST);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    //glEnable(GL_DEPTH_TEST);
  }

}