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

#ifndef __AGS_EE_UTIL__LIBRARY_H
#define __AGS_EE_UTIL__LIBRARY_H

#include "util/string.h"

namespace AGS
{
namespace Engine
{


class BaseLibrary
{
public:
  BaseLibrary() = default;

  virtual ~BaseLibrary() = default;

  virtual bool Load(AGS::Common::String libraryName) = 0;

  virtual bool Unload() = 0;

  virtual void *GetFunctionAddress(AGS::Common::String functionName) = 0;
};


} // namespace Engine
} // namespace AGS


#if defined(WINDOWS_VERSION)
#include "library_windows.h"

#elif defined(PSP_VERSION)
#include "library_psp.h"

#elif defined(LINUX_VERSION) \
   || defined(MAC_VERSION) \
   || defined(ANDROID_VERSION)
#include "library_posix.h"

#elif defined(IOS_VERSION) \
   || defined(WII_VERSION)
#include "library_dummy.h"

#endif


#endif // __AGS_EE_UTIL__MUTEX_H
