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
// Platform-independent Directory functions
//
//=============================================================================
#ifndef __AGS_CN_UTIL__DIRECTORY_H
#define __AGS_CN_UTIL__DIRECTORY_H

#include <memory>
#include <regex>
#include <stack>
#include <vector>
#include "core/platform.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

namespace Directory
{
    // Creates new directory (if it does not exist)
    bool   CreateDirectory(const String &path);
    // Makes sure all the sub-directories in the path are created. Parent path is
    // not touched, and function must fail if parent path is not accessible.
    bool   CreateAllDirectories(const String &parent, const String &sub_dirs);
    // Sets current working directory, returns the resulting path
    String SetCurrentDirectory(const String &path);
    // Gets current working directory
    String GetCurrentDirectory();

    // Get list of subdirs found in the given directory
    bool   GetDirs(const String &dir_path, std::vector<String> &dirs);
    // Get list of files found in the given directory
    bool   GetFiles(const String &dir_path, std::vector<String> &files);
} // namespace Directory


class FindFile
{
public:
    FindFile() = default;
    FindFile(FindFile &&ff);
    ~FindFile();

    static FindFile OpenFiles(const String &path, const String &wildcard = "*")
        { return Open(path, wildcard, true, false); }
    static FindFile OpenDirs(const String &path, const String &wildcard = "*")
        { return Open(path, wildcard, false, true); }
    bool AtEnd() const { return _current.IsEmpty(); }
    String Current() const { return _current; }
    time_t CurrentTime() const { return _currentTime; }
    void Close();
    bool Next();

    FindFile &operator =(FindFile &&ff);

private:
    // Internal data type, platform-dependent
    struct Internal;

    FindFile(Internal &&ffi);

    static FindFile Open(const String &path, const String &wildcard,
                         bool do_file, bool do_dir);

    std::unique_ptr<Internal> _i;
    String _current;
    time_t _currentTime{};
};


class FindFileRecursive
{
public:
    FindFileRecursive() = default;
    FindFileRecursive(FindFileRecursive &&ff) = default;
    ~FindFileRecursive() = default;

    static FindFileRecursive Open(const String &path, const String &wildcard = "*",
                                  size_t max_level = SIZE_MAX);
    // TODO: directory mode, like in FindFile
    bool AtEnd() const { return _ffile.AtEnd(); }
    String Current() const { return _curFile; }
    void Close();
    bool Next();

    FindFileRecursive &operator =(FindFileRecursive &&ff) = default;

private:
    bool PushDir(const String &sub);
    bool PopDir();

    std::stack<FindFile> _fdirs;
    FindFile _fdir; // current find dir iterator
    FindFile _ffile; // current find file iterator
    // max nesting level, SIZE_MAX for unrestricted
    size_t _maxLevel = SIZE_MAX;
    String _fullDir; // full directory path
    String _curDir; // current dir path, relative to the base path
    String _curFile; // current file path with parent dirs
};


} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__DIRECTORY_H
