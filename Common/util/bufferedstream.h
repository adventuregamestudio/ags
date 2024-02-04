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
// BufferedStream represents a buffered file stream; uses memory buffer
// during read and write operations to limit number reads and writes on disk
// and thus improve i/o performance.
//
// BufferedSectionStream is a subclass stream that limits reading by an
// arbitrary offset range.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__BUFFEREDSTREAM_H
#define __AGS_CN_UTIL__BUFFEREDSTREAM_H

#include <vector>
#include "util/filestream.h"
#include "util/file.h" // TODO: extract filestream mode constants

namespace AGS
{
namespace Common
{

class BufferedStream : public FileStream
{
public:
    // Needs tuning depending on the platform.
    static const size_t BufferSize = 1024u * 8;
    // The constructor may raise std::runtime_error if 
    // - there is an issue opening the file (does not exist, locked, permissions, etc)
    // - the open mode could not be determined
    // - could not determine the length of the stream
    // It is recommended to use File::OpenFile to safely construct this object.
    BufferedStream(const String &file_name, FileOpenMode open_mode,
        StreamMode work_mode, DataEndianess stream_endianess = kLittleEndian);
    ~BufferedStream();

    // Is end of stream
    bool    EOS() const override;
    // Total length of stream (if known)
    soff_t  GetLength() const override;
    // Current position (if known)
    soff_t  GetPosition() const override;

    void    Close() override;
    bool    Flush() override;

    size_t  Read(void *buffer, size_t size) override;
    int32_t ReadByte() override;
    size_t  Write(const void *buffer, size_t size) override;
    int32_t WriteByte(uint8_t b) override;

    soff_t  Seek(soff_t offset, StreamSeek origin) override;

protected:
    soff_t _start = 0; // valid section starting offset
    soff_t _end = -1; // valid section ending offset

private:
    // Reads a chunk of file into the buffer, starting from the given offset
    void FillBufferFromPosition(soff_t position);
    // Writes a buffer into the file, and reposition to the new offset
    void FlushBuffer(soff_t position);

    soff_t _position = 0; // absolute read/write offset
    soff_t _bufferPosition = 0; // buffer's location relative to file
    std::vector<uint8_t> _buffer;
};


// Creates a BufferedStream limited by an arbitrary offset range
class BufferedSectionStream : public BufferedStream
{
public:
    BufferedSectionStream(const String &file_name, soff_t start_pos, soff_t end_pos,
        FileOpenMode open_mode, StreamMode work_mode, DataEndianess stream_endianess = kLittleEndian);
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__BUFFEREDSTREAM_H
