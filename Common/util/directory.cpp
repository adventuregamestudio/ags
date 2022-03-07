#include "util/directory.h"
#include <errno.h>
#include <string.h>
#if AGS_PLATFORM_OS_WINDOWS
#include <direct.h>
#include "platform/windows/windows.h"
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif
#if AGS_PLATFORM_OS_ANDROID
#include "util/android_file.h"
#endif
#include "util/path.h"
#include "util/stdio_compat.h"
#include "util/string_utils.h"


namespace AGS
{
namespace Common
{

using namespace Path;

namespace Directory
{

bool CreateDirectory(const String &path)
{
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, path.GetCStr(), -1, wstr, MAX_PATH_SZ);
    return (CreateDirectoryW(wstr, NULL) != FALSE) || (GetLastError() == ERROR_ALREADY_EXISTS);
#else
    return (mkdir(path.GetCStr(), 0755) == 0) || (errno == EEXIST);
#endif
}

bool CreateAllDirectories(const String &parent, const String &sub_dirs)
{
    if (parent.IsEmpty() || !ags_directory_exists(parent.GetCStr()))
        return false; // no sense, or base dir not exist
    if (sub_dirs.IsEmpty())
        return true; // nothing to create, so fine

    String make_path = String::FromFormat("%s/", parent.GetCStr());
    for (const char *sect = sub_dirs.GetCStr();
        sect < sub_dirs.GetCStr() + sub_dirs.GetLength();)
    {
        const char *cur = sect + 1;
        for (; *cur && *cur != '/' && *cur != PATH_ALT_SEPARATOR; ++cur);
        // Skip empty dirs (duplicated separators etc)
        if ((cur - sect == 1) && (*cur == '.' || *cur == '/' || *cur == PATH_ALT_SEPARATOR))
        {
            sect = cur;
            continue;
        }
        // In case of ".." just fail
        if (strncmp(sect, "..", cur - sect) == 0)
            return false;
        make_path.Append(sect, cur - sect);
        if (!CreateDirectory(make_path))
            return false;
        sect = cur;
    }
    return true;
}

String SetCurrentDirectory(const String &path)
{
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, path.GetCStr(), -1, wstr, MAX_PATH_SZ);
    SetCurrentDirectoryW(wstr);
#else
    chdir(path.GetCStr());
#endif
    return GetCurrentDirectory();
}

String GetCurrentDirectory()
{
    String str;
#if AGS_PLATFORM_OS_WINDOWS
    WCHAR wstr[MAX_PATH_SZ];
    GetCurrentDirectoryW(MAX_PATH_SZ, wstr);
    str = Path::WidePathToUTF8(wstr);
#else
    char buf[MAX_PATH_SZ];
    getcwd(buf, sizeof(buf));
    str = buf;
#endif
    Path::FixupPath(str);
    return str;
}

#if ! AGS_PLATFORM_OS_WINDOWS
bool GetFilesImpl(const String &dir_path, std::vector<String> &files,
    int is_reg, int is_dir)
{
    DIR *dir = opendir(dir_path.GetCStr());
    if (!dir)
        return false;
    struct dirent *ent;
    struct stat f_stat;
    while ((ent = readdir(dir)) != nullptr)
    {
        if (strcmp(ent->d_name, ".") == 0 ||
            strcmp(ent->d_name, "..") == 0) continue;
        if (stat(ent->d_name, &f_stat) != 0) continue;
        if (S_ISREG(f_stat.st_mode) == is_reg &&
            S_ISDIR(f_stat.st_mode) == is_dir)
            files.push_back(ent->d_name);
    }
    closedir(dir);
    return true;
}
#else
bool GetFilesImpl(const String &dir_path, std::vector<String> &files,
    int attr_dir)
{
    char filename[MAX_PATH_SZ];
    wchar_t wpattern[MAX_PATH_SZ];
    snprintf(filename, sizeof(filename), "%s/%s", dir_path.GetCStr(), "*");
    StrUtil::ConvertUtf8ToWstr(filename, wpattern, sizeof(wpattern));
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(wpattern, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
        return false;
    do
    {
        StrUtil::ConvertWstrToUtf8(findData.cFileName, filename, sizeof(filename));
        if (strcmp(filename, ".") == 0 ||
            strcmp(filename, "..") == 0) continue;
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == attr_dir)
            files.push_back(filename);
    } while (FindNextFileW(hFind, &findData) != 0);
    FindClose(hFind);
    return true;
}
#endif

bool GetDirs(const String &dir_path, std::vector<String> &dirs)
{
#if ! AGS_PLATFORM_OS_WINDOWS
    return GetFilesImpl(dir_path, dirs, 0, 1);
#else
    return GetFilesImpl(dir_path, dirs, FILE_ATTRIBUTE_DIRECTORY);
#endif
}

bool GetFiles(const String &dir_path, std::vector<String> &files)
{
#if ! AGS_PLATFORM_OS_WINDOWS
    return GetFilesImpl(dir_path, files, 1, 0);
#else
    return GetFilesImpl(dir_path, files, 0);
#endif
}

} // namespace Directory


struct FindFile::Internal
{
    Internal() = default;
    Internal(Internal &&ffi)
    {
#if AGS_PLATFORM_OS_WINDOWS
        ff = ffi.ff;
        ffi.ff = nullptr;
        fdata = ffi.fdata;
#else
        dir = ffi.dir;
        ffi.dir = nullptr;
        regex = std::move(ffi.regex);
#endif
        attrFile = ffi.attrFile;
        attrDir = ffi.attrDir;
    }

#if AGS_PLATFORM_OS_WINDOWS
    HANDLE ff = nullptr;
    WIN32_FIND_DATAW fdata = {};
#else
    DIR *dir = nullptr;
    std::regex regex;
#endif
#if AGS_PLATFORM_OS_ANDROID
    std::unique_ptr<AndroidADir> aadir;
#endif
    int attrFile = 0;
    int attrDir = 0;
};


FindFile::FindFile(Internal &&ffi)
    : _i(new Internal(std::move(ffi)))
{
}

FindFile::FindFile(FindFile &&ff)
{
    *this = std::move(ff);
}

FindFile::~FindFile()
{
    Close();
}

FindFile &FindFile::operator =(FindFile &&ff)
{
    _i = std::move(ff._i);
    _current = std::move(ff._current);
    return *this;
}

FindFile FindFile::Open(const String &path, const String &wildcard, bool do_file, bool do_dir)
{
    Internal ffi;
#if AGS_PLATFORM_OS_WINDOWS
    char pattern[MAX_PATH_SZ];
    wchar_t wpattern[MAX_PATH_SZ];
    snprintf(pattern, sizeof(pattern), "%s/%s", path.GetCStr(), wildcard.GetCStr());
    StrUtil::ConvertUtf8ToWstr(pattern, wpattern, sizeof(wpattern));
    HANDLE hFind = FindFirstFileW(wpattern, &ffi.fdata);
    if (hFind == INVALID_HANDLE_VALUE)
        return FindFile(); // return invalid object
    ffi.ff = hFind;
    ffi.attrFile = do_file ? 1 : 0; // TODO
    ffi.attrDir = do_dir ? FILE_ATTRIBUTE_DIRECTORY : 0;
#else
    DIR *dir = opendir(path.GetCStr());
    if (!dir)
        return FindFile(); // return invalid object
    ffi.dir = dir;
    ffi.attrFile = do_file ? 1 : 0;
    ffi.attrDir = do_dir ? 1 : 0;
    String pattern = StrUtil::WildcardToRegex(wildcard);
    ffi.regex = std::regex(pattern.GetCStr(), std::regex_constants::icase);
#endif
#if AGS_PLATFORM_OS_ANDROID
    if (do_file) // NOTE: Android Asset API seem to not support dir list
        ffi.aadir.reset(new AndroidADir(path));
#endif
    FindFile ff(std::move(ffi));
    // Try get the first matching entry
    if (!ff.Next())
        return FindFile(); // return invalid object
    return ff; // success
}

void FindFile::Close()
{
#if AGS_PLATFORM_OS_WINDOWS
    if (_i)
        FindClose(_i->ff);
#else
    if (_i)
        closedir(_i->dir);
#endif
    _i.reset();
}

bool FindFile::Next()
{
    if (!_i)
        return false;
#if AGS_PLATFORM_OS_WINDOWS
    auto ff = _i->ff;
    auto &fdata = _i->fdata;
    const int attrDir = _i->attrDir;
    char filename[MAX_PATH_SZ];
    // We already have an entry opened at this point, so check that first;
    // if it's not valid then continue searching
    _current.Empty();
    for (; (fdata.cFileName[0] != 0) && _current.IsEmpty();
        fdata.cFileName[0] = 0, FindNextFileW(ff, &fdata) != 0)
    {
        StrUtil::ConvertWstrToUtf8(fdata.cFileName, filename, sizeof(filename));
        if (strcmp(filename, ".") == 0 ||
            strcmp(filename, "..") == 0) continue;
        if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != attrDir)
            continue;
        _current = filename;
    }
#else
    auto dir = _i->dir;
    const int is_reg = _i->attrFile;
    const int is_dir = _i->attrDir;
    struct dirent *ent;
    struct stat f_stat;
    std::cmatch mr;
    _current.Empty();
    while ((ent = readdir(dir)) != nullptr)
    {
        if (strcmp(ent->d_name, ".") == 0 ||
            strcmp(ent->d_name, "..") == 0) continue;
        if (stat(ent->d_name, &f_stat) != 0) continue;
        if (S_ISREG(f_stat.st_mode) != is_reg ||
            S_ISDIR(f_stat.st_mode) != is_dir)
            continue;
        if (!std::regex_match(ent->d_name, mr, _i->regex))
            continue;
        _current = ent->d_name;
        break;
    }
#endif
#if AGS_PLATFORM_OS_ANDROID
    if (_i->aadir && _current.IsEmpty())
        _current = _i->aadir->Next(_i->regex);
#endif
    return !_current.IsEmpty();
}


FindFileRecursive FindFileRecursive::Open(const String &path, const String &wildcard, size_t max_level)
{
    FindFile fdir = FindFile::OpenDirs(path);
    FindFile ffile = FindFile::OpenFiles(path, wildcard);
    if (ffile.AtEnd() && fdir.AtEnd())
        return FindFileRecursive(); // return invalid object
    FindFileRecursive ff;
    ff._fdir = std::move(fdir);
    ff._ffile = std::move(ffile);
    // Try get the first matching entry
    if (ff._ffile.AtEnd() && !ff.Next())
        return FindFileRecursive(); // return invalid object
    ff._maxLevel = max_level;
    ff._fullDir = path;
    ff._curFile = ff._ffile.Current();
    return ff; // success
}

void FindFileRecursive::Close()
{
    while (_fdirs.size()) _fdirs.pop();
    _fdir.Close();
    _ffile.Close();
}

bool FindFileRecursive::Next()
{
    // Look up for the next file in the current dir
    if (_ffile.Next())
    {
        Path::ConcatPaths(_curFile, _curDir, _ffile.Current());
        return true;
    }
    // No more files? Find a directory that still has
    while (_ffile.AtEnd())
    {
        // first make sure there are unchecked subdirs left in current dir
        while (_fdir.AtEnd())
        { // if not, go up, until found any, or hit the top
            if (!PopDir())
                return false; // no more directories
        }

        // Found an unchecked subdirectory/ies, try opening one
        while (!PushDir(_fdir.Current()) && !_fdir.AtEnd())
            _fdir.Next();
    }
    Path::ConcatPaths(_curFile, _curDir, _ffile.Current());
    return true; // success
}

bool FindFileRecursive::PushDir(const String &sub)
{
    if (_maxLevel != -1 && _fdirs.size() == _maxLevel)
        return false; // no more nesting allowed

    String path = Path::ConcatPaths(_fullDir, sub);
    FindFile fdir = FindFile::OpenDirs(path);
    FindFile ffile = FindFile::OpenFiles(path);
    if (ffile.AtEnd() && fdir.AtEnd())
        return false; // dir is empty, or error
    _fdirs.push(std::move(_fdir)); // save previous dir iterator
    _fdir = std::move(fdir);
    _ffile = std::move(ffile);
    _fullDir = path;
    _curDir = Path::ConcatPaths(_curDir, sub);
    return true;
}

bool FindFileRecursive::PopDir()
{
    if (_fdirs.size() == 0)
        return false; // no more parent levels
    // restore parent level
    _fdir = std::move(_fdirs.top());
    _fdirs.pop();
    _fullDir = Path::GetParent(_fullDir);
    _curDir = Path::GetParent(_curDir);
    if (_curDir.Compare(".") == 0)
        _curDir = ""; // hotfix for GetParent returning "."
    // advance dir iterator that we just recovered
    _fdir.Next();
    return true;
}

} // namespace Common
} // namespace AGS
