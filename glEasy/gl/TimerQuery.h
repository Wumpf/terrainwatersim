#pragma once

namespace gl
{
  class TimerQuery
  {
  public:
    TimerQuery();
    ~TimerQuery();

    void Start();
    void End();

    /// Returns if a result is available.
    bool IsResultAvailable();

    /// returns last timer result
    /// \param wait   If false it will return the last result (0 if none there) if there is no result available
    /// \attention wait== true can stall the cpu an arbitrary amount of time!
    ezTime GetLastTimeElapsed(bool wait = false);

  private:
    QueryId m_Query;
    bool m_querySubmitted;
    ezTime m_lastResult;
  };
}


