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

#if defined(WINDOWS_VERSION)
#include <io.h>
#else
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

bool File::GetFileModesFromCMode(const String &cmode, FileOpenMode &open_mode, FileWorkMode &work_mode)
{
    // We do not test for 'b' and 't' here, because text mode reading/writing should be done with
    // the use of ITextReader and ITextWriter implementations.
    // The number of supported variants here is quite limited due the restrictions AGS makes on them.
    bool read_base_mode = false;
    // Default mode is open/read for safety reasons
    open_mode = kFile_Open;
    work_mode = kFile_Read;
    for (int c = 0; c < cmode.GetLength(); ++c)
    {
        if (read_base_mode)
        {
            if (cmode[c] == '+')
            {
                work_mode = kFile_ReadWrite;
            }
            break;
        }
        else
        {
            if (cmode[c] == 'r')
            {
                open_mode = kFile_Open;
                work_mode = kFile_Read;
                read_base_mode = true;
            }
            else if (cmode[c] == 'a')
            {
                open_mode = kFile_Create;
                work_mode = kFile_Write;
                read_base_mode = true;
            }
            else if (cmode[c] == 'w')
            {
                open_mode = kFile_CreateAlways;
                work_mode = kFile_Write;
                read_base_mode = true;
            }
        }
    }
    return read_base_mode;
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
