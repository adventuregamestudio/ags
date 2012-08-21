
#include <stdio.h>
#include "util/filestream.h"

// FIXME
#define byte    unsigned char

namespace AGS
{
namespace Common
{

CFileStream::CFileStream(const CString &file_name, FileOpenMode open_mode, FileWorkMode work_mode)
    : CStream(kDefaultSystemEndianess, kDefaultSystemEndianess)
    , _file(NULL)
    , _openMode(open_mode)
    , _workMode(work_mode)
    , _length(0)
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

    _file = fopen(file_name, mode);
    _length = _file->_cnt;
}

CFileStream::CFileStream(const CString &file_name, FileOpenMode open_mode, FileWorkMode work_mode,
            DataEndianess caller_endianess, DataEndianess stream_endianess)
    : CStream(caller_endianess, stream_endianess)
    , _file(NULL)
    , _openMode(open_mode)
    , _workMode(work_mode)
    , _length(0)
{

}

CFileStream::~CFileStream()
{
    if (_file)
    {
        fclose(_file);
    }
}

bool CFileStream::IsValid() const
{
    return _file != NULL;
}

bool CFileStream::EOS() const
{
    return !IsValid() || feof(_file) != 0;
}

int CFileStream::Length() const
{
    // TODO
    return IsValid() ? _length : 0;
}

int CFileStream::Position() const
{
    // TODO
    return IsValid() ? _file->_cnt : 0;
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
    return IsValid() && _file != NULL;
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

    if (result == 0)
    {
        // Success
    }
    // TODO
    return 0;
}

int8_t CFileStream::ReadInt8()
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

CString CFileStream::ReadString(int max_chars)
{
    if (!CanRead())
    {
        return "";
    }

    CString str;
    int chars_read = 0;
    int buf_size = 0;
    char c = 0;
    do
    {
        buf_size = str.GetLength() + 3000;
        char *buf = str.GetBuffer(buf_size);
        do
        {
            buf[chars_read++] = c = getc(_file);
        }
        while (c != 0 && chars_read < buf_size);
        str.ReleaseBuffer(chars_read);
    }
    while (c != 0);
    return str;
}

void CFileStream::WriteInt8(int8_t val)
{
    if (CanWrite())
    {
        fputc(val, _file);
    }
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
        return fwrite(&buffer, sizeof(byte), size, _file);
    }
    return 0;
}

void CFileStream::WriteString(const CString &str)
{
    if (CanWrite())
    {
        fwrite(str.GetCStr(), sizeof(char), str.GetLength(), _file);
        fputc(0, _file);
    }
}

int CFileStream::ReadAndConvertArrayOfInt16(int16_t *buffer, int count)
{
    if (!CanRead())
    {
        return 0;
    }

    for (int i = 0; i < count; ++i, ++buffer)
    {
        fread(buffer, sizeof(int16_t), 1, _file);
        ConvertInt16(*buffer);
    }
    return count;
}

int CFileStream::ReadAndConvertArrayOfInt32(int32_t *buffer, int count)
{
    if (!CanRead())
    {
        return 0;
    }

    for (int i = 0; i < count; ++i, ++buffer)
    {
        fread(buffer, sizeof(int32_t), 1, _file);
        ConvertInt32(*buffer);
    }
    return count;
}

int CFileStream::ReadAndConvertArrayOfInt64(int64_t *buffer, int count)
{
    if (!CanRead())
    {
        return 0;
    }

    for (int i = 0; i < count; ++i, ++buffer)
    {
        fread(buffer, sizeof(int64_t), 1, _file);
        ConvertInt64(*buffer);
    }
    return count;
}

int CFileStream::WriteAndConvertArrayOfInt16(const int16_t *buffer, int count)
{
    if (!CanWrite())
    {
        return 0;
    }

    for (int i = 0; i < count; ++i, ++buffer)
    {
        int16_t val = *buffer;
        ConvertInt16(val);
        fwrite(&val, sizeof(int16_t), 1, _file);
    }
    return count;
}

int CFileStream::WriteAndConvertArrayOfInt32(const int32_t *buffer, int count)
{
    if (!CanWrite())
    {
        return 0;
    }

    for (int i = 0; i < count; ++i, ++buffer)
    {
        int32_t val = *buffer;
        ConvertInt32(val);
        fwrite(&val, sizeof(int32_t), 1, _file);
    }
    return count;
}

int CFileStream::WriteAndConvertArrayOfInt64(const int64_t *buffer, int count)
{
    if (!CanWrite())
    {
        return 0;
    }

    for (int i = 0; i < count; ++i, ++buffer)
    {
        int64_t val = *buffer;
        ConvertInt64(val);
        fwrite(&val, sizeof(int64_t), 1, _file);
    }
    return count;
}

} // namespace Common
} // namespace AGS
