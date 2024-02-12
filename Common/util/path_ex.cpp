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
#include "util/path.h"
#include "allegro/file.h"
#include "util/stdio_compat.h"

namespace AGS
{
namespace Common
{

namespace Path
{

int ComparePaths(const String &path1, const String &path2)
{
    // Make minimal absolute paths
    String fixed_path1 = MakeAbsolutePath(path1);
    String fixed_path2 = MakeAbsolutePath(path2);

    fixed_path1.TrimRight('/');
    fixed_path2.TrimRight('/');

    int cmp_result =
#if defined AGS_CASE_SENSITIVE_FILESYSTEM
        fixed_path1.Compare(fixed_path2);
#else
        fixed_path1.CompareNoCase(fixed_path2);
#endif // AGS_CASE_SENSITIVE_FILESYSTEM
    return cmp_result;
}

bool IsSameOrSubDir(const String &parent, const String &path)
{
    char can_parent[MAX_PATH_SZ];
    char can_path[MAX_PATH_SZ];
    char relative[MAX_PATH_SZ];
    // canonicalize_filename treats "." as "./." (file in working dir)
    const char *use_parent = parent == "." ? "./" : parent.GetCStr();
    const char *use_path   = path   == "." ? "./" : path.GetCStr();
    canonicalize_filename(can_parent, use_parent, MAX_PATH_SZ);
    canonicalize_filename(can_path, use_path, MAX_PATH_SZ);
    const char *pstr = make_relative_filename(relative, can_parent, can_path, MAX_PATH_SZ);
    if (!pstr)
        return false;
    for (pstr = strstr(pstr, ".."); pstr && *pstr; pstr = strstr(pstr, ".."))
    {
        pstr += 2;
        if (*pstr == '/' || *pstr == '\\' || *pstr == 0)
            return false;
    }
    return true;
}

String MakeAbsolutePath(const String &path)
{
    if (path.IsEmpty())
    {
        return "";
    }
    // canonicalize_filename treats "." as "./." (file in working dir)
    String abs_path = path == "." ? "./" : path;
    char buf[MAX_PATH_SZ];
    canonicalize_filename(buf, abs_path.GetCStr(), MAX_PATH_SZ);
    abs_path = buf;
    FixupPath(abs_path);
    return abs_path;
}

String MakeRelativePath(const String &base, const String &path)
{
    char can_parent[MAX_PATH_SZ];
    char can_path[MAX_PATH_SZ];
    char relative[MAX_PATH_SZ];
    // canonicalize_filename treats "." as "./." (file in working dir)
    const char *use_parent = base == "." ? "./" : base.GetCStr();
    const char *use_path = path == "." ? "./" : path.GetCStr(); // FIXME?
    canonicalize_filename(can_parent, use_parent, MAX_PATH_SZ);
    canonicalize_filename(can_path, use_path, MAX_PATH_SZ);
    String rel_path = make_relative_filename(relative, can_parent, can_path, MAX_PATH_SZ);
    FixupPath(rel_path);
    return rel_path;
}

} // namespace Path

} // namespace Common
} // namespace AGS
