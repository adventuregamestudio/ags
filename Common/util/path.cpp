
#include <sys/types.h>
#include <sys/stat.h>
#if defined (WINDOWS_VERSION)
#include <windows.h>
#endif
#include "util/path.h"
#include "allegro/file.h"

namespace AGS
{
namespace Common
{

namespace Path
{

bool IsDirectory(const String &filename)
{
    struct stat st;
    String fixed_path = filename;
    // stat() does not like trailing slashes, remove them
    fixed_path.TrimRight('/');
    fixed_path.TrimRight('\\');
    if (stat(fixed_path, &st) == 0)
    {
        return (st.st_mode & S_IFMT) == S_IFDIR;
    }
    return false;
}

bool IsFile(const String &filename)
{
    struct stat st;
    if (stat(filename, &st) == 0)
    {
        return (st.st_mode & S_IFMT) == S_IFREG;
    }
    return false;
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
        fixed_path1.CompareNoCase(fixed_path2);
#endif // AGS_CASE_SENSITIVE_FILESYSTEM
    return cmp_result;
}

void FixupPath(String &path)
{
    if (path.IsEmpty())
    {
        return;
    }
    path.Replace('\\', '/');
}

String MakeAbsolutePath(const String &path)
{
    if (path.IsEmpty())
    {
        return "";
    }
    String abs_path = path;
#if defined (WINDOWS_VERSION)
    // NOTE: cannot use long path names in the engine, because it does not have unicode strings support
    //
    //char long_path_buffer[MAX_PATH];
    //if (GetLongPathNameA(path, long_path_buffer, MAX_PATH) > 0)
    //{
    //    abs_path = long_path_buffer;
    //}
#elif defined (PSP_VERSION)
    // FIXME: Properly construct a full PSP path
    return path;
#endif
    char buf[512];
    canonicalize_filename(buf, abs_path, 512);
    abs_path = buf;
    FixupPath(abs_path);
    return abs_path;
}

} // namespace Path

} // namespace Common
} // namespace AGS
