#include "PCH.h"
#include "ShaderObject.h"
#include "GLUtils.h"
#include "resources/UniformBuffer.h"
#include "resources/textures/Texture.h"
#include "resources/textures/Texture3D.h"

#include <GL/glew.h>

#include <Foundation/io/FileSystem/FileReader.h>
#include <Foundation/io/OSFile.h>
#include <Foundation/Basics/Types/ArrayPtr.h>

#include "..\GlobalEvents.h"

namespace gl
{
  const ShaderObject* ShaderObject::g_pCurrentlyActiveShaderObject = NULL;

  ShaderObject::ShaderObject() :
    m_Program(0),
    m_bContainsAssembledProgram(false)
  {
    for(Shader& shader : m_aShader)
    {
      shader.iGLShaderObject = 0;
      shader.sOrigin = "";
      shader.bLoaded = false;
    }

    GlobalEvents::g_pShaderFileChanged->AddEventHandler(ezEvent<const ezString&>::Handler(&ShaderObject::FileEventHandler, this));
  }

  ShaderObject::~ShaderObject()
  {
    if(g_pCurrentlyActiveShaderObject == this)
    {
      glUseProgram(0);
      g_pCurrentlyActiveShaderObject = NULL;
    }

    for(Shader& shader : m_aShader)
    {
      if(shader.bLoaded)
        glDeleteShader(shader.iGLShaderObject);
    }

    if(m_bContainsAssembledProgram)
      glDeleteProgram(m_Program);

    GlobalEvents::g_pShaderFileChanged->RemoveEventHandler(ezEvent<const ezString&>::Handler(&ShaderObject::FileEventHandler, this));
  }

  ezResult ShaderObject::AddShaderFromFile(ShaderType Type, const ezString& sFilename)
  {
    // load new code
    ezSet<ezString> includingFiles;
    ezStringBuilder sourceCode = ReadShaderFromFile(sFilename, includingFiles);
    if(sourceCode == "")
      return EZ_FAILURE;

    ezResult result = AddShaderFromSource(Type, sourceCode.GetData(), sFilename);

    if(result != EZ_FAILURE)
    {
      // memorize files
      for(auto it=includingFiles.GetIterator(); it.IsValid(); ++it)
        m_filesPerShaderType.Insert(it.Key(), Type);
    }

    return result;
  }

  ezStringBuilder ShaderObject::ReadShaderFromFile(const ezString& sFilename, ezSet<ezString>& includingFiles)
  {
    // open file
    ezFileReader file;
    if(file.Open(sFilename.GetData(), ezFileMode::Read) != EZ_SUCCESS)
    {
      ezLog::Error("Unable to open shader file %s", sFilename.GetData());
      return "";
    }

    // reserve
    ezStringBuilder sourceCode;
    ezUInt32 uiFileSize = static_cast<ezUInt32>(file.GetFileSize());
    sourceCode.Reserve(uiFileSize+1);
    ezDynamicArray<char> pData;
    pData.SetCount(uiFileSize+1);

    // read
    ezUInt64 uiReadBytes = file.ReadBytes(static_cast<ezArrayPtr<char>>(pData).GetPtr(), uiFileSize);
    EZ_ASSERT(uiReadBytes == uiFileSize, "FileSize does not matches number of bytes read.");
    pData[static_cast<ezUInt32>(uiFileSize)] = '\0';
    file.Close();
    sourceCode.Append(static_cast<ezArrayPtr<char>>(pData).GetPtr());

    // push into file list to prevent circular includes
    includingFiles.Insert(sFilename);

    // parse all includes and load files if they don't lead 
    ezString relativePath = ezPathUtils::GetFileDirectory(sFilename.GetData());
    const char* pIncludePosition = NULL;
    while((pIncludePosition = sourceCode.FindSubString("#include")) != NULL)
    {
      // parse filepath
      const char* pQuotMarksFirst = ezStringIterator(pIncludePosition).FindSubString("\"");
      if(pQuotMarksFirst == NULL)
      {
        ezLog::Error("Invalid #include directive in shader file %s. Expected \"", sFilename.GetData());
        break;
      }
      const char* pQuotMarksSecond = ezStringIterator(pQuotMarksFirst+1).FindSubString("\"");
      if(pQuotMarksSecond == NULL)
      {
        ezLog::Error("Invalid #include directive in shader file %s. Expected \"", sFilename.GetData());
        break;
      }

      ezUInt32 uiStringLength = static_cast<ezUInt32>(pQuotMarksSecond - pQuotMarksFirst);
      if(uiStringLength == 0)
      {
        ezLog::Error("Invalid #include directive in shader file %s. Quotation marks empty!", sFilename.GetData());
        break;
      }
      char* includeCommand = EZ_DEFAULT_NEW_RAW_BUFFER(char, uiStringLength);
      ezMemoryUtils::Copy(includeCommand, pQuotMarksFirst+1, uiStringLength - 1);
      includeCommand[uiStringLength - 1] = '\0';

      ezStringBuilder includeFile(relativePath.GetData());
      includeFile.AppendPath(includeCommand);

      EZ_DEFAULT_DELETE_RAW_BUFFER(includeCommand);
     

      // check if already included
      ezString includeFileString(includeFile.GetData());
      if(includingFiles.Find(includeFileString).IsValid())
      {
        sourceCode.ReplaceSubString(pQuotMarksFirst, pQuotMarksSecond+1, "");
        ezLog::Warning("Shader include file \"%s\" was already included! File will be ignored.", includeFileString.GetData());
      }
      else
      {
        sourceCode.ReplaceSubString(pIncludePosition, pQuotMarksSecond+1, ReadShaderFromFile(includeFileString, includingFiles).GetData());
      }
    }

    return sourceCode;
  }

  ezResult ShaderObject::AddShaderFromSource(ShaderType type, const ezString& pSourceCode, const ezString& sOriginName)
  {
    Shader& shader = m_aShader[static_cast<ezUInt32>(type)];

    // create shader
    GLuint shaderObjectTemp = 0;
    switch (type)
    {
    case ShaderObject::ShaderType::VERTEX:
      shaderObjectTemp = glCreateShader(GL_VERTEX_SHADER);
      break;
    case ShaderObject::ShaderType::FRAGMENT:
      shaderObjectTemp = glCreateShader(GL_FRAGMENT_SHADER);
      break;
    case ShaderObject::ShaderType::EVALUATION:
      shaderObjectTemp = glCreateShader(GL_TESS_EVALUATION_SHADER);
      break;
    case ShaderObject::ShaderType::CONTROL:
      shaderObjectTemp = glCreateShader(GL_TESS_CONTROL_SHADER);
      break;
    case ShaderObject::ShaderType::GEOMETRY:
      shaderObjectTemp = glCreateShader(GL_GEOMETRY_SHADER);
      break;
    case ShaderObject::ShaderType::COMPUTE:
      shaderObjectTemp = glCreateShader(GL_COMPUTE_SHADER);
      break;

    default:
      EZ_ASSERT(false, "Unkown shader type");
      break;
    }

    // compile shader
    const char* pSourceRaw = pSourceCode.GetData();

    glShaderSource(shaderObjectTemp, 1, &pSourceRaw, NULL);	// attach shader code

    ezResult result = gl::Utils::CheckError("glShaderSource");
    if(result == EZ_SUCCESS)
    {
      glCompileShader(shaderObjectTemp);								    // compile

      result = gl::Utils::CheckError("glCompileShader");
    }

    // gl get error seems to be unreliable - another check!
    if(result == EZ_SUCCESS)
    {
      GLint shaderCompiled;
      glGetShaderiv(shaderObjectTemp, GL_COMPILE_STATUS, &shaderCompiled);

      if(shaderCompiled == GL_FALSE)
        result = EZ_FAILURE;
    }

    // log output
    PrintShaderInfoLog(shaderObjectTemp, sOriginName);			

    // check result
    if(result == EZ_SUCCESS)
    {
      // destroy old shader
      if(shader.bLoaded)
      {
        glDeleteShader(shader.iGLShaderObject);
        shader.sOrigin = "";
      }

      // memorize new data only if loading successful - this way a failed reload won't affect anything
      shader.iGLShaderObject = shaderObjectTemp;
      shader.sOrigin = sOriginName;

      // remove old associated files
      for(auto it=m_filesPerShaderType.GetIterator(); it.IsValid(); ++it)
      {
        if(it.Value() == type)
        {
          it = m_filesPerShaderType.Erase(it);
          if(!it.IsValid())
            break;
        }
      }

      shader.bLoaded = true;
    }
    else
      glDeleteShader(shaderObjectTemp);		

    return result;
  }


  ezResult ShaderObject::CreateProgram()
  {
    // Create shader program
    GLuint tempProgram = glCreateProgram();	

    // attach programs
    int numAttachedShader = 0;
    for(Shader& shader : m_aShader)
    {
      if(shader.bLoaded)
      {
        glAttachShader(tempProgram, shader.iGLShaderObject);
        ++numAttachedShader;
      }
    }
    EZ_ASSERT(numAttachedShader > 0, "Need at least one shader to link a gl program!");

    // Link program
    glLinkProgram(tempProgram);
    ezResult result = gl::Utils::CheckError("glLinkProgram");

    // gl get error seems to be unreliable - another check!
    if(result == EZ_SUCCESS)
    {
      GLint programLinked;
      glGetProgramiv(tempProgram, GL_LINK_STATUS, &programLinked);

      if(programLinked == GL_FALSE)
        result = EZ_FAILURE;
    }
    
    // debug output
    PrintProgramInfoLog(tempProgram);

    // check
    if(result == EZ_SUCCESS)
    {
      // already a program there? destroy old one!
      if(m_bContainsAssembledProgram)
      {
        glUseProgram(0);
        glDeleteProgram(m_Program);

        // clear meta information
        m_iTotalProgramInputCount = 0;
        m_iTotalProgramOutputCount = 0;
        m_GlobalUniformInfo.Clear();
        m_UniformBlockInfos.Clear();
        m_ShaderStorageInfos.Clear();
      }

      // memorize new data only if loading successful - this way a failed reload won't affect anything
      m_Program = tempProgram;
      m_bContainsAssembledProgram = true;

      // get informations about the program
      QueryProgramInformations();

      return EZ_SUCCESS;
    }
    else
      glDeleteProgram(tempProgram);


    return result;
  }

  void ShaderObject::QueryProgramInformations()
  {
    // query basic uniform & shader storage block infos
    QueryBlockInformations(m_UniformBlockInfos, GL_UNIFORM_BLOCK);
    QueryBlockInformations(m_ShaderStorageInfos, GL_SHADER_STORAGE_BLOCK);

    // informations about uniforms ...
    GLint iTotalNumUniforms = 0;
    glGetProgramInterfaceiv(m_Program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &iTotalNumUniforms);
    const GLuint iNumQueriedUniformProps = 10;
    const GLenum pQueriedUniformProps[] = { GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE, GL_OFFSET, GL_BLOCK_INDEX, GL_ARRAY_STRIDE, GL_MATRIX_STRIDE, GL_IS_ROW_MAJOR, GL_ATOMIC_COUNTER_BUFFER_INDEX, GL_LOCATION  };
    GLint pRawUniformBlockInfoData[iNumQueriedUniformProps];
    for(int iBlock=0; iBlock<iTotalNumUniforms; ++iBlock)
    {
      // general data
      glGetProgramResourceiv(m_Program, GL_UNIFORM, iBlock, iNumQueriedUniformProps, pQueriedUniformProps, iNumQueriedUniformProps, NULL, pRawUniformBlockInfoData);
      UniformVariableInfo uniformInfo;
      uniformInfo.Type = static_cast<gl::ShaderVariableType>(pRawUniformBlockInfoData[1]);
      uniformInfo.iArrayElementCount = static_cast<ezInt32>(pRawUniformBlockInfoData[2]);
      uniformInfo.iBlockOffset = static_cast<ezInt32>(pRawUniformBlockInfoData[3]);
      uniformInfo.iArrayStride = static_cast<ezInt32>(pRawUniformBlockInfoData[5]) * 4;
      uniformInfo.iMatrixStride = static_cast<ezInt32>(pRawUniformBlockInfoData[6]);
      uniformInfo.bRowMajor = pRawUniformBlockInfoData[7] > 0;
      uniformInfo.iAtomicCounterbufferIndex = pRawUniformBlockInfoData[8];
      uniformInfo.iLocation = pRawUniformBlockInfoData[9];

      // name
      GLint iActualNameLength = 0;
      ezHybridArray<char, 64> RawName;
      RawName.SetCount(pRawUniformBlockInfoData[0] + 1);
      glGetProgramResourceName(m_Program, GL_UNIFORM, iBlock, RawName.GetCount(), &iActualNameLength, static_cast<ezArrayPtr<char>>(RawName).GetPtr());
      RawName[iActualNameLength] = '\0';
      ezString sName(static_cast<ezArrayPtr<char>>(RawName).GetPtr());

      // where to store:
      if(pRawUniformBlockInfoData[4] < 0)
        m_GlobalUniformInfo.Insert(sName, uniformInfo);
      else
      {
        for(auto it=m_UniformBlockInfos.GetIterator(); it.IsValid(); ++it)
        {
          if(it.Value().iInternalBufferIndex == pRawUniformBlockInfoData[4])
          {
            it.Value().Variables.Insert(sName, uniformInfo);
            break;
          }
        }
      }
    }

    // informations about shader storage variables 
    GLint iTotalNumStorages = 0;
    glGetProgramInterfaceiv(m_Program, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, &iTotalNumStorages);
    const GLuint iNumQueriedStorageProps = 10;
    const GLenum pQueriedStorageProps[] = { GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE, GL_OFFSET, GL_BLOCK_INDEX, GL_ARRAY_STRIDE, GL_MATRIX_STRIDE, GL_IS_ROW_MAJOR, GL_TOP_LEVEL_ARRAY_SIZE, GL_TOP_LEVEL_ARRAY_STRIDE };
    GLint pRawStorageBlockInfoData[iNumQueriedStorageProps];
    for(int iBlock=0; iBlock<iTotalNumStorages; ++iBlock)
    {
      // general data
      glGetProgramResourceiv(m_Program, GL_BUFFER_VARIABLE, iBlock, iNumQueriedStorageProps, pQueriedStorageProps, iNumQueriedStorageProps, NULL, pRawStorageBlockInfoData);
      BufferVariableInfo storageInfo;
      storageInfo.Type = static_cast<gl::ShaderVariableType>(pRawStorageBlockInfoData[1]);
      storageInfo.iArrayElementCount = static_cast<ezInt32>(pRawStorageBlockInfoData[2]);
      storageInfo.iBlockOffset = static_cast<ezInt32>(pRawStorageBlockInfoData[3]);
      storageInfo.iArrayStride = static_cast<ezInt32>(pRawStorageBlockInfoData[5]);
      storageInfo.iMatrixStride = static_cast<ezInt32>(pRawStorageBlockInfoData[6]);
      storageInfo.bRowMajor = pRawStorageBlockInfoData[7] > 0;
      storageInfo.iTopLevelArraySize = pRawStorageBlockInfoData[8];
      storageInfo.iTopLevelArrayStride = pRawStorageBlockInfoData[9];

      // name
      GLint iActualNameLength = 0;
      ezHybridArray<char, 64> RawName;
      RawName.SetCount(pRawStorageBlockInfoData[0] + 1);
      glGetProgramResourceName(m_Program, GL_BUFFER_VARIABLE, iBlock, RawName.GetCount(), &iActualNameLength, static_cast<ezArrayPtr<char>>(RawName).GetPtr());
      RawName[iActualNameLength] = '\0';
      ezString sName = static_cast<ezArrayPtr<char>>(RawName).GetPtr();

      // where to store
      for(auto it=m_ShaderStorageInfos.GetIterator(); it.IsValid(); ++it)
      {
        if(it.Value().iInternalBufferIndex == pRawStorageBlockInfoData[4])
        {
          it.Value().Variables.Insert(sName, storageInfo);
          break;
        }
      }
    }

    // other informations
    glGetProgramInterfaceiv(m_Program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &m_iTotalProgramInputCount);
    glGetProgramInterfaceiv(m_Program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &m_iTotalProgramOutputCount);
  }

  template<typename BufferVariableType>
  void ShaderObject::QueryBlockInformations(ezMap<ezString, BufferInfo<BufferVariableType>>& BufferToFill, GLenum InterfaceName)
  {
    BufferToFill.Clear();

    GLint iTotalNumBlocks = 0;
    glGetProgramInterfaceiv(m_Program, InterfaceName, GL_ACTIVE_RESOURCES, &iTotalNumBlocks);

    // gather infos about uniform blocks
    const GLuint iNumQueriedBlockProps = 4;
    const GLenum pQueriedBlockProps[] = { GL_NAME_LENGTH, GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES };
    GLint pRawUniformBlockInfoData[iNumQueriedBlockProps];
    for(int iBlock=0; iBlock<iTotalNumBlocks; ++iBlock)
    {
      // general data
      glGetProgramResourceiv(m_Program, InterfaceName, iBlock, iNumQueriedBlockProps, pQueriedBlockProps, iNumQueriedBlockProps, NULL, pRawUniformBlockInfoData);
      BufferInfo<BufferVariableType> BlockInfo;
      BlockInfo.iInternalBufferIndex = iBlock;
      BlockInfo.iBufferBinding = pRawUniformBlockInfoData[1];
      BlockInfo.iBufferDataSizeByte = pRawUniformBlockInfoData[2];// * sizeof(float);
      //BlockInfo.Variables.Reserve(pRawUniformBlockInfoData[3]);

      // name
      GLint iActualNameLength = 0;
      ezHybridArray<char, 64> RawName;
      RawName.SetCount(pRawUniformBlockInfoData[0] + 1);
      glGetProgramResourceName(m_Program, InterfaceName, iBlock, RawName.GetCount(), &iActualNameLength, static_cast<ezArrayPtr<char>>(RawName).GetPtr());
      RawName[iActualNameLength] = '\0';
      ezString key(static_cast<ezArrayPtr<char>>(RawName).GetPtr());
      BufferToFill.Insert(key, BlockInfo);
    }
  }

  GLuint ShaderObject::GetProgram() const
  {
    EZ_ASSERT(m_bContainsAssembledProgram, "No shader program ready yet. Call CreateProgram first!");
    return m_Program;
  }

  void ShaderObject::Activate() const 	
  {
    EZ_ASSERT(m_bContainsAssembledProgram, "No shader program ready yet. Call CreateProgram first!");
    glUseProgram(m_Program);
    g_pCurrentlyActiveShaderObject = this;
  }

  ezResult ShaderObject::BindUBO(UniformBuffer& ubo)
  {
    return BindUBO(ubo, ubo.GetBufferName());
  }

  ezResult ShaderObject::BindUBO(UniformBuffer& ubo, const ezString& sUBOName)
  {
    // does not change any uniform - not necessary!
    //EZ_ASSERT(g_pCurrentlyActiveShaderObject == this, "You need to activate the ShaderObject before calling BindUBO!");

    auto it = m_UniformBlockInfos.Find(sUBOName);
    if(!it.IsValid())
      return EZ_FAILURE;

    return ubo.BindBuffer(it.Value().iBufferBinding);
  }

  /*
  ezResult ShaderObject::BindImage(Texture& texture, Texture::ImageAccess accessMode, const ezString& sImageName)
  {
    auto it = m_GlobalUniformInfo.Find(sImageName);
    if(!it.IsValid())
      return EZ_FAILURE;

    // optional typechecking
    switch(it.Value().Type)
    {
    case ShaderVariableType::UNSIGNED_INT_IMAGE_3D:
    case ShaderVariableType::INT_IMAGE_3D:
    case ShaderVariableType::IMAGE_3D:
      EZ_ASSERT(dynamic_cast<Texture3D*>(&texture) != NULL, "3D Texture expected!");
      break;
    default:
      EZ_ASSERT(false, "Handling for this type of uniform not implemented!");
      break;
    }

    EZ_ASSERT(it.Value(). ??  >= 0, "Location of shader variable %s is invalid. You need to to specify the location with the layout qualifier!", it.Key());

    texture.BindImage(it.Value(). ??, accessMode);

    return EZ_SUCCESS;
  }

  ezResult ShaderObject::BindTexture(Texture& texture, const ezString& sTextureName)
  {
    auto it = m_GlobalUniformInfo.Find(sTextureName);
    if(!it.IsValid())
      return EZ_FAILURE;

    // optional typechecking
    switch(it.Value().Type)
    {
    case ShaderVariableType::UNSIGNED_INT_SAMPLER_3D:
    case ShaderVariableType::INT_SAMPLER_3D:
    case ShaderVariableType::SAMPLER_3D:
      EZ_ASSERT(dynamic_cast<Texture3D*>(&texture) != NULL, "3D Texture expected!");
      break;
    default:
      EZ_ASSERT(false, "Handling for this type of uniform not implemented!");
      break;
    }

    EZ_ASSERT(it.Value(). ?? >= 0, "Location of shader variable %s is invalid. You need to to specify the location with the layout qualifier!", it.Key());

    texture.Bind(it.Value(). ??);

    return EZ_SUCCESS;
  }
  */

  void ShaderObject::PrintShaderInfoLog(GLuint Shader, const ezString& sShaderName)
  {
#ifdef EZ_COMPILE_FOR_DEVELOPMENT
    GLint infologLength = 0;
    GLsizei charsWritten  = 0;

    glGetShaderiv(Shader, GL_INFO_LOG_LENGTH,&infologLength);		
    ezArrayPtr<char> pInfoLog = EZ_DEFAULT_NEW_ARRAY(char, infologLength);
    glGetShaderInfoLog(Shader, infologLength, &charsWritten, pInfoLog.GetPtr());
    pInfoLog[charsWritten] = '\0';
    if(strlen(pInfoLog.GetPtr()) > 0)
    {
      ezLog::Error("Shader %s compiled. Output:", sShaderName.GetData());
      ezLog::Error(pInfoLog.GetPtr());
    }
    else
      ezLog::Success("Shader %s compiled successfully", sShaderName.GetData());

    EZ_DEFAULT_DELETE_ARRAY(pInfoLog);
#endif
  }

  // Print information about the linking step
  void ShaderObject::PrintProgramInfoLog(GLuint Program)
  {
#ifdef EZ_COMPILE_FOR_DEVELOPMENT
    GLint infologLength = 0;
    GLsizei charsWritten  = 0;

    glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &infologLength);		
    ezArrayPtr<char> pInfoLog = EZ_DEFAULT_NEW_ARRAY(char, infologLength);
    glGetProgramInfoLog(Program, infologLength, &charsWritten, pInfoLog.GetPtr());
    pInfoLog[charsWritten] = '\0';

    if(strlen(pInfoLog.GetPtr()) > 0)
    {
     // ezLog::Error("Linked program. Output:");
      ezLog::Error(pInfoLog.GetPtr());
    }
    else
      ezLog::Success("Linked program successfully");

    EZ_DEFAULT_DELETE_ARRAY(pInfoLog);
#endif
  }

  /// file handler event for hot reloading
  void ShaderObject::FileEventHandler(const ezString& changedShaderFile)
  {
    auto it = m_filesPerShaderType.Find(changedShaderFile);
    if(it.IsValid())
    {
      if(m_aShader[static_cast<ezUInt32>(it.Value())].bLoaded)
      {
        ezString origin(m_aShader[static_cast<ezUInt32>(it.Value())].sOrigin);  // need to copy the string, since it could be deleted in the course of reloading..
        if(AddShaderFromFile(it.Value(), origin) != EZ_FAILURE && m_bContainsAssembledProgram)
          CreateProgram();
      }
    }
  }
}