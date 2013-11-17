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

    /// returns last timer result
    /// \attention This can stall the cpu an arbitrary amount of time!
    ezTime GetLastTimeElapsed();

  private:
    GLuint m_Query;
    bool m_queryResultAvailable;
  };
}


