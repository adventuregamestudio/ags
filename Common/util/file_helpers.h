//=============================================================================
//
// Adventure Game Studio (AGS)
//
// std::string file I/O helpers built on Common/util/file.h
//
//=============================================================================
#ifndef __AGS_CN_UTIL__FILE_HELPERS_H
#define __AGS_CN_UTIL__FILE_HELPERS_H

#include <ctime>
#include <cctype>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/path_helpers.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

inline String ToCommonPath(const std::string &path)
{
    return String(path.c_str());
}

inline std::string ToStdPath(const String &path)
{
    return path.GetCStr();
}

inline bool PathExists(const std::string &path)
{
    return File::IsFileOrDir(ToCommonPath(path));
}

inline bool PathIsFile(const std::string &path)
{
    return File::IsFile(ToCommonPath(path));
}

inline bool PathIsDirectory(const std::string &path)
{
    return File::IsDirectory(ToCommonPath(path));
}

inline bool EnsureDirectory(const std::string &path)
{
    if (path.empty())
        return false;
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    return !ec;
}

inline bool CopyPath(const std::string &src, const std::string &dst, bool overwrite = true)
{
    return File::CopyFile(ToCommonPath(src), ToCommonPath(dst), overwrite);
}

inline bool DoesFileNeedRecompile(const std::string &source_file, const std::string &dest_file)
{
    if (!PathIsFile(source_file))
        return false;
    if (!PathIsFile(dest_file))
        return true;

    const time_t src_time = File::GetFileTime(ToCommonPath(source_file));
    const time_t dst_time = File::GetFileTime(ToCommonPath(dest_file));
    if (src_time == static_cast<time_t>(-1) || dst_time == static_cast<time_t>(-1))
        return true;
    return src_time >= dst_time;
}

inline std::string FindFileCaseInsensitive(const std::string &dir, const std::string &filename)
{
    if (dir.empty())
        return ResolveToAbsolutePath(filename);

    const String found = File::FindFileCI(ToCommonPath(dir), ToCommonPath(filename));
    if (!found.IsEmpty())
        return found.GetCStr();

    return dir + "/" + filename;
}

inline bool ReadTextFile(const std::string &path, std::string &out)
{
    std::unique_ptr<Stream> in(File::OpenFileRead(ToCommonPath(path)));
    if (!in)
        return false;

    out.clear();
    out.reserve(static_cast<size_t>(in->GetLength()));
    char buf[4096];
    while (true)
    {
        const size_t read = in->Read(buf, sizeof(buf));
        if (read == 0)
            break;
        out.append(buf, read);
    }
    return true;
}

inline void CollectDirectoryFileNames(
    const std::string &dir,
    const std::function<bool(const std::string &)> &matcher,
    std::vector<std::string> &out_names)
{
    for (FindFile ff = FindFile::OpenFiles(ToCommonPath(dir)); !ff.AtEnd(); ff.Next())
    {
        const std::string fname = ff.Current().GetCStr();
        if (matcher(fname))
            out_names.push_back(fname);
    }
}

inline void CollectFilesByExtension(
    const std::string &dir,
    const std::vector<std::string> &extensions,
    std::vector<std::string> &out_files)
{
    for (FindFile ff = FindFile::OpenFiles(ToCommonPath(dir)); !ff.AtEnd(); ff.Next())
    {
        const std::string full_path =
            Path::ConcatPaths(ToCommonPath(dir), ff.Current()).GetCStr();
        const std::string fname = ff.Current().GetCStr();
        size_t dot = fname.rfind('.');
        if (dot == std::string::npos)
            continue;

        std::string ext = fname.substr(dot);
        for (char &c : ext)
            c = static_cast<char>(tolower(c));

        for (const auto &wanted : extensions)
        {
            if (ext == wanted)
            {
                out_files.push_back(full_path);
                break;
            }
        }
    }
}

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__FILE_HELPERS_H
