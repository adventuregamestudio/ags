// AGS Editor ImGui - Path utility functions
// Provides cross-platform path normalization and relative path helpers.
#pragma once

#include <string>
#include <algorithm>
#include <filesystem>

namespace AGSEditor
{

// Normalize a file path for the current platform:
// - Replaces backslashes with forward slashes on non-Windows platforms
// - This is needed because AGS project files (AGF) store paths with
//   Windows-style backslashes (e.g. "AudioCache\au000001.ogg")
inline std::string NormalizePath(const std::string& path)
{
    std::string result = path;
#ifndef _WIN32
    std::replace(result.begin(), result.end(), '\\', '/');
#endif
    return result;
}

// Make a path relative to a base directory.
// If the path is already relative, returns it as-is.
// If the path starts with the base directory, strips the base prefix.
// All returned paths use forward slashes.
inline std::string MakeRelativePath(const std::string& path, const std::string& base_dir)
{
    namespace fs = std::filesystem;
    std::string norm_path = NormalizePath(path);
    std::string norm_base = NormalizePath(base_dir);

    // Ensure base_dir ends with a separator
    if (!norm_base.empty() && norm_base.back() != '/')
        norm_base += '/';

    // If path starts with the base dir, strip it
    if (norm_path.size() > norm_base.size() &&
        norm_path.substr(0, norm_base.size()) == norm_base)
    {
        return norm_path.substr(norm_base.size());
    }

    // Try using std::filesystem for more robust relative path resolution
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

// Resolve a potentially-relative path against a base directory.
// Returns a normalized absolute path.
inline std::string ResolvePath(const std::string& path, const std::string& base_dir)
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

} // namespace AGSEditor
