#include "PCH.h"
#include "FolderChangeWatcher.h"
#include <Foundation/IO/OSFile.h>

FolderChangeWatcher::FolderChangeWatcher(const ezString& sDirectory) :
  m_pWatchThread(NULL)
{
  #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  m_pWatchThread = EZ_DEFAULT_NEW(FolderWatchThread)(sDirectory, *this);
  m_pWatchThread->Start();
  #endif
}

FolderChangeWatcher::~FolderChangeWatcher(void)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if(m_pWatchThread)
  {
    m_pWatchThread->Stop();
    m_pWatchThread->Join();
  }
  EZ_DEFAULT_DELETE(m_pWatchThread);
#endif
}

ezString FolderChangeWatcher::PopChangedFile()
{
  ezString output = "";

  m_queueMutex.Acquire();
  if(!m_changedFileQueue.IsEmpty())
  {
    output = m_changedFileQueue.PeekFront();
    m_changedFileQueue.PopFront();
  }
  m_queueMutex.Release();

  return output;
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

FolderChangeWatcher::FolderWatchThread::FolderWatchThread(const ezString& sWatchedDirectory, FolderChangeWatcher& parent) :
  ezThread(sWatchedDirectory.GetData()),
  m_parent(parent),
  m_watchedDirectory(sWatchedDirectory),
  m_run(true)
{
  m_changeHandle = FindFirstChangeNotificationW( 
    ezStringWChar(sWatchedDirectory.GetData()).GetData(),      // directory to watch 
    TRUE,                         // watch subtree 
    FILE_NOTIFY_CHANGE_LAST_WRITE); // watch file name changes 

  if (m_changeHandle == INVALID_HANDLE_VALUE) 
    ezLog::Error("FindFirstChangeNotification function failed.");

  // get current filetime
  SYSTEMTIME systemTime;
  GetSystemTime(&systemTime);
  SystemTimeToFileTime(&systemTime, reinterpret_cast<LPFILETIME>(&m_lastTrackedModification));
}

ezUInt32 FolderChangeWatcher::FolderWatchThread::Run()
{
  ezList<ezString> directoryQueue;

  while(m_run)
  {
    DWORD waitStatus = WaitForMultipleObjects(1, &m_changeHandle, TRUE, 200);

    if(waitStatus == WAIT_OBJECT_0)
    {
      directoryQueue.PushBack("");

      ezUInt64 lastTrackedModificationNew = m_lastTrackedModification;

      while(!directoryQueue.IsEmpty())
      {
        ezString currentSubDirectory = directoryQueue.PeekFront();
        directoryQueue.PopFront();

        ezStringBuilder wildcardPath(m_watchedDirectory.GetData());
        wildcardPath.AppendPath(currentSubDirectory.GetData(), "*");
        ezStringWChar wildcardPath_wchar(wildcardPath.GetData());

        // iterate over each file
        WIN32_FIND_DATAW ffd;
        HANDLE hFind = FindFirstFileW(wildcardPath_wchar.GetData(), &ffd);
        if(hFind == INVALID_HANDLE_VALUE)
          continue; // don't log this, happens far to often!


        do {
          if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && ffd.cFileName[0] != L'.')
          {
            ezStringBuilder newDirectory(currentSubDirectory.GetData());
            newDirectory.AppendPath(ezStringUtf8(ffd.cFileName).GetData());
            directoryQueue.PushBack(newDirectory);
          }
          else
          {
            ezUInt64 fileTime = *reinterpret_cast<ezUInt64*>(&ffd.ftLastWriteTime);
            if(fileTime > m_lastTrackedModification)
            {
              lastTrackedModificationNew = std::max(lastTrackedModificationNew, fileTime);

              ezStringBuilder changedFile(currentSubDirectory.GetData());
              changedFile.AppendPath(ezStringUtf8(ffd.cFileName).GetData());

              m_parent.m_queueMutex.Acquire();
              m_parent.m_changedFileQueue.PushBack(changedFile.GetData());
              m_parent.m_queueMutex.Release();

              ezLog::Info("File was changed: \"%s\"", changedFile.GetData());
            }
          }
        } while(FindNextFile(hFind, &ffd) != 0);
        FindClose(hFind);
      }

      m_lastTrackedModification = lastTrackedModificationNew;
    }
  }

  return 0;
}

#endif