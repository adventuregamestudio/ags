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

#ifndef __AGS_EE_UTIL__MUTEX_H
#define __AGS_EE_UTIL__MUTEX_H

namespace AGS
{
namespace Engine
{


class BaseMutex
{
public:
  BaseMutex()
  {
  };

  virtual ~BaseMutex()
  {
  };

  virtual void Lock() = 0;

  virtual void Unlock() = 0;
};


} // namespace Engine
} // namespace AGS


#if defined(WINDOWS_VERSION)
#include "mutex_windows.h"

#elif defined(PSP_VERSION)
#include "mutex_psp.h"

#elif defined(WII_VERSION)
#include "mutex_wii.h"

#elif defined(LINUX_VERSION) \
   || defined(MAC_VERSION) \
   || defined(IOS_VERSION) \
   || defined(ANDROID_VERSION)
#include "mutex_pthread.h"

#endif


#endif // __AGS_EE_UTIL__MUTEX_H
