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
//
// Platform-independent Path functions
//
//=============================================================================
#ifndef __AGS_CN_UTIL__PATH_H
#define __AGS_CN_UTIL__PATH_H

#include "util/string.h"

#if AGS_PLATFORM_OS_WINDOWS
#define PATH_ALT_SEPARATOR ('\\')
#else
#define PATH_ALT_SEPARATOR ('/')
#endif

namespace AGS
{
namespace Common
{

namespace Path
{
    // Tells if the given path is a directory
    bool    IsDirectory(const String &directory);
    // Tells if the given path is a file
    bool    IsFile(const String &filename);
    // Tells if the given path is file or directory;
    // may be used to check if it's valid to use
    bool    IsFileOrDir(const String &filename);
    // Returns parent directory of the given path;
    // returns "." (current dir) if the path does not contain a parent segment
    String  GetParent(const String &path);
    // Returns filename part out of the longer path
    String  GetFilename(const String &path);
    // Returns file's extension; file may be a fully qualified path too
    String  GetFileExtension(const String &path);
    // Returns part of the filename without extension
    String  RemoveExtension(const String &filename);

    // Makes a platform-dependant path comparison.
    // This takes into consideration platform's filename case (in)sensivity and
    // DOS-compatible 8.3 filenames;
    // The result value corresponds to stdlib strcmp function.
    int     ComparePaths(const String &path1, const String &path2);

    // Returns path to the actual directory, referenced by given path;
    // if path is a directory, returns path unchanged, if path is a file, returns
    // parent directory containing that file.
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
    // Concatenates parent and relative paths
    String  ConcatPaths(const String &parent, const String &child);
    // Concatenates paths into the buffer, returns the buffer;
    // warning: passing buffer as one of the path params results in UB
    String  ConcatPaths(String &buf, const String &parent, const String &child);
    // Creates path by combining directory, file name and extension
    String  MakePath(const String &parent, const String &filename, const String &ext);
    // Splits path into components, divided by path separator
    std::vector<String> Split(const String &path);

    // Subsitutes illegal characters with '_'. This function uses a combined set
    // of illegal chars from all the supported platforms to make a name that
    // could be copied across systems without problems.
    String  FixupSharedFilename(const String &filename);

    // Converts filepath into ASCII variant; returns empty string on failure
    String  GetPathInASCII(const String &path);
    // Converts filepath from command line's argument into ASCII variant
    String  GetCmdLinePathInASCII(const char *arg, int arg_index);
} // namespace Path

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__PATH_H
