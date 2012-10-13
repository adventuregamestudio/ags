//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

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
