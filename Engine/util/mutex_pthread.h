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

#ifndef __AGS_EE_UTIL__MUTEX_PTHREAD_H
#define __AGS_EE_UTIL__MUTEX_PTHREAD_H

#include <pthread.h>

namespace AGS
{
namespace Engine
{


class PThreadMutex : BaseMutex
{
public:
  inline PThreadMutex()
  {
    pthread_mutex_init(&_mutex, NULL);
  }

  inline ~PThreadMutex()
  {
    Unlock();
    pthread_mutex_destroy(&_mutex);
  }

  inline void Lock()
  {
    pthread_mutex_lock(&_mutex);
  }

  inline void Unlock()
  {
    pthread_mutex_unlock(&_mutex);
  }

private:
  pthread_mutex_t _mutex;
};

typedef PThreadMutex Mutex;


} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_UTIL__MUTEX_PTHREAD_H
