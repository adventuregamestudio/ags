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
#ifndef __AGS_EE_UTIL__LIBRARY_WINDOWS_H
#define __AGS_EE_UTIL__LIBRARY_WINDOWS_H

#include "debug/out.h"
#include "platform/windows/winapi_exclusive.h"
#include "util/path.h"
#include "util/stdio_compat.h"
#include "util/string.h"

// Because this class may be exposed to generic code in sake of inlining,
// we should avoid including <windows.h> full of macros with common names.
#ifdef __cplusplus
extern "C" {
#endif

    WINBASEAPI BOOL WINAPI FreeLibrary(HMODULE hLibModule);
    WINBASEAPI FARPROC WINAPI GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
    WINBASEAPI HMODULE WINAPI LoadLibraryW(LPCWSTR lpLibFileName);

#ifdef __cplusplus
} // extern "C"
#endif


namespace AGS
{
namespace Engine
{

using AGS::Common::String;

class WindowsLibrary : public BaseLibrary
{
public:
    WindowsLibrary() = default;
    ~WindowsLibrary() override
    {
        Unload();
    };

    String GetFilenameForLib(const String &libname) override
    {
        return String::FromFormat("%s.dll", libname.GetCStr());
    }

    bool Load(const String &libname) override
    {
        Unload();
        String path;
        HMODULE lib = TryLoad(libname, path);
        if (lib == NULL)
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
            FreeLibrary(_library);
            _library = NULL;
            _name = "";
            _path = "";
        }
    }

    virtual bool IsLoaded() const override
    {
        return _library != NULL;
    }

    void *GetFunctionAddress(const String &fn_name) override
    {
        if (!_library)
            return nullptr;
        return GetProcAddress(_library, fn_name.GetCStr());
    }

private:
    HMODULE TryLoad(const String &libname, String &path)
    {
        path = GetFilenameForLib(libname);
        AGS::Common::Debug::Printf("Built library path: %s", path.GetCStr());
        WCHAR wpath[MAX_PATH_SZ];
        MultiByteToWideChar(CP_UTF8, 0, path.GetCStr(), -1, wpath, MAX_PATH_SZ);
        return LoadLibraryW(wpath);
    }

    HMODULE _library = NULL;
};


typedef WindowsLibrary Library;


} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_UTIL__LIBRARY_WINDOWS_H
