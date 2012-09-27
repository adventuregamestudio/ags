/*
  Adventure Game Studio

  Copyright 1999 - 2011 Chris Jones.
  Copyright 2012 by various contributers.
  All rights reserved.

  The AGS source code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.
*/

#ifndef __AGS_EE_PLATFORM__THREAD_PTHREAD_H
#define __AGS_EE_PLATFORM__THREAD_PTHREAD_H

#include <pthread.h>

namespace AGS
{
namespace Engine
{


class PThreadThread : public BaseThread
{
public:
  PThreadThread()
  {
    _running = false;
  }

  ~PThreadThread()
  {
    Stop();
  }

  inline bool Create(AGSThreadEntry entryPoint, bool looping)
  {
    _looping = looping;
    _entry = entryPoint;

    // Thread creation is delayed till the thread is started
    return true;
  }

  inline bool Start()
  {
    if (!_running)
    {
      _running = (pthread_create(&_thread, NULL, _thread_start, this) != 0);

      return _running;
    }
    else
    {
      return false;
    }
  }

  bool Stop()
  {
    if (_running)
    {
      if (_looping)
      {
        _looping = false;
      }

      pthread_join(_thread, NULL);

      return true;
    }
    else
    {
      return false;
    }
  }

private:
  pthread_t _thread;
  bool      _running;
  bool      _looping;

  AGSThreadEntry _entry;

  static void *_thread_start(void *arg)
  {
    AGSThreadEntry entry = ((PThreadThread *)arg)->_entry;
    bool *looping = &((PThreadThread *)arg)->_looping;

    do
    {
      entry();
    }
    while (*looping);

    return NULL;
  }
};


typedef PThreadThread Thread;


} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_PLATFORM__THREAD_PTHREAD_H
