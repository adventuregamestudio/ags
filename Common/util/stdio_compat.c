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
#include "util/stdio_compat.h"

#include "core/platform.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/windows.h"
#include <io.h>
#include <shlwapi.h>
#endif

FILE *ags_fopen(const char *path, const char *mode)
{
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wpath[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_PATH_SZ);
    WCHAR wmode[10];
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, 10);
    return _wfopen(wpath, wmode);
#else
    return fopen(path, mode);
#endif
}

int	 ags_fseek(FILE * stream, file_off_t offset, int whence)
{
#if defined(HAVE_FSEEKO) // Contemporary POSIX libc
    return fseeko(stream, offset, whence);
#elif AGS_PLATFORM_OS_WINDOWS // MSVC
    return _fseeki64(stream, offset, whence); 
#else // No distinct interface with off_t
    return fseek(stream, offset, whence);
#endif
}

file_off_t ags_ftell(FILE * stream) 
{
    #if defined(HAVE_FSEEKO) // Contemporary POSIX libc
        return ftello(stream);
    #elif AGS_PLATFORM_OS_WINDOWS // MSVC
        return _ftelli64(stream); 
    #else // No distinct interface with off_t
        return ftell(stream);
    #endif
}

int  ags_file_exists(const char *path) 
{
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wstr, MAX_PATH_SZ);
    return PathFileExistsW(wstr) && !PathIsDirectoryW(wstr);
#else
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return 0;
    }
    return S_ISREG(path_stat.st_mode);
#endif
}

int ags_directory_exists(const char *path)
{
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wstr, MAX_PATH_SZ);
    return PathFileExistsW(wstr) && PathIsDirectoryW(wstr);
#else
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return 0;
    }
    return S_ISDIR(path_stat.st_mode);
#endif
}

int ags_path_exists(const char *path)
{
    #if AGS_PLATFORM_OS_WINDOWS
        WCHAR wstr[MAX_PATH_SZ];
        MultiByteToWideChar(CP_UTF8, 0, path, -1, wstr, MAX_PATH_SZ);
        return PathFileExistsW(wstr);
    #else
        struct stat path_stat;
        if (stat(path, &path_stat) != 0) {
            return 0;
        }
        return S_ISREG(path_stat.st_mode) || S_ISDIR(path_stat.st_mode);
    #endif
}

file_off_t ags_file_size(const char *path)
{
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wstr, MAX_PATH_SZ);
    struct _stat64 path_stat;
    if (_wstat64(wstr, &path_stat) != 0) {
        return -1;
    }
    return path_stat.st_size;
#else
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return -1;
    }
    return path_stat.st_size;
#endif
}

int ags_remove(const char *path)
{
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wstr, MAX_PATH_SZ);
    return _wremove(wstr);
#else
    return remove(path);
#endif
}

int ags_rename(const char *src, const char *dst)
{
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wsrc[MAX_PATH_SZ], wdst[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, src, -1, wsrc, MAX_PATH_SZ);
    MultiByteToWideChar(CP_UTF8, 0, dst, -1, wdst, MAX_PATH_SZ);
    return _wrename(wsrc, wdst);
#else
    return rename(src, dst);
#endif
}
