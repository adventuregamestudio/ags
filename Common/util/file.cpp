//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#if !defined(WINDOWS_VERSION)
#include <unistd.h> // for unlink()
#endif

#include <errno.h>
#include "util/file.h"

#if !defined (WINDOWS_VERSION)
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

bool File::TestReadFile(const String &filename)
{
    FILE *test_file = fopen(filename, "rb");
    if (test_file)
    {
        fclose(test_file);
        return true;
    }
    return false;
}

bool File::TestCreateFile(const String &filename)
{
    FILE *test_file = fopen(filename, "wb");
    if (test_file)
    {
        fclose(test_file);
        unlink(filename);
        return true;
    }
    return false;
}

bool File::DeleteFile(const String &filename)
{
    if (unlink(filename) != 0)
    {
        int err;
#if defined(WINDOWS_VERSION)
        _get_errno(&err);
#else
        err = errno;
#endif
        if (err == EACCES)
        {
            return false;
        }
    }
    return true;
}

FileStream *File::OpenFile(const String &filename, FileOpenMode open_mode, FileWorkMode work_mode)
{
    FileStream *fs = new FileStream(filename, open_mode, work_mode);
    if (!fs->IsValid())
    {
        delete fs;
        return NULL;
    }
    return fs;
}

} // namespace Common
} // namespace AGS