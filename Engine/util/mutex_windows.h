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

#ifndef __AGS_EE_UTIL__WINDOWS_MUTEX_H
#define __AGS_EE_UTIL__WINDOWS_MUTEX_H

// FIXME: This is a horrible hack to avoid conflicts between Allegro and Windows
#define BITMAP WINDOWS_BITMAP
#include <windows.h>
#undef BITMAP

#include <crtdbg.h>


namespace AGS
{
namespace Engine
{


class WindowsMutex : BaseMutex
{
public:
  WindowsMutex()
  {
    _mutex = CreateMutex(NULL, FALSE, NULL);

    _ASSERT(_mutex != NULL);
  }

  ~WindowsMutex()
  {
    Unlock();

    _ASSERT(_mutex != NULL);

    CloseHandle(_mutex);
  }

  inline void Lock()
  {
    _ASSERT(_mutex != NULL);

    WaitForSingleObject(_mutex, INFINITE);
  }

  inline void Unlock()
  {
    _ASSERT(_mutex != NULL);

    ReleaseMutex(_mutex);
  }

private:
  HANDLE _mutex;
};


typedef WindowsMutex Mutex;


} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_UTIL__WINDOWS_MUTEX_H
