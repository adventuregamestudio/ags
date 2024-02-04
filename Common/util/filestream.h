//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__FILESTREAM_H
#define __AGS_CN_UTIL__FILESTREAM_H

#include <stdio.h>
#include <functional>

#include "util/datastream.h"
#include "util/file.h" // TODO: extract filestream mode constants

namespace AGS
{
namespace Common
{

class FileStream : public DataStream
{
public:
    struct CloseNotifyArgs
    {
        String Filepath;
        StreamMode WorkMode;
    };

    // definition of function called when file closes
    typedef std::function<void(const CloseNotifyArgs &args)> FFileCloseNotify;

    static FFileCloseNotify FileCloseNotify;

    // Represents an open file object
    // The constructor may raise std::runtime_error if 
    // - there is an issue opening the file (does not exist, locked, permissions, etc)
    // - the open mode could not be determined
    FileStream(const String &file_name, FileOpenMode open_mode, StreamMode work_mode,
        DataEndianess stream_endianess = kLittleEndian);
    // Constructs a file stream over an open FILE handle;
    // Take an ownership over it and will close upon disposal
    static FileStream *OwnHandle(FILE *file, StreamMode work_mode, DataEndianess stream_end = kLittleEndian)
        { return new FileStream(file, true, work_mode, stream_end); }
    // Constructs a file stream over an open FILE handle;
    // does NOT take an ownership over it
    static FileStream *WrapHandle(FILE *file, StreamMode work_mode, DataEndianess stream_end = kLittleEndian)
        { return new FileStream(file, false, work_mode, stream_end); }
    ~FileStream() override;

    // Tells which open mode was used when opening a file.
    FileOpenMode GetFileOpenMode() const { return _openMode; }
    // Tells which mode the stream is working in, which defines
    // supported io operations, such as reading, writing, seeking, etc.
    // Invalid streams return kStream_None to indicate that they are not functional.
    StreamMode GetMode() const override { return _workMode; }

    // Tells if there were errors during previous io operation(s);
    // the call to GetError() *resets* the error record.
    bool    GetError() const override;
    // Is end of stream
    bool    EOS() const override;
    // Total length of stream (if known)
    soff_t  GetLength() const override;
    // Current position (if known)
    soff_t  GetPosition() const override;

    size_t  Read(void *buffer, size_t size) override;
    int32_t ReadByte() override;
    size_t  Write(const void *buffer, size_t size) override;
    int32_t WriteByte(uint8_t b) override;

    soff_t  Seek(soff_t offset, StreamSeek origin) override;

    bool    Flush() override;
    void    Close() override;

private:
    FileStream(FILE *file, bool own, StreamMode work_mode, DataEndianess stream_end);
    void    Open(const String &file_name, FileOpenMode open_mode, StreamMode work_mode);

    FILE         *_file = nullptr;
    bool          _ownHandle = false;
    FileOpenMode  _openMode = kFile_None;
    StreamMode    _workMode = kStream_None;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__FILESTREAM_H
