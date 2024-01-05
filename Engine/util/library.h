//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// BaseLibrary is a virtual parent class for a dynamic library loader.
//
//=============================================================================
#ifndef __AGS_EE_UTIL__LIBRARY_H
#define __AGS_EE_UTIL__LIBRARY_H

#include "core/platform.h"
#include "util/string.h"

namespace AGS
{
namespace Engine
{

using AGS::Common::String;

class BaseLibrary
{
public:
    BaseLibrary() = default;
    BaseLibrary(const BaseLibrary&) = delete;
    BaseLibrary(BaseLibrary&&) = default;
    virtual ~BaseLibrary() = default;

    // Get library name; returns empty string if not loaded
    inline String GetName() const { return _name; }
    // Get actual filename; returns empty string if not loaded
    inline String GetFileName() const { return _filename; }
    // Get path used to load the library; or empty string is not loaded.
    // NOTE: this is NOT a fully qualified path, but a lookup path.
    inline String GetPath() const { return _path; }

    // Returns expected filename form for the dynamic library of a given name
    virtual String GetFilenameForLib(const String &libname) = 0;

    // Try load a library of a given name, optionally looking it up in the list of paths
    virtual bool Load(const String &libname,
        const std::vector<String> &lookup = std::vector<String>()) = 0;
    // Unload a library; does nothing if was not loaded
    virtual void Unload() = 0;
    // Tells if library is loaded
    virtual bool IsLoaded() const = 0;
    // Tries to get a function address from a loaded library
    virtual void *GetFunctionAddress(const String &fn_name) = 0;

protected:
    String _name;
    String _filename;
    String _path;
};

} // namespace Engine
} // namespace AGS


#if AGS_PLATFORM_OS_WINDOWS
#include "library_windows.h"

#elif AGS_PLATFORM_OS_LINUX \
   || AGS_PLATFORM_OS_MACOS \
   || AGS_PLATFORM_OS_ANDROID \
   || AGS_PLATFORM_OS_FREEBSD
#include "library_posix.h"

#else
#include "library_dummy.h"

#endif


#endif // __AGS_EE_UTIL__MUTEX_H
