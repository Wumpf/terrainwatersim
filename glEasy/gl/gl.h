#pragma once

#include <GL/glew.h>

namespace gl
{
  // Typedefs for different kinds of OpenGL IDs for type safety.
  typedef GLuint ShaderId;
  typedef GLuint ProgramId;

  typedef GLuint BufferId;
  typedef GLuint IndexBufferId;
  typedef GLuint VertexArrayObjectId;

  typedef GLuint TextureId;
  typedef GLuint Framebuffer;
  typedef GLuint SamplerId;

  typedef GLuint QueryId;
}