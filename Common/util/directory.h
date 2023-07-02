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
    void   GetDirs(const String &dir_path, std::vector<String> &dirs);
    // Get list of files found in the given directory
    void   GetFiles(const String &dir_path, std::vector<String> &files);
} // namespace Directory


// FileEntry describes a single entry in the filesystem.
struct FileEntry
{
    String Name;
    // TODO: make flags instead?
    bool IsFile = false;
    bool IsDir = false;
    time_t Time{};

    FileEntry() = default;
    FileEntry(const String &name, bool is_file, bool is_dir, const time_t &time)
        : Name(name), IsFile(is_file), IsDir(is_dir), Time(time) {}

    operator bool() const { return !Name.IsEmpty(); }
};

//
// DirectoryIterator iterates entries in the directory.
// The order of iteration is undefined.
//
class DirectoryIterator
{
public:
    DirectoryIterator();
    DirectoryIterator(DirectoryIterator &&di);
    ~DirectoryIterator();

    static DirectoryIterator Open(const String &path);

    bool AtEnd() const { return _current.IsEmpty(); }
    void Close();
    const String &Current() const { return _current; }
    const FileEntry &GetEntry() const;
    bool Next();

    operator bool() const { return _i != nullptr; }
    DirectoryIterator &operator =(DirectoryIterator &&di) = default;

private:
    // Internal data type, platform-dependent
    struct Internal;

    DirectoryIterator(const String &path, Internal &&diint);

    std::unique_ptr<Internal> _i;
    String _dirPath;
    String _current;
    mutable String _buf; // for storing absolute entry path, if needed in impl
    mutable FileEntry _fileEntry; // cached entry stats
};


class FindFile
{
public:
    FindFile() = default;
    FindFile(FindFile &&ff) = default;
    ~FindFile() = default;

    static FindFile OpenFiles(const String &path, const String &wildcard = "*")
        { return Open(path, wildcard, true, false); }
    static FindFile OpenDirs(const String &path, const String &wildcard = "*")
        { return Open(path, wildcard, false, true); }

    bool AtEnd() const { return _di.AtEnd(); }
    void Close();
    const String &Current() const { return _di.Current(); }
    const FileEntry &GetEntry() const { return _di.GetEntry(); }
    bool Next();

    operator bool() const { return _di; }
    FindFile &operator =(FindFile &&ff) = default;

private:
    FindFile(DirectoryIterator &&di, std::regex &&regex, bool files, bool dirs)
        : _di(std::move(di)), _regex(std::move(regex))
        , _doFiles(files), _doDirs(dirs) {}

    static FindFile Open(const String &path, const String &wildcard,
                         bool do_files, bool do_dirs);
    bool Test();

    DirectoryIterator _di;
    std::regex _regex; // match pattern
    // TODO: make flags instead?
    bool _doFiles = false;
    bool _doDirs = false;
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
