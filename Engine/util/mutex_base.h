/*
  Adventure Game Studio

  Copyright 1999 - 2011 Chris Jones.
  Copyright 2012 by the AGS developers.
  All rights reserved.

  The AGS source code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.
*/

#ifndef __AGS_EE_PLATFORM__MUTEX_BASE_H
#define __AGS_EE_PLATFORM__MUTEX_BASE_H


namespace AGS
{
namespace Common
{


class BaseMutex
{
public:
  BaseMutex() = 0;
  virtual ~BaseMutex() = 0;
  virtual void Lock() = 0;
  virtual void Unlock() = 0;
};


} // namespace Common
} // namespace AGS

#endif // __AGS_EE_PLATFORM__MUTEX_BASE_H