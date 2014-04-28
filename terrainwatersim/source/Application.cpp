#include "PCH.h"

#include "Application.h"
#include "RenderWindow.h"
#include "scene/Scene.h"
#include "OnScreenLogWriter.h"
#include "math/Random.h"

#include <Foundation/Configuration/Startup.h>

#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Logging/HTMLWriter.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>

#include <Foundation/Utilities/Stats.h>

#include "GlobalEvents.h"

#include <gl/ShaderObject.h>


Application::Application() :
   m_pHTMLLogWriter(nullptr),
   m_bRunning(true),
   m_pWindow(NULL),
   m_pScene(NULL)
{
}

Application::~Application()
{
}

void Application::AfterEngineInit()
{
  // start engine
  ezStartup::StartupEngine();

  // setups file system stuff
  SetupFileSystem();

  // setup log
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  m_pHTMLLogWriter = EZ_DEFAULT_NEW(ezLogWriter::HTML);
  m_pHTMLLogWriter->BeginLog("log.html", "voxelsurfacerealtimeexp");
  ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, m_pHTMLLogWriter));

  // load global config variables
  ezCVar::SetStorageFolder("CVars");
  ezCVar::LoadCVars();

  // global events
  GlobalEvents::g_pWindowMessage = EZ_DEFAULT_NEW(ezEvent<const GlobalEvents::Win32Message&>);
  
  // setup random
  Random::Init(231656522); // 231656522

  // start window
  m_pWindow = EZ_DEFAULT_NEW(RenderWindowGL);

  // onscreen log
  m_pOnScreenLogWriter = EZ_DEFAULT_NEW(OnScreenLogWriter)(*m_pWindow);
  ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler(&OnScreenLogWriter::LogMessageHandler, m_pOnScreenLogWriter));

  // setup input
  SetupInput();

  // load graphics stuff
  m_pScene = EZ_DEFAULT_NEW(Scene)(*m_pWindow);

  // reset time
  m_LastFrameTime = ezTime::Now();
}

void Application::BeforeEngineShutdown()
{
  ezStartup::ShutdownEngine();

  EZ_DEFAULT_DELETE(m_pWindow);

  EZ_DEFAULT_DELETE(m_pScene);

  ezGlobalLog::RemoveLogWriter(ezLoggingEvent::Handler(&OnScreenLogWriter::LogMessageHandler, m_pOnScreenLogWriter));
  EZ_DEFAULT_DELETE(m_pOnScreenLogWriter);
  
  EZ_DEFAULT_DELETE(m_pShaderChangesWatcher);
  EZ_DEFAULT_DELETE(GlobalEvents::g_pWindowMessage);

  ezGlobalLog::RemoveLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, m_pHTMLLogWriter));
  m_pHTMLLogWriter->EndLog();
  EZ_DEFAULT_DELETE(m_pHTMLLogWriter);
}

void Application::handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
{
  if (action == FW::Actions::Modified)
    gl::ShaderObject::s_shaderFileChangedEvent.Broadcast(filename.c_str());
}

ezApplication::ApplicationExecution Application::Run()
{
  // timing
  ezTime now = ezTime::Now();
  ezTime lastFrameDuration = now - m_LastFrameTime;
  m_LastFrameTime = now;

  // update
  Update(lastFrameDuration);

  // rendering
  RenderFrame(lastFrameDuration);


  return m_bRunning ? ezApplication::Continue : ezApplication::Quit;
}

void Application::Update(ezTime lastFrameDuration)
{
  UpdateInput(lastFrameDuration);
  m_pScene->Update(lastFrameDuration);
  m_pWindow->ProcessWindowMessages();
  m_bRunning = m_pWindow->IsInitialized();
  m_pOnScreenLogWriter->Update(lastFrameDuration);

  m_pShaderChangesWatcher->update();

  // Update frame statistics.
  ezStringBuilder stats; stats.Format("%.2f fps", 1.0 / lastFrameDuration.GetSeconds());
  ezStats::SetStat("Frames per second", stats.GetData());
  stats.Format("%.2f ms", lastFrameDuration.GetMilliseconds());
  ezStats::SetStat("Frame time", stats.GetData()); 
}

void Application::RenderFrame(ezTime lastFrameDuration)
{
  m_pScene->Render(lastFrameDuration);
  m_pOnScreenLogWriter->Render();
  m_pScene->RenderUI();

  m_pWindow->SwapBuffers();
}

void Application::SetupFileSystem()
{
  ezStringBuilder applicationDir(ezOSFile::GetApplicationDirectory());

  ezOSFile::CreateDirectoryStructure(applicationDir.GetData());
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

  ezFileSystem::AddDataDirectory(applicationDir.GetData(), ezFileSystem::AllowWrites, "general", "");
  
  ezStringBuilder shaderDir(applicationDir);
  shaderDir.AppendPath("..", "..", "..", "terrainwatersim"); // dev only! otherwise the realtime shader editing doesn't work as expected (since path is too long for automated copy -.-)
  shaderDir.AppendPath("Shader");
  ezFileSystem::AddDataDirectory(shaderDir.GetData(), ezFileSystem::ReadOnly, "graphics", "");
  m_pShaderChangesWatcher = EZ_DEFAULT_NEW(FW::FileWatcher)();
  m_pShaderChangesWatcher->addWatch(shaderDir.GetData(), this);

  ezStringBuilder textureDir(applicationDir);
  textureDir.AppendPath("..", "..", "..", "terrainwatersim"); // dev only! otherwise manual copies must be made (since path is too long for automated copy -.-)
  textureDir.AppendPath("textures");
  ezFileSystem::AddDataDirectory(textureDir.GetData(), ezFileSystem::ReadOnly, "graphics", "");
  // optionally a texture change watcher could be established here ;)
}

EZ_APPLICATION_ENTRY_POINT(Application);