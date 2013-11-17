#pragma once

#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Containers/List.h>

class FolderChangeWatcher
{
public:
  FolderChangeWatcher(const ezString& directory);
  ~FolderChangeWatcher();

  ezString PopChangedFile();

private:

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  class FolderWatchThread : public ezThread
  {
  public:
    FolderWatchThread(const ezString& sWatchedDirectory, FolderChangeWatcher& parent);
    void Stop() { m_bRun = false; }

  private:
    virtual ezUInt32 Run() EZ_OVERRIDE;

    FolderChangeWatcher& m_parent;
    const ezString m_sWatchedDirectory;
    bool m_bRun;
    HANDLE m_changeHandle;
    ezUInt64 m_lastTrackedModification;
  };
#endif
  friend FolderChangeWatcher;

  ezMutex m_queueMutex;
  ezList<ezString> m_changedFileQueue;
  FolderWatchThread* m_pWatchThread;
};

