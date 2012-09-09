
#include <io.h>
#include <stdio.h>
#include "util/filestream.h"
#include "util/math.h"

namespace AGS
{
namespace Common
{

CFileStream::CFileStream(const CString &file_name, FileOpenMode open_mode, FileWorkMode work_mode)
    : CDataStream(kDefaultSystemEndianess, kDefaultSystemEndianess)
    , _file(NULL)
    , _openMode(open_mode)
    , _workMode(work_mode)
{
    Open(file_name, open_mode, work_mode);
}

CFileStream::CFileStream(const CString &file_name, FileOpenMode open_mode, FileWorkMode work_mode,
            DataEndianess caller_endianess, DataEndianess stream_endianess)
    : CDataStream(caller_endianess, stream_endianess)
    , _file(NULL)
    , _openMode(open_mode)
    , _workMode(work_mode)
{
    Open(file_name, open_mode, work_mode);
}

CFileStream::~CFileStream()
{
    Close();
}

bool CFileStream::IsValid() const
{
    return _file != NULL;
}

bool CFileStream::EOS() const
{
    return !IsValid() || feof(_file) != 0;
}

int CFileStream::GetLength() const
{
    if (IsValid())
    {
        return filelength(fileno(_file));
    }

    return 0;
}

int CFileStream::GetPosition() const
{
    return IsValid() ? ftell(_file) : 0;
}

bool CFileStream::CanRead() const
{
    return IsValid() && _workMode != kFile_Write;
}

bool CFileStream::CanWrite() const
{
    return IsValid() && _workMode != kFile_Read;
}

bool CFileStream::CanSeek() const
{
    return IsValid();
}

void CFileStream::Close()
{
    if (_file)
    {
        fclose(_file);
    }
    _file = NULL;
}

int CFileStream::ReadByte()
{
    if (CanRead())
    {
        return fgetc(_file);
    }
    return 0;
}

int16_t CFileStream::ReadInt16()
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

int32_t CFileStream::ReadInt32()
{
    if (CanRead())
    {
        int32_t val = getw(_file);
        ConvertInt32(val);
        return val;
    }
    return 0;
}

int64_t CFileStream::ReadInt64()
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

int CFileStream::Read(void *buffer, int32_t size)
{
    if (CanRead())
    {
        return fread(buffer, sizeof(byte), size, _file);
    }
    return 0;
}

int CFileStream::WriteByte(byte val)
{
    if (CanWrite())
    {
        return fputc(val, _file);
    }
    return 0;
}

void CFileStream::WriteInt16(int16_t val)
{
    if (CanWrite())
    {
        ConvertInt16(val);
        fwrite(&val, sizeof(int16_t), 1, _file);
    }
}

void CFileStream::WriteInt32(int32_t val)
{
    if (CanWrite())
    {
        ConvertInt32(val);
        putw(val, _file);
    }
}
 
void CFileStream::WriteInt64(int64_t val)
{
    if (CanWrite())
    {
        ConvertInt64(val);
        fwrite(&val, sizeof(int64_t), 1, _file);
    }
}

int CFileStream::Write(const void *buffer, int size)
{
    if (CanWrite())
    {
        return fwrite(buffer, sizeof(byte), size, _file);
    }
    return 0;
}

int CFileStream::Seek(StreamSeek seek, int pos)
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

void CFileStream::Open(const CString &file_name, FileOpenMode open_mode, FileWorkMode work_mode)
{
    CString mode;

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
