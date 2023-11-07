//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "util/filestream.h"
#include <stdexcept>
#include "util/stdio_compat.h"

namespace AGS
{
namespace Common
{

FileStream::FileStream(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode,
            DataEndianess stream_endianess)
    : DataStream(stream_endianess)
    , _file(nullptr)
    , _openMode(open_mode)
    , _workMode(work_mode)
{
    Open(file_name, open_mode, work_mode);
}

FileStream::FileStream(FILE *file, bool own, FileWorkMode work_mode, DataEndianess stream_end)
    : DataStream(stream_end)
    , _file(file)
    , _ownHandle(own)
    , _openMode(kFile_Open)
    , _workMode(work_mode)
{
}

FileStream::~FileStream()
{
    Close();
}

bool FileStream::HasErrors() const
{
    return IsValid() && ferror(_file) != 0;
}

void FileStream::Close()
{
    if (_ownHandle)
    {
        fclose(_file);
    }
    _file = nullptr;
    _ownHandle = false;
    if (FileCloseNotify)
    {
        CloseNotifyArgs args;
        args.Filepath = _path;
        args.WorkMode = _workMode;
        FileCloseNotify(args);
    }
}

bool FileStream::Flush()
{
    return fflush(_file) == 0;
}

bool FileStream::IsValid() const
{
    return _file != nullptr;
}

bool FileStream::EOS() const
{
    return feof(_file) != 0;
}

soff_t FileStream::GetLength() const
{
    soff_t pos = (soff_t)ags_ftell(_file);
    ags_fseek(_file, 0, SEEK_END);
    soff_t end = (soff_t)ags_ftell(_file);
    ags_fseek(_file, pos, SEEK_SET);
    return end;
}

soff_t FileStream::GetPosition() const
{
    return static_cast<soff_t>(ags_ftell(_file));
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
    return fread(buffer, sizeof(uint8_t), size, _file);
}

int32_t FileStream::ReadByte()
{
    return fgetc(_file);
}

size_t FileStream::Write(const void *buffer, size_t size)
{
    return fwrite(buffer, sizeof(uint8_t), size, _file);
}

int32_t FileStream::WriteByte(uint8_t val)
{
    return fputc(val, _file);
}

bool FileStream::Seek(soff_t offset, StreamSeek origin)
{
    int stdclib_origin;
    switch (origin)
    {
    case kSeekBegin:    stdclib_origin = SEEK_SET; break;
    case kSeekCurrent:  stdclib_origin = SEEK_CUR; break;
    case kSeekEnd:      stdclib_origin = SEEK_END; break;
    default:
        // TODO: warning to the log
        return false;
    }

    return ags_fseek(_file, (file_off_t)offset, stdclib_origin) == 0;
}

void FileStream::Open(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode)
{
    String mode = File::GetCMode(open_mode, work_mode);
    if (mode.IsEmpty())
        throw std::runtime_error("Error determining open mode");
    _file = ags_fopen(file_name.GetCStr(), mode.GetCStr());
    if (_file == nullptr)
        throw std::runtime_error("Error opening file.");
    _ownHandle = true;
    _path = file_name;
}

FileStream::FFileCloseNotify FileStream::FileCloseNotify = nullptr;

} // namespace Common
} // namespace AGS
