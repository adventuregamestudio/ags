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
#include "util/string_utils.h"

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
    DirectoryIterator &operator =(DirectoryIterator &&di);

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

//
// DirectoryRecursiveIterator iterates entries in the directory
// and all the subdirectories. The order of iteration among subitems
// is undefined, but each subdirectory is passed in one go before
// going to others.
//
class DirectoryRecursiveIterator
{
public:
    DirectoryRecursiveIterator() = default;
    DirectoryRecursiveIterator(DirectoryRecursiveIterator &&di) = default;
    ~DirectoryRecursiveIterator() = default;

    static DirectoryRecursiveIterator Open(const String &path,
                                           size_t max_level = SIZE_MAX);

    bool AtEnd() const { return _dir.AtEnd(); }
    void Close();
    const String &Current() const { return _curFile; }
    const FileEntry &GetEntry() const { return _dir.GetEntry(); }
    bool Next();

    operator bool() const { return _dir; }
    DirectoryRecursiveIterator &operator =(DirectoryRecursiveIterator &&di) = default;

private:
    bool PushDir();
    bool PopDir();

    // A stack of directory iteration positions
    std::stack<DirectoryIterator> _dirStack;
    DirectoryIterator _dir; // current dir iterator
    DirectoryIterator _subSearch; // current subdirectories searcher
    // max nesting level, SIZE_MAX for unrestricted
    size_t _maxLevel = SIZE_MAX;
    String _fullDir; // full directory path
    String _curDir; // current dir path, relative to the base path
    String _curFile; // current file path with parent dirs
};


//
// FindFile searches the directory for files or subdirs,
// using defined sort of directory iteration (flat or recursive), and a match pattern.
// TODO: accept regex in ctor.
// TODO: move to its own header? with or w/o DirIterator?
//
class FindFile
{
public:
    FindFile() = default;
    FindFile(FindFile &&ff) = default;
    ~FindFile() = default;

    // Open FindFile with the given search parameters
    static FindFile Open(const String &path, const String &wildcard,
        bool do_files, bool do_dirs, size_t max_level = SIZE_MAX);
    // Search for files, strictly in the given dir
    static FindFile OpenFiles(const String &path, const String &wildcard = "*")
        { return Open(path, wildcard, true, false, 0); }
    // Search for directories, strictly in the given dir
    static FindFile OpenDirs(const String &path, const String &wildcard = "*")
        { return Open(path, wildcard, false, true, 0); }
    // Search for files, recursively, in the given dir, and all nested subdirs
    static FindFile OpenFilesRecursive(const String &path, const String &wildcard = "*")
        { return Open(path, wildcard, true, false, SIZE_MAX); }
    // Search for directories, recursively, in the given dir, and all nested subdirs
    static FindFile OpenDirsRecursive(const String &path, const String &wildcard = "*")
        { return Open(path, wildcard, false, true, SIZE_MAX); }

    bool AtEnd() const { return _di.AtEnd(); }
    void Close() { _di = DirectoryRecursiveIterator(); }
    const String &Current() const { return _di.Current(); }
    const FileEntry &GetEntry() const { return _di.GetEntry(); }
    bool Next();

    operator bool() const { return _di; }
    FindFile &operator =(FindFile &&ff) = default;

private:
    FindFile(DirectoryRecursiveIterator &&di, std::regex &&regex, bool files, bool dirs)
        : _di(std::move(di)), _regex(std::move(regex))
        , _doFiles(files), _doDirs(dirs) {}

    DirectoryRecursiveIterator _di;
    bool Test();
    std::regex _regex; // match pattern
    // TODO: make flags instead?
    bool _doFiles = false;
    bool _doDirs = false;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__DIRECTORY_H
