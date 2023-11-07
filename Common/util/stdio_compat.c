//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
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
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
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

int ags_file_remove(const char *path)
{
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wstr, MAX_PATH_SZ);
    return _wremove(wstr);
#else
    return remove(path);
#endif
}

int ags_file_rename(const char *src, const char *dst)
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

int ags_file_copy(const char *src, const char *dst, int overwrite)
{
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wsrc[MAX_PATH_SZ], wdst[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, src, -1, wsrc, MAX_PATH_SZ);
    MultiByteToWideChar(CP_UTF8, 0, dst, -1, wdst, MAX_PATH_SZ);
    return !CopyFileW(wsrc, wdst, !overwrite); // inverse CopyFile's result to match 0 = success
#else
    int fd_src, fd_dst;
    int dst_flags;
    char buf[4096]; // CHECKME: larger buffer? malloc a bigger one on heap?
    size_t read_num;

    fd_src = open(src, O_RDONLY);
    if (fd_src < 0)
        return -1;
    dst_flags = O_WRONLY | O_CREAT;
    if (!overwrite)
        dst_flags |= O_EXCL;
    fd_dst = open(dst, dst_flags, 0666);
    if (fd_dst < 0) {
        close(fd_src);
        return -1;
    }

    while ((read_num = read(fd_src, buf, sizeof(buf))) > 0) {
        char *out_ptr = buf;
        size_t wrote_num;
        do {
            wrote_num = write(fd_dst, out_ptr, read_num);
            if (wrote_num >= 0) {
                read_num -= wrote_num;
                out_ptr += wrote_num;
            } else if (errno != EINTR) {
                close(fd_src);
                close(fd_dst);
                return -1;
            }
        } while (read_num > 0);
    }

    close(fd_src);
    if (close(fd_dst) < 0)
        return -1;
    // At this point read_num contains either 0 or -1 as error
    return read_num;
#endif // POSIX
}
