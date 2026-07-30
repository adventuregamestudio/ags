//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "util/path.h"
#include <allegro/file.h>
#include "platform/platform.h"
#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/windows.h"
#endif
#include "util/file.h"
#include "util/stdio_compat.h"

namespace AGS
{
namespace Common
{

namespace Path
{

String GetParent(const String &path)
{
    const char *cstr = path.GetCStr();
    const char *ptr_end = cstr + path.GetLength();
    for (const char *ptr = ptr_end; ptr >= cstr; --ptr)
    {
        if (*ptr == '/' || *ptr == PATH_ALT_SEPARATOR)
            return String(cstr, ptr - cstr);
    }
    return ".";
}

String GetFilename(const String &path)
{
    const char *cstr = path.GetCStr();
    const char *ptr_end = cstr + path.GetLength();
    for (const char *ptr = ptr_end; ptr >= cstr; --ptr)
    {
        if (*ptr == '/' || *ptr == PATH_ALT_SEPARATOR)
            return String(ptr + 1);
    }
    return path;
}

String GetFileExtension(const String &path)
{
    const char *cstr = path.GetCStr();
    const char *ptr_end = cstr + path.GetLength();
    for (const char *ptr = ptr_end; ptr >= cstr; --ptr)
    {
        if (*ptr == '.') return String(ptr + 1);
        if (*ptr == '/' || *ptr == PATH_ALT_SEPARATOR) break;
    }
    return "";
}

String RemoveExtension(const String &filename)
{
    const char *cstr = filename.GetCStr();
    const char *ptr_end = cstr + filename.GetLength();
    for (const char *ptr = ptr_end; ptr >= cstr; --ptr)
    {
        if (*ptr == '.') return String(cstr, ptr - cstr);
        if (*ptr == '/' || *ptr == PATH_ALT_SEPARATOR) break;
    }
    return filename;
}

String ReplaceExtension(const String &filename, const String &ext)
{
    String noext = RemoveExtension(filename);
    return String::FromFormat("%s.%s", noext.GetCStr(), ext.GetCStr());
}

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
        fixed_path1.CompareUtf8NoCase(fixed_path2);
#endif // AGS_CASE_SENSITIVE_FILESYSTEM
    return cmp_result;
}

String GetDirectoryPath(const String &path)
{
    if (File::IsDirectory(path))
        return path;

    String dir = path;
    FixupPath(dir);
    size_t slash_at = dir.FindCharReverse('/');
    if (slash_at != String::NoIndex)
    {
        dir.ClipMid(slash_at + 1);
        return dir;
    }
    return "./";
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

bool IsRelativePath(const String &path)
{
    // Consider empty path relative
    if (path.IsEmpty())
        return true;

    // All filenames that start with a '.' are relative.
    if (path[0] == '.' )
        return true;

    // Filenames that contain a device separator (DOS/Windows)
    // or start with a '/' (Unix) are considered absolute.
#if AGS_PLATFORM_OS_WINDOWS
   if (path.FindChar(PATH_DEVICE_SEPARATOR) != String::NoIndex)
      return false;
#endif
    if ((path[0] == '/') || (path[0] == PATH_ALT_SEPARATOR))
        return false;
    return true;
}

bool IsOnlyFilename(const String &path)
{
    if (!IsRelativePath(path))
        return false;
#if AGS_PLATFORM_OS_WINDOWS
    return (path.FindChar('/') == String::NoIndex) && (path.FindChar(PATH_ALT_SEPARATOR) == String::NoIndex);
#else
    return (path.FindChar('/') == String::NoIndex);
#endif
}

void FixupPath(String &path)
{
#if AGS_PLATFORM_OS_WINDOWS
    path.Replace('\\', '/'); // bring Windows path separators to uniform style
#endif
    path.MergeSequences('/');
}

String MakePathNoSlash(const String &path)
{
    String dir_path = path;
    FixupPath(dir_path);
#if AGS_PLATFORM_OS_WINDOWS
    // if the path is 'x:/' don't strip the slash
    if (path.GetLength() == 3 && path[1u] == ':')
        ;
    else
#endif
    // if the path is '/' don't strip the slash
    if (dir_path.GetLength() > 1)
        dir_path.TrimRight('/');
    return dir_path;
}

String MakeTrailingSlash(const String &path)
{
    if (path.GetLast() == '/' || path.GetLast() == '\\')
        return path;
    String dir_path = String::FromFormat("%s/", path.GetCStr());
    FixupPath(dir_path);
    return dir_path;
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

String &AppendPath(String &path, const String &child)
{
    if (path.IsEmpty())
        path = child;
    else if (!child.IsEmpty())
        path.AppendFmt("/%s", child.GetCStr());
    FixupPath(path);
    return path;
}

String ConcatPaths(const String &parent, const String &child)
{
    if (parent.IsEmpty())
        return child;
    if (child.IsEmpty())
        return parent;
    String path = String::FromFormat("%s/%s", parent.GetCStr(), child.GetCStr());
    FixupPath(path);
    return path;
}

String ConcatPaths(String &buf, const String &parent, const String &child)
{
    if (parent.IsEmpty())
        buf = child;
    else if (child.IsEmpty())
        buf = parent;
    else
        buf.Format("%s/%s", parent.GetCStr(), child.GetCStr());
    FixupPath(buf);
    return buf;
}

String MakePath(const String &parent, const String &filename)
{
    String path = String::FromFormat("%s/%s", parent.GetCStr(), filename.GetCStr());
    FixupPath(path);
    return path;
}

String MakePath(const String &parent, const String &filename, const String &ext)
{
    String path = String::FromFormat("%s/%s.%s", parent.GetCStr(), filename.GetCStr(), ext.GetCStr());
    FixupPath(path);
    return path;
}

std::vector<String> Split(const String &path)
{
    return path.Split('/');
}

String FixupSharedFilename(const String &filename)
{
    const char *illegal_chars = "\\/:?\"<>|*";
    String fixed_name = filename;
    for (size_t i = 0; i < filename.GetLength(); ++i)
    {
        if (filename[i] < ' ')
        {
            fixed_name.SetAt(i, '_');
        }
        else
        {
            for (const char *ch_ptr = illegal_chars; *ch_ptr; ++ch_ptr)
                if (filename[i] == *ch_ptr)
                    fixed_name.SetAt(i, '_');
        }
    }
    return fixed_name;
}

#if AGS_PLATFORM_OS_ANDROID
    String GetPathInForeignAsset(const String &filename)
    {
        if(filename.IsEmpty()) return filename;

        if(filename[0] == '/')
        {
            return filename.Mid(1);
        }
        else if(filename[0] == '.' && filename[1] == '/')
        {
            return filename.Mid(2);
        }

        return filename;
    }
#endif

#if AGS_PLATFORM_OS_WINDOWS
String WidePathToUTF8(const wchar_t *ws)
{
    char buf[MAX_PATH_SZ];
    int need_size = WideCharToMultiByte(CP_UTF8, 0, ws, -1, NULL, 0, NULL, NULL);
    char *pbuf = (need_size <= MAX_PATH_SZ) ? buf : new char[need_size];
    WideCharToMultiByte(CP_UTF8, 0, ws, -1, pbuf, need_size, NULL, NULL);
    String s = pbuf;
    if (pbuf != buf) delete pbuf;
    return s;
}
#endif

} // namespace Path

} // namespace Common
} // namespace AGS
