
#include <errno.h>
#if defined (WINDOWS_VERSION)
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "util/directory.h"
#include "util/path.h"

namespace AGS
{
namespace Common
{

namespace Directory
{

bool CreateDirectory(const String &path)
{
    return mkdir(path
#if !defined (WINDOWS_VERSION)
        , 0755
#endif
        ) == 0 || errno == EEXIST;
}

String SetCurrentDirectory(const String &path)
{
    chdir(path);
    return GetCurrentDirectory();
}

String GetCurrentDirectory()
{
    char buf[512];
    getcwd(buf, 512);
    String str(buf);
    Path::FixupPath(str);
    return str;
}

} // namespace Directory

} // namespace Common
} // namespace AGS
