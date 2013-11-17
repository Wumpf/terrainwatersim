#pragma once

#include "ShaderDataMetaInfo.h"
#include "resources/textures/Texture.h"
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

namespace gl
{

  /// Easy to use wrapper for OpenGL shader.
  /// \todo Automatic Shader reload on file change -> state restore needed
  class ShaderObject
  {
  public:
    typedef ezMap<ezString, UniformVariableInfo> GlobalUniformInfos;
    typedef ezMap<ezString, ShaderStorageBufferMetaInfo> ShaderStorageInfos;
    typedef ezMap<ezString, UniformBufferMetaInfo> UniformBlockInfos;

    enum class ShaderType
    {
      VERTEX,
      FRAGMENT,
      EVALUATION,
      CONTROL,
      GEOMETRY,
      COMPUTE,

      NUM_SHADER_TYPES
    };

    ShaderObject();
    ~ShaderObject();

    ezResult AddShaderFromFile(ShaderType type, const ezString& sFilename);
    ezResult AddShaderFromSource(ShaderType type, const ezString& pSourceCode, const ezString& sOriginName);
    ezResult CreateProgram();

    /// Returns raw gl program identifier (you know what you're doing, right?)
    GLuint GetProgram() const;

    /// Makes program active
    /// You can only activate one program at a time
    /// \todo use new pipeline stage feature instead of glUseProgram - things like that: http://www.opengl.org/wiki/GLAPI/glBindProgramPipeline
    ///         -> then you also can acitvate multiple programs at a time because they only overwrite the stages they use
    void Activate() const;


    // Manipulation of global uniforms
    // Make sure that the ShaderObject is already activated
    // (Setting of ordinary global uniform variables is explicitly not supported! You can still handle this yourself using the available Meta-Info)

    /// Binds an ubo by name
    ezResult BindUBO(class UniformBuffer& ubo, const ezString& sUBOName);
    /// Binds an ubo by its intern buffer name
    ezResult BindUBO(UniformBuffer& ubo);


    // currently not possible to retrieve given texture binding
    /// Binds a texture
  //  ezResult BindTexture(Texture& texture, const ezString& sTextureName);
    /// Binds texture as image
  //  ezResult BindImage(Texture& texture, Texture::ImageAccess accessMode, const ezString& sImageName);



    /// The set of active user-defined inputs to the first shader stage in this program. 
    /// 
    /// If the first stage is a Vertex Shader, then this is the list of active attributes.
    /// If the program only contains a Compute Shader, then there are no inputs.
    GLint GetTotalProgramInputCount() const   { return m_iTotalProgramInputCount; }

    /// The set of active user-defined outputs from the final shader stage in this program.
    /// 
    /// If the final stage is a Fragment Shader, then this represents the fragment outputs that get written to individual color buffers.
    /// If the program only contains a Compute Shader, then there are no outputs.
    GLint GetTotalProgramOutputCount() const  { return m_iTotalProgramOutputCount; }



    /// Returns infos about global uniforms
    /// \remarks Deliberatly not const so user can use operator[] on the map
    GlobalUniformInfos& GetGlobalUniformInfo()    { return m_GlobalUniformInfo; }
    const GlobalUniformInfos& GetGlobalUniformInfo() const { return m_GlobalUniformInfo; }

    /// Returns infos about used uniform buffer definitions
    /// \remarks Deliberatly not const so user can use operator[] on the map
    UniformBlockInfos& GetUniformBufferInfo()    { return m_UniformBlockInfos; }
    const UniformBlockInfos& GetUniformBufferInfo() const { return m_UniformBlockInfos; }

    /// Returns infos about used shader storage buffer definitions
    const ShaderStorageInfos& GetShaderStorageBufferInfo() const    { return m_ShaderStorageInfos; }


  private:
    /// Print information about the compiling step
    static void PrintShaderInfoLog(GLuint shader, const ezString& sShaderName);
    /// Print information about the linking step
    static void PrintProgramInfoLog(GLuint program);

    /// file handler event for hot reloading
    void FileEventHandler(const ezString& changedShaderFile);

    /// Reads shader source code from file and performs parsing of #include directives
    static ezStringBuilder ReadShaderFromFile(const ezString& filename, ezSet<ezString>& includingFiles = ezSet<ezString>());



    /// queries uniform informations from the program
    void QueryProgramInformations();

    /// Intern helper function for gather general BufferInformations
    template<typename BufferVariableType>
    void QueryBlockInformations(ezMap<ezString, BufferInfo<BufferVariableType>>& BufferToFill, GLenum InterfaceName);
    
    // the program itself
    GLuint m_Program;
    bool m_bContainsAssembledProgram;

    /// currently active shader object
    /// As long as no user bypasses the Activate mechanism by calling glUseProgram, this pointer will always point the currently bound program.
    static const ShaderObject* g_pCurrentlyActiveShaderObject;

    /// list of relevant files - if any of these changes a reload will be triggered
    ezMap<ezString, ShaderType> m_filesPerShaderType;

    // underlying shaders
    struct Shader
    {
      GLuint    iGLShaderObject;
      ezString  sOrigin;
      bool      bLoaded;
    };
    Shader m_aShader[ShaderType::NUM_SHADER_TYPES];

    // meta information
    GlobalUniformInfos m_GlobalUniformInfo;
    UniformBlockInfos  m_UniformBlockInfos;
    ShaderStorageInfos m_ShaderStorageInfos;

    // misc
    GLint m_iTotalProgramInputCount;  ///< \see GetTotalProgramInputCount
    GLint m_iTotalProgramOutputCount; ///< \see GetTotalProgramOutputCount

    // currently missing meta information
    // - transform feedback buffer
    // - transform feedback varying
    // - subroutines
    // - atomic counter buffer
  };

}