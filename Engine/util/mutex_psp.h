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

#ifndef __AGS_EE_UTIL__PSP_MUTEX_H
#define __AGS_EE_UTIL__PSP_MUTEX_H

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspthreadman.h>

namespace AGS
{
namespace Engine
{


class PSPMutex : BaseMutex
{
public:
  PSPMutex()
  {
    _mutex = sceKernelCreateSema("", 0, 1, 1, 0);
  }

  ~PSPMutex()
  {
    Unlock();
    sceKernelDeleteSema(_mutex);
  }

  inline void Lock()
  {
    sceKernelWaitSema(_mutex, 1, 0);
  }

  inline void Unlock()
  {
    sceKernelSignalSema(_mutex, 1);
  }

private:
  SceUID _mutex;
};


typedef PSPMutex Mutex;


} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_UTIL__PSP_MUTEX_H
