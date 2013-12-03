#pragma once

namespace gl
{
  namespace Utils
  {
    /// classes of Debug message severity
    enum class DebugMessageSeverity
    {
      LOW,
      MEDIUM,
      HIGH
    };

    /// Activates the OpenGL 4.3 debug output
    void ActivateDebugOutput(DebugMessageSeverity minMessageSeverity = DebugMessageSeverity::MEDIUM);

    /// checks if there is any OpenGL error
    ezResult CheckError(const ezString& sTitle);
  }

  // Buffer definition for glDrawArraysIndirect
  struct DrawArraysIndirectCommand  // http://www.opengl.org/sdk/docs/man/xhtml/glDrawArraysInstancedBaseInstance.xml
  {
    ezUInt32 count;
    ezUInt32 primCount;
    ezUInt32 first;
    ezUInt32 baseInstance;
  };
}