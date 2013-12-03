#include "PCH.h"
#include "TimerQuery.h"

namespace gl
{

  TimerQuery::TimerQuery(bool doubleBuffered) : 
    m_doubleBuffered(doubleBuffered),
    m_useQueryIdx(0),
    m_getQueryIdx(doubleBuffered ? 1 : 0)
  {
    if(m_doubleBuffered)
      glGenQueries(2, m_queries);
    else
      glGenQueries(1, m_queries);

    m_queryResultAvailable[0] = false;
    m_queryResultAvailable[1] = false;
  }

  TimerQuery::~TimerQuery(void)
  {
    if(m_doubleBuffered)
      glDeleteQueries(2, m_queries);
    else
      glDeleteQueries(1, m_queries);
  }

  void TimerQuery::Start()
  {
    if(m_doubleBuffered)
    {
      m_getQueryIdx = m_useQueryIdx;
      m_useQueryIdx = 1 - m_useQueryIdx;
    }

    m_queryResultAvailable[m_useQueryIdx] = false;
    glBeginQuery(GL_TIME_ELAPSED, m_queries[m_useQueryIdx]);
  }

  void TimerQuery::End()
  {
    glEndQuery(GL_TIME_ELAPSED);
    m_queryResultAvailable[m_useQueryIdx] = true;


  }

  bool TimerQuery::IsResultAvailable()
  {
    if(m_queryResultAvailable[m_getQueryIdx])
    {
      GLuint available = 0;
      glGetQueryObjectuiv(m_queries[m_getQueryIdx], GL_QUERY_RESULT_AVAILABLE, &available);
      return available == GL_TRUE;
    }
    else
      return false;
  }

  ezTime TimerQuery::GetLastTimeElapsed(bool wait)
  {
    if(m_queryResultAvailable[m_getQueryIdx] && (wait || IsResultAvailable()))
    {
      GLuint time = 0;
      glGetQueryObjectuiv(m_queries[m_getQueryIdx], GL_QUERY_RESULT, &time);

      m_lastResult = ezTime::Nanoseconds(time);
    }

    return m_lastResult;
  }
}
