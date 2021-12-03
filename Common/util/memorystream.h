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
//
// MemoryStream does reading and writing over the buffer of bytes stored in
// memory. Currently has rather trivial implementation. Does not own a buffer
// itself, but works with the provided std::vector reference, which means that
// the buffer *must* persist until stream is closed.
// TODO: perhaps accept const char* for reading mode, for compatibility with
// the older code, and maybe to let also read String objects?
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
    // Construct memory stream in the read-only mode over a const std::vector;
    // vector must persist in memory until the stream is closed.
    MemoryStream(const std::vector<uint8_t> &cbuf, DataEndianess stream_endianess = kLittleEndian);
    // Construct memory stream in the read-only mode over a const String;
    // String object must persist in memory until the stream is closed.
    MemoryStream(const String &cbuf, DataEndianess stream_endianess = kLittleEndian);
    // Construct memory stream in the chosen mode over a given std::vector;
    // vector must persist in memory until the stream is closed.
    MemoryStream(std::vector<uint8_t> &buf, StreamWorkMode mode, DataEndianess stream_endianess = kLittleEndian);
    ~MemoryStream() override;

    void    Close() override;
    bool    Flush() override;

    // Is stream valid (underlying data initialized properly)
    bool    IsValid() const override;
    // Is end of stream
    bool    EOS() const override;
    // Total length of stream (if known)
    soff_t  GetLength() const override;
    // Current position (if known)
    soff_t  GetPosition() const override;
    bool    CanRead() const override;
    bool    CanWrite() const override;
    bool    CanSeek() const override;

    size_t  Read(void *buffer, size_t size) override;
    int32_t ReadByte() override;
    size_t  Write(const void *buffer, size_t size) override;
    int32_t WriteByte(uint8_t b) override;

    bool    Seek(soff_t offset, StreamSeek origin) override;

private:
    const uint8_t           *_cbuf;
    size_t                   _len;
    std::vector<uint8_t>    *_buf;
    const StreamWorkMode     _mode;
    soff_t                   _pos;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__MEMORYSTREAM_H
