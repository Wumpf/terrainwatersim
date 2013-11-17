#pragma once

#include <Foundation/Containers/StaticRingBuffer.h>

namespace gl
{
  class Font;
}
class RenderWindowGL;

class OnScreenLogWriter
{
public:
  OnScreenLogWriter(const RenderWindowGL& renderWindow);
  ~OnScreenLogWriter();

  /// \brief Register this at ezLog to write all log messages to an HTML file.
  void LogMessageHandler(const ezLoggingEventData& eventData);

  ezResult Update(ezTime lastFrameDuration);
  ezResult Render(); 

private:
  struct LogEntry
  {
    ezString text;
    ezColor color;
  };

  static const ezVec2 m_vScreenPos;
  static const ezUInt32 m_uiMaxDisplayedMessages = 10;
  ezStaticRingBuffer<LogEntry, m_uiMaxDisplayedMessages> m_MessageBuffer;

  ezUniquePtr<gl::Font> m_pFont;

  ezString m_sCurrentGroup;
  
  static const float m_fFadeSpeed;
  float m_fOldestItemFade;
};