#pragma once

namespace gl
{
  /// Class for OpenGL queries
  /// \todo Unify to all query types
  class TimerQuery
  {
  public:
    /// Creates query object
    /// \param doubleBuffered    If true two query objects will be created, at every moment one for Start/End and one for results.
    ///                           This way you can avoid stalls when using wait=true but also you'll always get last frame's result.
    TimerQuery(bool doubleBuffered = true);
    ~TimerQuery();

    void Start();
    void End();

    /// Returns if a result is available.
    bool IsResultAvailable();

    /// returns last timer result
    /// \param wait   If false it will return the last result (0 if none there) if there is no result available
    /// \attention wait= true can stall the cpu an arbitrary amount of time! This can be massively reduced if you use doubleBuffered queries
    ezTime GetLastTimeElapsed(bool wait = true);

  private:
    /// Performs a raw ogl glGetQueryObjectuiv on the correct query object
    GLuint GlGetQueryObject(GLenum queryName);

    bool m_doubleBuffered;
    QueryId m_queries[2];

    /// Index of query use for glBeginQuery
    ezUInt32 m_useQueryIdx;
    /// Index of query use for glGetQueryObjectuiv
    ezUInt32 m_getQueryIdx;

    /// True if the given gl query is neither never used nor during a begin
    bool m_queryResultAvailable[2];

    ezTime m_lastResult;
  };
}


