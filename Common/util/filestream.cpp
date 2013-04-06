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
#endif
#include <stdio.h>
#include "util/filestream.h"
#include "util/math.h"

// TODO: use fstat on Windows too?
#if !defined (WINDOWS_VERSION)
#include <sys/stat.h>
long int filelength(int fhandle)
{
    struct stat statbuf;
    fstat(fhandle, &statbuf);
    return statbuf.st_size;
}
#endif

namespace AGS
{
namespace Common
{

FileStream::FileStream(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode,
            DataEndianess stream_endianess)
    : DataStream(stream_endianess)
    , _file(NULL)
    , _openMode(open_mode)
    , _workMode(work_mode)
{
    Open(file_name, open_mode, work_mode);
}

FileStream::~FileStream()
{
    Close();
}

void FileStream::Close()
{
    if (_file)
    {
        fclose(_file);
    }
    _file = NULL;
}

bool FileStream::IsValid() const
{
    return _file != NULL;
}

bool FileStream::EOS() const
{
    return !IsValid() || feof(_file) != 0;
}

size_t FileStream::GetLength() const
{
    if (IsValid())
    {
        long len = filelength(fileno(_file));
        return len > 0 ? (size_t)len : 0;
    }

    return 0;
}

size_t FileStream::GetPosition() const
{
    if (IsValid())
    {
        long pos = ftell(_file);
        return pos > 0 ? (size_t)pos : 0;
    }
    return 0;
}

bool FileStream::CanRead() const
{
    return IsValid() && _workMode != kFile_Write;
}

bool FileStream::CanWrite() const
{
    return IsValid() && _workMode != kFile_Read;
}

bool FileStream::CanSeek() const
{
    return IsValid();
}

size_t FileStream::Read(void *buffer, size_t size)
{
    if (CanRead())
    {
        return fread(buffer, sizeof(uint8_t), size, _file);
    }
    return 0;
}

int32_t FileStream::ReadByte()
{
    if (CanRead())
    {
        return fgetc(_file);
    }
    return -1;
}

size_t FileStream::Write(const void *buffer, size_t size)
{
    if (CanWrite())
    {
        return fwrite(buffer, sizeof(uint8_t), size, _file);
    }
    return 0;
}

int32_t FileStream::WriteByte(uint8_t val)
{
    if (CanWrite())
    {
        return fputc(val, _file);
    }
    return -1;
}

size_t FileStream::WriteByteCount(uint8_t val, size_t count)
{
    if (CanWrite())
    {
        size_t size = 0;
        for (size_t i = 0; i < count; ++i, ++size)
        {
            if (fputc(val, _file) < 0)
            {
                break;
            }
        }
        return size;
    }
    return 0;
}

size_t FileStream::Seek(StreamSeek seek, int pos)
{
    if (!CanSeek())
    {
        return 0;
    }

    int stdclib_seek;
    switch (seek)
    {
    case kSeekBegin:    stdclib_seek = SEEK_SET; break;
    case kSeekCurrent:  stdclib_seek = SEEK_CUR; break;
    case kSeekEnd:      stdclib_seek = SEEK_END; break;
    default:
        // TODO: warning to the log
        return 0;
    }

    fseek(_file, pos, stdclib_seek);
    return GetPosition();
}

void FileStream::Open(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode)
{
    String mode;

    if (open_mode == kFile_Open)
    {
        if (work_mode == kFile_Read)
        {
            mode.AppendChar('r');
        }
        else if (work_mode == kFile_Write || work_mode == kFile_ReadWrite)
        {
            mode.Append("r+");
        }
    }
    else if (open_mode == kFile_Create)
    {
        if (work_mode == kFile_Write)
        {
            mode.AppendChar('a');
        }
        else if (work_mode == kFile_Read || work_mode == kFile_ReadWrite)
        {
            mode.Append("a+");
        }
    }
    else if (open_mode == kFile_CreateAlways)
    {
        if (work_mode == kFile_Write)
        {
            mode.AppendChar('w');
        }
        else if (work_mode == kFile_Read || work_mode == kFile_ReadWrite)
        {
            mode.Append("w+");
        }
    }

    if (mode.IsEmpty())
    {
        // TODO: warning to the log
        return;
    }

    mode.AppendChar('b');
    _file = fopen(file_name, mode);
}

} // namespace Common
} // namespace AGS
