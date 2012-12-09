
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
    char long_path_buffer[MAX_PATH];
    if (GetLongPathNameA(path, long_path_buffer, MAX_PATH) > 0)
    {
        abs_path = long_path_buffer;
    }
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
