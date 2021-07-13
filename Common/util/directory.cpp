#include "util/directory.h"
#include <errno.h>
#include <string.h>
#if AGS_PLATFORM_OS_WINDOWS
#include <direct.h>
#include <windows.h>
#undef CreateFile
#undef DeleteFile
#undef CreateDirectory
#undef GetCurrentDirectory
#undef SetCurrentDirectory
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif
#include "util/path.h"
#include "util/stdio_compat.h"


namespace AGS
{
namespace Common
{

namespace Directory
{

bool CreateDirectory(const String &path)
{
    return mkdir(path.GetCStr()
#if ! AGS_PLATFORM_OS_WINDOWS
        , 0755
#endif
        ) == 0 || errno == EEXIST;
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
    chdir(path.GetCStr());
    return GetCurrentDirectory();
}

String GetCurrentDirectory()
{
    char buf[512];
    getcwd(buf, 512);
    String str(buf);
    Path::FixupPath(str);
    return str;
}

#if ! AGS_PLATFORM_OS_WINDOWS
bool GetFilesImpl(const String &dir_path, std::vector<String> &files,
    int is_reg, int is_dir)
{
    struct dirent *ent;
    DIR *dir = opendir(dir_path.GetCStr());
    if (!dir)
        return false;
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
    char pattern[MAX_PATH];
    snprintf(pattern, MAX_PATH, "%s/%s", dir_path.GetCStr(), "*");
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(pattern, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
        return false;
    do
    {
        if (strcmp(findData.cFileName, ".") == 0 ||
            strcmp(findData.cFileName, "..") == 0) continue;
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == attr_dir)
            files.push_back(findData.cFileName);
    } while (FindNextFileA(hFind, &findData) != 0);
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

} // namespace Common
} // namespace AGS
