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
#include "core/platform.h"
#include "debug/out.h"
#include "util/path.h"
#include "util/string.h"

// FIXME: Replace with a unified way to get the directory which contains the engine binary
#if AGS_PLATFORM_OS_ANDROID
extern char android_app_directory[256];
#else
extern AGS::Common::String appDirectory;
#endif


namespace AGS
{
namespace Engine
{

using AGS::Common::String;

class PosixLibrary : public BaseLibrary
{
public:
    PosixLibrary() = default;
    ~PosixLibrary() override
    {
        Unload();
    };

    String GetFilenameForLib(const String &libname) override
    {
        return String::FromFormat(
#if AGS_PLATFORM_OS_MACOS
            "lib%s.dylib"
#else
            "lib%s.so"
#endif
            , libname.GetCStr());
    }

    bool Load(const String &libname) override
    {
        Unload();
        String path;
        void *lib = TryLoadAnywhere(libname, path);
        if (!lib)
            return false;
        _library = lib;
        _name = libname;
        _path = path;
        return true;
    }

    void Unload() override
    {
        if (_library)
        {
            dlclose(_library);
            _library = nullptr;
            _name = "";
            _path = "";
        }
    }

    virtual bool IsLoaded() const override
    {
        return _library != nullptr;
    }

    void *GetFunctionAddress(const String &fn_name) override
    {
        if (!_library)
            return nullptr;
        return dlsym(_library, fn_name.GetCStr());
    }

private:
    void *TryLoad(const String &path)
    {
        AGS::Common::Debug::Printf("Built library path: %s", path.GetCStr());
        void *lib = dlopen(path.GetCStr(), RTLD_LAZY);
        if (!lib)
            AGS::Common::Debug::Printf("dlopen returned: %s", dlerror());
        return lib;
    }

    // TODO: Perhaps move this out of the Library class? testing for engine dir
    // should not be a part of utility class imo
    void *TryLoadAnywhere(const String &libname, String &path)
    {
        String libfile = GetFilenameForLib(libname);
        // Try rpath first
        path = libfile;
        void *lib = TryLoad(path);
        if (lib)
            return lib;

        // Try current path
        path = AGS::Common::Path::ConcatPaths(".", libfile);
        lib = TryLoad(path);
        if (lib)
            return lib;

        // Try the engine directory
#if AGS_PLATFORM_OS_ANDROID
        char buffer[200];
        snprintf(buffer, sizeof(buffer), "%s%s", android_app_directory, "/lib");
        path = AGS::Common::Path::ConcatPaths(buffer, libfile);
#else
        path = AGS::Common::Path::ConcatPaths(appDirectory, libfile);
#endif
        return TryLoad(path);
    }

    void *_library = nullptr;
};


typedef PosixLibrary Library;



} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_UTIL__LIBRARY_POSIX_H
