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
// Platform-independent Path functions
//
//=============================================================================
#ifndef __AGS_CN_UTIL__PATH_H
#define __AGS_CN_UTIL__PATH_H

#include "util/string.h"

#if AGS_PLATFORM_OS_WINDOWS
#define PATH_ALT_SEPARATOR      ('\\')
#define PATH_DEVICE_SEPARATOR   (':')
#else
#define PATH_ALT_SEPARATOR      ('/')
#define PATH_DEVICE_SEPARATOR   ('\0')
#endif

namespace AGS
{
namespace Common
{

namespace Path
{
    // Returns parent directory of the given path;
    // returns "." (current dir) if the path does not contain a parent segment
    String  GetParent(const String &path);
    // Returns filename part out of the longer path
    String  GetFilename(const String &path);
    // Returns file's extension; file may be a fully qualified path too
    String  GetFileExtension(const String &path);
    // Returns part of the filename without extension
    String  RemoveExtension(const String &filename);
    // Returns filename with a different extension
    String  ReplaceExtension(const String &filename, const String &ext);

    // Makes a platform-dependant path comparison.
    // This takes into consideration platform's filename case (in)sensivity and
    // DOS-compatible 8.3 filenames;
    // The result value corresponds to stdlib strcmp function.
    int     ComparePaths(const String &path1, const String &path2);

    // Returns path to the actual directory, referenced by given path;
    // if path is a directory, returns path unchanged, if path is a file, returns
    // parent directory containing that file.
    // FIXME: this function is misleading, and does a "directory exists" check,
    // which is not always wanted (and may be not suitable for performance reasons);
    // Path namespace must not rely on actual fs entries existance,
    // check all this function uses and replace the solution, get rid of this func!
    String  GetDirectoryPath(const String &path);
    // Tells if the path points to the parent path's location or lower directory;
    // return FALSE if the path points to outside of the parent location.
    bool    IsSameOrSubDir(const String &parent, const String &path);
    // Tells if the path is relative.
    bool    IsRelativePath(const String &path);

    // Makes a path have only '/' slashes; this is to make it easier to work
    // with path, knowing it contains only one type of directory separators
    void    FixupPath(String &path);
    // Fixups path and removes trailing slash
    String  MakePathNoSlash(const String &path);
    // Fixups path and adds trailing slash if it's missing
    String  MakeTrailingSlash(const String &path);
    // Converts any path to an absolute path; relative paths are assumed to
    // refer to the current working directory.
    String  MakeAbsolutePath(const String &path);
    // Tries to create a relative path that would point to 'path' location
    // if walking out of the 'base'. Returns empty string on failure.
    // NOTE: the 'base' is only considered a directory if it has a trailing slash.
    String  MakeRelativePath(const String &base, const String &path);
    // Creates path by combining directory, file name and extension
    String  MakePath(const String &parent, const String &filename, const String &ext);
    // Appends another section to existing path
    String &AppendPath(String &path, const String &child);
    // Concatenates parent and relative paths
    String  ConcatPaths(const String &parent, const String &child);
    // Concatenates paths into the buffer, returns the buffer;
    // warning: passing buffer as one of the path params results in UB
    String  ConcatPaths(String &buf, const String &parent, const String &child);
    // Splits path into components, divided by path separator
    std::vector<String> Split(const String &path);

    // Subsitutes illegal characters with '_'. This function uses a combined set
    // of illegal chars from all the supported platforms to make a name that
    // could be copied across systems without problems.
    String  FixupSharedFilename(const String &filename);

#if AGS_PLATFORM_OS_ANDROID
    // gets a path stripped of directory roots
    String GetPathInForeignAsset(const String &filename);
#endif

    // NOTE: these are only required for internal util implementations
#if AGS_PLATFORM_OS_WINDOWS
    // Converts system wide-char path into a UTF-8 string
    String  WidePathToUTF8(const wchar_t *ws);
#endif
} // namespace Path

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__PATH_H
