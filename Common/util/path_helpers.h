//=============================================================================
//
// Adventure Game Studio (AGS)
//
// std::string path helpers built on Common/util/path.h
//
//=============================================================================
#ifndef __AGS_CN_UTIL__PATH_HELPERS_H
#define __AGS_CN_UTIL__PATH_HELPERS_H

#include <algorithm>
#include <filesystem>
#include <string>

#include "util/path.h"

namespace AGS
{
namespace Common
{

inline std::string NormalizePath(const std::string &path)
{
    String result(path.c_str());
    Path::FixupPath(result);
    return result.GetCStr();
}

inline std::string MakeRelativePath(const std::string &path, const std::string &base_dir)
{
    namespace fs = std::filesystem;
    std::string norm_path = NormalizePath(path);
    std::string norm_base = NormalizePath(base_dir);

    if (!norm_base.empty() && norm_base.back() != '/')
        norm_base += '/';

    if (norm_path.size() > norm_base.size() &&
        norm_path.substr(0, norm_base.size()) == norm_base)
    {
        return norm_path.substr(norm_base.size());
    }

    try
    {
        fs::path rel = fs::relative(fs::path(norm_path), fs::path(NormalizePath(base_dir)));
        std::string result = rel.string();
#ifndef _WIN32
        std::replace(result.begin(), result.end(), '\\', '/');
#endif
        return result;
    }
    catch (...)
    {
        return norm_path;
    }
}

inline std::string ResolvePath(const std::string &path, const std::string &base_dir)
{
    namespace fs = std::filesystem;
    std::string norm = NormalizePath(path);
    fs::path p(norm);
    if (p.is_absolute())
        return norm;
    std::string result = (fs::path(NormalizePath(base_dir)) / p).string();
#ifndef _WIN32
    std::replace(result.begin(), result.end(), '\\', '/');
#endif
    return result;
}

inline std::string ResolveToAbsolutePath(const std::string &path)
{
    if (path.empty())
        return path;

    String abs = Path::MakeAbsolutePath(NormalizePath(path).c_str());
    return abs.GetCStr();
}

inline std::string ResolveToAbsolutePath(const std::string &path, const std::string &base_dir)
{
    namespace fs = std::filesystem;
    if (path.empty())
        return path;

    std::string norm = NormalizePath(path);
    fs::path p(norm);
    if (p.is_absolute())
        return norm;

    if (base_dir.empty())
        return ResolveToAbsolutePath(path);

    std::error_code ec;
    p = fs::absolute(fs::path(NormalizePath(base_dir)) / p, ec);
    if (ec)
        return ResolvePath(path, base_dir);

    std::string result = p.string();
#ifndef _WIN32
    std::replace(result.begin(), result.end(), '\\', '/');
#endif
    return result;
}

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__PATH_HELPERS_H
