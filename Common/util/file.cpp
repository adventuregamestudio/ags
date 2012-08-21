
#include "util/file.h"

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
#include <sys/stat.h>
long int filelength(int fhandle)
{
    struct stat statbuf;
    fstat(fhandle, &statbuf);
    return statbuf.st_size;
}
#endif

#if !defined ALLEGRO_BIG_ENDIAN
// Two functions to match those defined by bigend version by McCrea
short int   getshort(FILE * f)
{
    short i;
    fread(&i, sizeof(short), 1, f);
    return i; 
}

void        putshort(short int i, FILE * f)
{
    fwrite(&i, sizeof(short), 1, f);
}
#endif // !ALLEGRO_BIG_ENDIAN

#include "util/filestream.h"

namespace AGS
{
namespace Common
{

bool File::FileExists(const CString &filename)
{
    // TODO
    return false;
}

CFileStream *File::OpenFile(const CString &filename, FileOpenMode open_mode, FileWorkMode work_mode)
{
    CFileStream *fs = new CFileStream(filename, open_mode, work_mode);
    if (!fs->IsValid())
    {
        delete fs;
        return NULL;
    }
    return fs;
}

} // namespace Common
} // namespace AGS