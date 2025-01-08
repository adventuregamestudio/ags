//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// BufferedStream is a proxy implementation of IStreamBase which works
// as a buffering mechanism over another IStreamBase. BufferedStream uses the
// memory buffer during read and write operations, which reduces number of
// reads and writes on actual device, thus possibly improving i/o performance.
//
// BufferedStream optionally supports limiting the stream operations
// to an arbitrary offset range.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__BUFFEREDSTREAM_H
#define __AGS_CN_UTIL__BUFFEREDSTREAM_H

#include <vector>
#include "util/stream.h"

namespace AGS
{
namespace Common
{

class BufferedStream : public StreamBase
{
public:
    // Needs tuning depending on the platform.
    static const size_t BufferSize = 1024u * 8;
    BufferedStream(std::unique_ptr<IStreamBase> &&base_stream)
        { Open(std::move(base_stream)); }
    // Constructs a BufferedStream limited by an arbitrary offset range
    BufferedStream(std::unique_ptr<IStreamBase> &&base_stream, soff_t start_pos, soff_t end_pos)
        { OpenSection(std::move(base_stream), start_pos, end_pos); }

    ~BufferedStream();

    const char *GetPath() const override { return _base->GetPath(); }
    StreamMode GetMode() const override { return _base->GetMode(); }
    bool    GetError() const override { return _base->GetError(); }

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

private:
    void Open(std::unique_ptr<IStreamBase> &&base_stream);
    void OpenSection(std::unique_ptr<IStreamBase> &&base_stream, soff_t start_pos, soff_t end_pos);
    // Reads a chunk of file into the buffer, starting from the given offset
    void FillBufferFromPosition(soff_t position);
    // Writes a buffer into the file, and reposition to the new offset
    void FlushBuffer(soff_t position);

    std::unique_ptr<IStreamBase> _base;
    soff_t _start = 0; // valid section starting offset
    soff_t _end = 0; // valid section ending offset
    soff_t _position = 0; // absolute read/write offset
    soff_t _bufferPosition = 0; // buffer's location relative to file
    std::vector<uint8_t> _buffer;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__BUFFEREDSTREAM_H
