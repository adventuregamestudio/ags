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
#include "core/platform.h"
#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/windows.h"
#endif
#include "util/file.h"
#include "util/path.h"
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

bool IsRelativePath(const String &path)
{
    // All filenames that start with a '.' are relative. */
    if (path[0] == '.')
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
