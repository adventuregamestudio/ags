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
#include "util/filestream.h"

namespace AGS
{
namespace Common
{

soff_t File::GetFileSize(const String &filename)
{
    struct stat_t st;
    if (stat_fn(filename, &st) == 0)
        return st.st_size;
    return -1;
}

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

bool File::TestWriteFile(const String &filename)
{
    FILE *test_file = fopen(filename, "r+");
    if (test_file)
    {
        fclose(test_file);
        return true;
    }
    return TestCreateFile(filename);
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
    for (size_t c = 0; c < cmode.GetLength(); ++c)
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

String File::GetCMode(FileOpenMode open_mode, FileWorkMode work_mode)
{
    String mode;
    if (open_mode == kFile_Open)
    {
        if (work_mode == kFile_Read)
            mode.AppendChar('r');
        else if (work_mode == kFile_Write || work_mode == kFile_ReadWrite)
            mode.Append("r+");
    }
    else if (open_mode == kFile_Create)
    {
        if (work_mode == kFile_Write)
            mode.AppendChar('a');
        else if (work_mode == kFile_Read || work_mode == kFile_ReadWrite)
            mode.Append("a+");
    }
    else if (open_mode == kFile_CreateAlways)
    {
        if (work_mode == kFile_Write)
            mode.AppendChar('w');
        else if (work_mode == kFile_Read || work_mode == kFile_ReadWrite)
            mode.Append("w+");
    }
    mode.AppendChar('b');
    return mode;
}

Stream *File::OpenFile(const String &filename, FileOpenMode open_mode, FileWorkMode work_mode)
{
    FileStream *fs = new FileStream(filename, open_mode, work_mode);
    if (!fs->IsValid())
    {
        delete fs;
        return nullptr;
    }
    return fs;
}

} // namespace Common
} // namespace AGS
