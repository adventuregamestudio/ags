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
// MemoryStream does reading and writing over the buffer of bytes stored in
// memory. Currently has rather trivial implementation. Does not own a buffer
// itself, but works with the provided C-buffer pointer, which means that the
// buffer object *must* persist until stream is closed.
//
// VectorStream is a specialized implementation that works with std::vector.
// Unlike base MemoryStream provides continiously resizing buffer for writing.
// TODO: separate StringStream for reading & writing String object?
//
//=============================================================================
#ifndef __AGS_CN_UTIL__MEMORYSTREAM_H
#define __AGS_CN_UTIL__MEMORYSTREAM_H

#include <vector>
#include "util/datastream.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class MemoryStream : public DataStream
{
public:
    // Construct memory stream in the read-only mode over a const C-buffer;
    // reading will never exceed buf_sz bytes;
    // buffer must persist in memory until the stream is closed.
    MemoryStream(const uint8_t *cbuf, size_t buf_sz, DataEndianess stream_endianess = kLittleEndian);
    // Construct memory stream in the chosen mode over a given C-buffer;
    // neither reading nor writing will ever exceed buf_sz bytes;
    // buffer must persist in memory until the stream is closed.
    MemoryStream(uint8_t *buf, size_t buf_sz, StreamMode mode, DataEndianess stream_endianess = kLittleEndian);
    ~MemoryStream() override = default;

    void    Close() override;
    bool    Flush() override;

    StreamMode GetMode() const override { return _mode; }
    bool    GetError() const override { return false; }
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

protected:
    const uint8_t           *_cbuf = nullptr; // readonly buffer ptr
    size_t                   _buf_sz = 0u; // hard buffer limit
    size_t                   _len = 0u; // calculated length of stream
    StreamMode               _mode = kStream_None;
    size_t                   _pos = 0u; // current stream pos

private:
    uint8_t                 *_buf = nullptr; // writeable buffer ptr
};


class VectorStream : public MemoryStream
{
public:
    // Construct memory stream in the read-only mode over a const std::vector;
    // vector must persist in memory until the stream is closed.
    VectorStream(const std::vector<uint8_t> &cbuf, DataEndianess stream_endianess = kLittleEndian);
    // Construct memory stream in the chosen mode over a given std::vector;
    // vector must persist in memory until the stream is closed.
    VectorStream(std::vector<uint8_t> &buf, StreamMode mode, DataEndianess stream_endianess = kLittleEndian);
    ~VectorStream() override = default;

    void    Close() override;

    size_t  Write(const void *buffer, size_t size) override;
    int32_t WriteByte(uint8_t b) override;

private:
    std::vector<uint8_t> *_vec = nullptr; // writeable vector (may be null)
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__MEMORYSTREAM_H
