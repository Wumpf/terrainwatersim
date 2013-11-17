#include "PCH.h"
#include "GLUtils.h"

namespace gl
{
  namespace Utils
  {
    static void APIENTRY DebugOutput(
      GLenum source,
      GLenum type,
      GLuint id,
      GLenum severity,
      GLsizei length,
      const GLchar* message,
      GLvoid* userParam
      )
    {
      ezLogMsgType::Enum eventType = ezLogMsgType::Enum::InfoMsg;
      ezString debSource, debType, debSev;

      if(source == GL_DEBUG_SOURCE_API_ARB)
        debSource = "OpenGL";
      else if(source == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)
        debSource = "Windows";
      else if(source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)
        debSource = "Shader Compiler";
      else if(source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB)
        debSource = "Third Party";
      else if(source == GL_DEBUG_SOURCE_APPLICATION_ARB)
        debSource = "Application";
      else if(source == GL_DEBUG_SOURCE_OTHER_ARB)
        debSource = "Other";

      if(type == GL_DEBUG_TYPE_ERROR_ARB)
      {
        eventType = ezLogMsgType::Enum::ErrorMsg;
        debType = "error";
      }
      else if(type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)
      {
        eventType = ezLogMsgType::Enum::WarningMsg;
        debType = "deprecated behavior";
      }
      else if(type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)
      {
        eventType = ezLogMsgType::Enum::SeriousWarningMsg;
        debType = "undefined behavior";
      }
      else if(type == GL_DEBUG_TYPE_PORTABILITY_ARB)
      {
        eventType = ezLogMsgType::Enum::WarningMsg;
        debType = "portability";
      }
      else if(type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
      {
        eventType = ezLogMsgType::Enum::WarningMsg;
        debType = "performance";
      }
      else if(type == GL_DEBUG_TYPE_OTHER_ARB)
        debType = "message";

     
      EZ_ASSERT_ALWAYS(severity != GL_DEBUG_SEVERITY_HIGH_ARB, "Fatal GL error occurred: %s: %s(high) %d: %s", debSource.GetData(), debType.GetData(), id, message);

      if(severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
        debSev = "medium";
      else if(severity == GL_DEBUG_SEVERITY_LOW_ARB)
        debSev = "low";

      switch(eventType)
      {
      case ezLogMsgType::Enum::WarningMsg:
        ezLog::Warning("%s: %s(%s) %d: %s\n", debSource.GetData(), debType.GetData(), debSev.GetData(), id, message);
        break;
      case ezLogMsgType::Enum::SeriousWarningMsg:
        ezLog::SeriousWarning("%s: %s(%s) %d: %s\n", debSource.GetData(), debType.GetData(), debSev.GetData(), id, message);
        break;
      case ezLogMsgType::Enum::InfoMsg:
        ezLog::Info("%s: %s(%s) %d: %s\n", debSource.GetData(), debType.GetData(), debSev.GetData(), id, message);
        break;
      }
    }

    void ActivateDebugOutput(DebugMessageSeverity minMessageSeverity)
    {
      glEnable(GL_DEBUG_OUTPUT);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      switch (minMessageSeverity)
      {
      case gl::Utils::DebugMessageSeverity::LOW:
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, NULL, GL_TRUE);
      case gl::Utils::DebugMessageSeverity::MEDIUM:
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, NULL, GL_TRUE);
      case gl::Utils::DebugMessageSeverity::HIGH:
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_TRUE);
      }
      glDebugMessageCallback(&DebugOutput, NULL);
    }

    ezResult CheckError(const ezString& sTitle)
    {
      int Error = glGetError();
      if(Error != GL_NO_ERROR)
      {
        std::string ErrorString;
        switch(Error)
        {
        case GL_INVALID_ENUM:
          ErrorString = "GL_INVALID_ENUM";
          break;
        case GL_INVALID_VALUE:
          ErrorString = "GL_INVALID_VALUE";
          break;
        case GL_INVALID_OPERATION:
          ErrorString = "GL_INVALID_OPERATION";
          break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
          ErrorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
          break;
        case GL_OUT_OF_MEMORY:
          ErrorString = "GL_OUT_OF_MEMORY";
          break;
        default:
          ErrorString = "UNKNOWN";
          break;
        }
        ezLog::Error("OpenGL Error(%s): %s\n", ErrorString.c_str(), sTitle);
      }
      return Error == GL_NO_ERROR ? EZ_SUCCESS : EZ_FAILURE;
    }
  }
}