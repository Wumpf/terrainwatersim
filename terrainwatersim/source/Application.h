#pragma once

#include <Core/Application/Application.h>
#include "FileWatcher/FileWatcher.h"

namespace ezLogWriter
{
  class HTML;
}
class OnScreenLogWriter;

/// Basic application framework
class Application : public ezApplication, FW::FileWatchListener
{
public:
  Application();
  ~Application();

  virtual void AfterEngineInit() EZ_OVERRIDE;
  virtual void BeforeEngineShutdown() EZ_OVERRIDE;
  virtual ezApplication::ApplicationExecution Run() EZ_OVERRIDE;

  void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action);

private:
  void SetupFileSystem();
  void SetupInput();
  

  void Update(ezTime lastFrameDuration);
  void RenderFrame(ezTime lastFrameDuration);

  void UpdateInput(ezTime lastFrameDuration);


  class RenderWindowGL* m_pWindow;
  class Scene* m_pScene;

  FW::FileWatcher* m_pShaderChangesWatcher;

  ezTime m_LastFrameTime;
  bool m_bRunning;

  ezLogWriter::HTML* m_pHTMLLogWriter;
  OnScreenLogWriter* m_pOnScreenLogWriter;
};
