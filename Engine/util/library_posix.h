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

#ifndef __AGS_EE_UTIL__LIBRARY_POSIX_H
#define __AGS_EE_UTIL__LIBRARY_POSIX_H

#include <dlfcn.h>
#include "util/string.h"
#include "debug/out.h"

// FIXME: Replace with a unified way to get the directory which contains the engine binary
#if defined (ANDROID_VERSION)
extern char android_app_directory[256];
#else
extern char appDirectory[512];
#endif


namespace AGS
{
namespace Engine
{


class PosixLibrary : BaseLibrary
{
public:
  PosixLibrary()
    : _library(NULL)
  {
  };

  virtual ~PosixLibrary()
  {
    Unload();
  };

  AGS::Common::String BuildPath(const char *path, AGS::Common::String libraryName)
  {
    AGS::Common::String platformLibraryName = "";
    if (path)
    {
      platformLibraryName = path;
      platformLibraryName.Append("/");
    }
    platformLibraryName.Append("lib");
    platformLibraryName.Append(libraryName);

#if defined (MAC_VERSION)
    platformLibraryName.Append(".dylib");
#else
    platformLibraryName.Append(".so");
#endif

    AGS::Common::Debug::Printf("Built library path: %s", platformLibraryName.GetCStr());
    return platformLibraryName;
  }

  bool Load(AGS::Common::String libraryName)
  {
    Unload();

    // Try rpath first
    _library = dlopen(BuildPath(NULL, libraryName).GetCStr(), RTLD_LAZY);
    AGS::Common::Debug::Printf("dlopen returned: %s", dlerror());
    if (_library != NULL)
    {
      return true;
    }

    // Try current path
    _library = dlopen(BuildPath(".", libraryName).GetCStr(), RTLD_LAZY);

    AGS::Common::Debug::Printf("dlopen returned: %s", dlerror());

    if (_library == NULL)
    {
      // Try the engine directory

#if defined (ANDROID_VERSION)
      char buffer[200];
      sprintf(buffer, "%s%s", android_app_directory, "/lib");
      _library = dlopen(BuildPath(buffer, libraryName).GetCStr(), RTLD_LAZY);
#else
      _library = dlopen(BuildPath(appDirectory, libraryName).GetCStr(), RTLD_LAZY);
#endif

      AGS::Common::Debug::Printf("dlopen returned: %s", dlerror());
    }

    return (_library != NULL);
  }

  bool Unload()
  {
    if (_library)
    {
      return (dlclose(_library) == 0);
    }
    else
    {
      return true;
    }
  }

  void *GetFunctionAddress(AGS::Common::String functionName)
  {
    if (_library)
    {
      return dlsym(_library, functionName.GetCStr());
    }
    else
    {
      return NULL;
    }
  }

private:
  void *_library;
};


typedef PosixLibrary Library;



} // namespace Engine
} // namespace AGS



#endif // __AGS_EE_UTIL__LIBRARY_POSIX_H
