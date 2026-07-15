// Thin wrappers around AGS Common file/path utilities for agsbuild.
#pragma once

#include <ctime>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "path_utils.h"

#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stream.h"

namespace AGSBuild
{

inline AGS::Common::String ToCommonPath(const std::string &path)
{
    return AGS::Common::String(path.c_str());
}

inline std::string ToStdPath(const AGS::Common::String &path)
{
    return path.GetCStr();
}

inline bool PathExists(const std::string &path)
{
    return AGS::Common::File::IsFileOrDir(ToCommonPath(path));
}

inline bool PathIsFile(const std::string &path)
{
    return AGS::Common::File::IsFile(ToCommonPath(path));
}

inline bool PathIsDirectory(const std::string &path)
{
    return AGS::Common::File::IsDirectory(ToCommonPath(path));
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
    return AGS::Common::File::CopyFile(ToCommonPath(src), ToCommonPath(dst), overwrite);
}

inline bool DoesFileNeedRecompile(const std::string &source_file, const std::string &dest_file)
{
    if (!PathIsFile(source_file))
        return false;
    if (!PathIsFile(dest_file))
        return true;

    const time_t src_time = AGS::Common::File::GetFileTime(ToCommonPath(source_file));
    const time_t dst_time = AGS::Common::File::GetFileTime(ToCommonPath(dest_file));
    if (src_time == static_cast<time_t>(-1) || dst_time == static_cast<time_t>(-1))
        return true;
    return src_time >= dst_time;
}

inline std::string FindFileCaseInsensitive(const std::string &dir, const std::string &filename)
{
    if (dir.empty())
        return ResolveToAbsolutePath(filename);

    const AGS::Common::String found =
        AGS::Common::File::FindFileCI(ToCommonPath(dir), ToCommonPath(filename));
    if (!found.IsEmpty())
        return found.GetCStr();

    return dir + "/" + filename;
}

inline bool ReadTextFile(const std::string &path, std::string &out)
{
    std::unique_ptr<AGS::Common::Stream> in(
        AGS::Common::File::OpenFileRead(ToCommonPath(path)));
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
    for (AGS::Common::FindFile ff = AGS::Common::FindFile::OpenFiles(ToCommonPath(dir));
         !ff.AtEnd(); ff.Next())
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
    for (AGS::Common::FindFile ff = AGS::Common::FindFile::OpenFiles(ToCommonPath(dir));
         !ff.AtEnd(); ff.Next())
    {
        const std::string full_path = AGS::Common::Path::ConcatPaths(
            ToCommonPath(dir), ff.Current()).GetCStr();
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

} // namespace AGSBuild
