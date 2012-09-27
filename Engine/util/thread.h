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

#ifndef __AGS_EE_UTIL__THREAD_H
#define __AGS_EE_UTIL__THREAD_H

namespace AGS
{
namespace Engine
{


class BaseThread
{
public:
  typedef void(* AGSThreadEntry)();

  BaseThread()
  {
  };

  virtual ~BaseThread()
  {
  };

  virtual bool Create(AGSThreadEntry entryPoint, bool looping) = 0;
  virtual bool Start() = 0;
  virtual bool Stop() = 0;

  inline bool CreateAndStart(AGSThreadEntry entryPoint, bool looping)
  {
    if (Create(entryPoint, looping))
    {
      if (Start())
      {
        return true;
      }
    }

    return false;
  }
};


} // namespace Engine
} // namespace AGS


#if defined(WINDOWS_VERSION)
#include "thread_windows.h"

#elif defined(PSP_VERSION)
#include "thread_psp.h"

#elif defined(WII_VERSION)
#include "thread_wii.h"

#elif defined(LINUX_VERSION) \
   || defined(MAC_VERSION) \
   || defined(IOS_VERSION) \
   || defined(ANDROID_VERSION)
#include "thread_pthread.h"

#endif


#endif // __AGS_EE_UTIL__THREAD_H
