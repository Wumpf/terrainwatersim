#include "PCH.h"
#include "TimerQuery.h"

namespace gl
{

  TimerQuery::TimerQuery() : m_querySubmitted(false)
  {
    glGenQueries(1, &m_Query);
  }

  TimerQuery::~TimerQuery(void)
  {
    glDeleteQueries(1, &m_Query);
  }

  void TimerQuery::Start()
  {
    m_querySubmitted = false;
    glBeginQuery(GL_TIME_ELAPSED, m_Query);
  }

  void TimerQuery::End()
  {
    glEndQuery(GL_TIME_ELAPSED);
    m_querySubmitted = true;
  }

  bool TimerQuery::IsResultAvailable()
  {
    if(m_querySubmitted)
    {
      GLuint available = 0;
      glGetQueryObjectuiv(m_Query, GL_QUERY_RESULT_AVAILABLE, &available);
      return available == GL_TRUE;
    }
    else
      return false;
  }

  ezTime TimerQuery::GetLastTimeElapsed(bool wait)
  {
    if(m_querySubmitted && (wait || IsResultAvailable()))
    {
      GLuint time = 0;
      glGetQueryObjectuiv(m_Query, GL_QUERY_RESULT, &time);

      m_lastResult = ezTime::Nanoseconds(time);
    }

    return m_lastResult;
  }
}
