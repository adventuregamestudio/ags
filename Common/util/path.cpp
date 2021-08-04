#include "core/platform.h"
#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/windows.h"
#endif
#include "util/path.h"
#include "util/stdio_compat.h"

// TODO: implement proper portable path length
#ifndef MAX_PATH
#define MAX_PATH 512
#endif

namespace AGS
{
namespace Common
{

namespace Path
{

bool IsDirectory(const String &filename)
{
    // stat() does not like trailing slashes, remove them
    String fixed_path = MakePathNoSlash(filename);
    return ags_directory_exists(fixed_path.GetCStr()) != 0;
}

bool IsFile(const String &filename)
{
    return ags_file_exists(filename.GetCStr()) != 0;
}

bool IsFileOrDir(const String &filename)
{
    // stat() does not like trailing slashes, remove them
    String fixed_path = MakePathNoSlash(filename);
    return ags_path_exists(fixed_path.GetCStr()) != 0;
}

String GetParent(const String &path)
{
    const char *cstr = path.GetCStr();
    const char *ptr_end = cstr + path.GetLength();
    for (const char *ptr = ptr_end; ptr > cstr; --ptr)
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
    for (const char *ptr = ptr_end; ptr > cstr; --ptr)
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

String GetDirectoryPath(const String &path)
{
    if (IsDirectory(path))
        return path;

    String dir = path;
    FixupPath(dir);
    size_t slash_at = dir.FindCharReverse('/');
    if (slash_at != -1)
    {
        dir.ClipMid(slash_at + 1);
        return dir;
    }
    return "./";
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

String GetPathInASCII(const String &path)
{
#if AGS_PLATFORM_OS_WINDOWS
    char ascii_buffer[MAX_PATH];
    if (GetShortPathNameA(path.GetCStr(), ascii_buffer, MAX_PATH) == 0)
        return "";
    return ascii_buffer;
#else
    // TODO: implement conversion for other platforms!
    return path;
#endif
}

#if AGS_PLATFORM_OS_WINDOWS
String WidePathNameToAnsi(LPCWSTR pathw)
{
    WCHAR short_path[MAX_PATH];
    char ascii_buffer[MAX_PATH];
    LPCWSTR arg_path = pathw;
    if (GetShortPathNameW(arg_path, short_path, MAX_PATH) == 0)
        return "";
    WideCharToMultiByte(CP_ACP, 0, short_path, -1, ascii_buffer, MAX_PATH, NULL, NULL);
    return ascii_buffer;
}
#endif

String GetCmdLinePathInASCII(const char *arg, int arg_index)
{
#if AGS_PLATFORM_OS_WINDOWS
    // Hack for Windows in case there are unicode chars in the path.
    // The normal argv[] array has ????? instead of the unicode chars
    // and fails, so instead we manually get the short file name, which
    // is always using ASCII chars.
    int wargc = 0;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (wargv == nullptr)
        return "";
    String path;
    if (arg_index <= wargc)
        path = WidePathNameToAnsi(wargv[arg_index]);
    LocalFree(wargv);
    return path;
#else
    // TODO: implement conversion for other platforms!
    return arg;
#endif
}

} // namespace Path

} // namespace Common
} // namespace AGS
