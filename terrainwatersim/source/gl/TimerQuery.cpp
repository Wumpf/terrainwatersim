#include "PCH.h"
#include "TimerQuery.h"

namespace gl
{

  TimerQuery::TimerQuery() : m_queryResultAvailable(false)
  {
    glGenQueries(1, &m_Query);
  }

  TimerQuery::~TimerQuery(void)
  {
    glDeleteQueries(1, &m_Query);
  }

  void TimerQuery::Start()
  {
    m_queryResultAvailable = false;
    glBeginQuery(GL_TIME_ELAPSED, m_Query);
  }

  void TimerQuery::End()
  {
    glEndQuery(GL_TIME_ELAPSED);
    m_queryResultAvailable = true;
  }

  ezTime TimerQuery::GetLastTimeElapsed()
  {
    if(m_queryResultAvailable)
    {
      GLuint time = 0;
      glGetQueryObjectuiv(m_Query, GL_QUERY_RESULT, &time);

      return ezTime::NanoSeconds(time);
    }
    else
      return ezTime::Seconds(0);
  }
}
