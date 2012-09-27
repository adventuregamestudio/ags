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

#ifndef __AGS_EE_UTIL__WII_MUTEX_H
#define __AGS_EE_UTIL__WII_MUTEX_H

#include <gccore.h>

namespace AGS
{
namespace Engine
{


class WiiMutex : Mutex
{
public:
  inline Mutex()
  {
    LWP_MutexInit(&_mutex, 0);
  }

  inline ~Mutex()
  {
    Unlock();
    LWP_MutexDestroy(_mutex);
  }

  inline void Lock()
  {
    LWP_MutexLock(_mutex);
  }

  inline void Unlock()
  {
    LWP_MutexUnlock(_mutex);
  }

private:
  mutex_t _mutex;
};


typedef WiiMutex Mutex;


} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_UTIL__WII_MUTEX_H
