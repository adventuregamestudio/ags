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

namespace AGS
{
namespace Common
{

FileStream::FileStream(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode)
    : DataStream(kDefaultSystemEndianess, kLittleEndian)
    , _file(NULL)
    , _openMode(open_mode)
    , _workMode(work_mode)
{
    Open(file_name, open_mode, work_mode);
}

FileStream::FileStream(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode,
            DataEndianess caller_endianess, DataEndianess stream_endianess)
    : DataStream(caller_endianess, stream_endianess)
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

bool FileStream::IsValid() const
{
    return _file != NULL;
}

bool FileStream::EOS() const
{
    return !IsValid() || feof(_file) != 0;
}

int FileStream::GetLength() const
{
    if (IsValid())
    {
        return filelength(fileno(_file));
    }

    return 0;
}

int FileStream::GetPosition() const
{
    return IsValid() ? ftell(_file) : 0;
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

void FileStream::Close()
{
    if (_file)
    {
        fclose(_file);
    }
    _file = NULL;
}

int FileStream::ReadByte()
{
    if (CanRead())
    {
        return fgetc(_file);
    }
    return 0;
}

int16_t FileStream::ReadInt16()
{
    if (CanRead())
    {
        int16_t val;
        fread(&val, sizeof(int16_t), 1, _file);
        ConvertInt16(val);
        return val;
    }
    return 0;
}

int32_t FileStream::ReadInt32()
{
    if (CanRead())
    {
        int32_t val = getw(_file);
        ConvertInt32(val);
        return val;
    }
    return 0;
}

int64_t FileStream::ReadInt64()
{
    if (CanRead())
    {
        int64_t val;
        fread(&val, sizeof(int64_t), 1, _file);
        ConvertInt64(val);
        return val;
    }
    return 0;
}

int FileStream::Read(void *buffer, int32_t size)
{
    if (CanRead())
    {
        return fread(buffer, sizeof(uint8_t), size, _file);
    }
    return 0;
}

int FileStream::WriteByte(uint8_t val)
{
    if (CanWrite())
    {
        return fputc(val, _file);
    }
    return 0;
}

void FileStream::WriteInt16(int16_t val)
{
    if (CanWrite())
    {
        ConvertInt16(val);
        fwrite(&val, sizeof(int16_t), 1, _file);
    }
}

void FileStream::WriteInt32(int32_t val)
{
    if (CanWrite())
    {
        ConvertInt32(val);
        putw(val, _file);
    }
}
 
void FileStream::WriteInt64(int64_t val)
{
    if (CanWrite())
    {
        ConvertInt64(val);
        fwrite(&val, sizeof(int64_t), 1, _file);
    }
}

int FileStream::Write(const void *buffer, int size)
{
    if (CanWrite())
    {
        return fwrite(buffer, sizeof(uint8_t), size, _file);
    }
    return 0;
}

int FileStream::Seek(StreamSeek seek, int pos)
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
    }

    int result = fseek(_file, pos, stdclib_seek);
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
        else
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
        else
        {
            mode.Append("a+");
        }
    }
    else // kFile_CreateAlways
    {
        if (work_mode == kFile_Write)
        {
            mode.AppendChar('w');
        }
        else
        {
            mode.Append("w+");
        }
    }

    mode.AppendChar('b');
    _file = fopen(file_name, mode);
}

} // namespace Common
} // namespace AGS
