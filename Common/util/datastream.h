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
// Standard AGS stream implementation for reading raw data with support for
// converting to opposite endianess. Most I/O devices should inherit this
// class and provide implementation for basic reading and writing only.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__DATASTREAM_H
#define __AGS_CN_UTIL__DATASTREAM_H

#include "util/bbop.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

class DataStream : public Stream
{
public:
    DataStream(DataEndianess stream_endianess = kLittleEndian);
    ~DataStream() override;

    int8_t  ReadInt8() override;
    int16_t ReadInt16() override;
    int32_t ReadInt32() override;
    int64_t ReadInt64() override;

    //
    // Read- and WriteArray methods return number of full elements (NOT bytes)
    // read or written, or -1 if end of stream is reached
    //
    // Note that ReadArray and WriteArray do NOT convert byte order even when
    // work with data of different endianess; they are meant for optimal
    // reading and writing blocks of raw bytes
    size_t ReadArray(void *buffer, size_t elem_size, size_t count) override
    {
        return Read(buffer, elem_size * count) / elem_size;
    }

    size_t ReadArrayOfInt16(int16_t *buffer, size_t count) override
    {
        return MustSwapBytes() ?
            ReadAndConvertArrayOfInt16(buffer, count) : ReadArray(buffer, sizeof(int16_t), count);
    }

    size_t ReadArrayOfInt32(int32_t *buffer, size_t count) override
    {
        return MustSwapBytes() ?
            ReadAndConvertArrayOfInt32(buffer, count) : ReadArray(buffer, sizeof(int32_t), count);
    }

    size_t ReadArrayOfInt64(int64_t *buffer, size_t count) override
    {
        return MustSwapBytes() ?
            ReadAndConvertArrayOfInt64(buffer, count) : ReadArray(buffer, sizeof(int64_t), count);
    }

    size_t  WriteInt8(int8_t val) override;
    size_t  WriteInt16(int16_t val) override;
    size_t  WriteInt32(int32_t val) override;
    size_t  WriteInt64(int64_t val) override;
    
    size_t WriteArray(const void *buffer, size_t elem_size, size_t count) override
    {
        return Write(buffer, elem_size * count) / elem_size;
    }

    size_t WriteArrayOfInt16(const int16_t *buffer, size_t count) override
    {
        return MustSwapBytes() ?
            WriteAndConvertArrayOfInt16(buffer, count) : WriteArray(buffer, sizeof(int16_t), count);
    }

    size_t WriteArrayOfInt32(const int32_t *buffer, size_t count) override
    {
        return MustSwapBytes() ?
            WriteAndConvertArrayOfInt32(buffer, count) : WriteArray(buffer, sizeof(int32_t), count);
    }

    size_t WriteArrayOfInt64(const int64_t *buffer, size_t count) override
    {
        return MustSwapBytes() ?
            WriteAndConvertArrayOfInt64(buffer, count) : WriteArray(buffer, sizeof(int64_t), count);
    }

protected:
    DataEndianess _streamEndianess;

    // Helper methods for reading/writing arrays of basic types and
    // converting their elements to opposite endianess (swapping bytes).
    size_t  ReadAndConvertArrayOfInt16(int16_t *buffer, size_t count);
    size_t  ReadAndConvertArrayOfInt32(int32_t *buffer, size_t count);
    size_t  ReadAndConvertArrayOfInt64(int64_t *buffer, size_t count);
    size_t  WriteAndConvertArrayOfInt16(const int16_t *buffer, size_t count);
    size_t  WriteAndConvertArrayOfInt32(const int32_t *buffer, size_t count);
    size_t  WriteAndConvertArrayOfInt64(const int64_t *buffer, size_t count);

    inline bool MustSwapBytes()
    {
        return kDefaultSystemEndianess != _streamEndianess;
    }

    inline void ConvertInt16(int16_t &val)
    {
        if (MustSwapBytes()) val = BBOp::SwapBytesInt16(val);
    }
    inline void ConvertInt32(int32_t &val)
    {
        if (MustSwapBytes()) val = BBOp::SwapBytesInt32(val);
    }
    inline void ConvertInt64(int64_t &val)
    {
        if (MustSwapBytes()) val = BBOp::SwapBytesInt64(val);
    }
};


//
// DataStreamSection wraps another stream and restricts its access
// to a particular range of offsets of the base stream.
// StreamSection does NOT own the base stream, and closing
// a "stream section" does NOT close the base stream.
// Base stream must stay in memory for as long as there are
// "stream sections" refering it. 
class DataStreamSection : public DataStream
{
public:
    // Constructs a StreamSection over a base stream,
    // restricting working range to [start, end), i.e. end offset is
    // +1 past allowed position.
    DataStreamSection(Stream *base, soff_t start, soff_t end);

    StreamMode  GetMode() const override { return _base->GetMode(); }
    const char *GetPath() const override { return _base->GetPath(); }
    bool        EOS() const override     { return _position >= _end; }
    bool        GetError() const override { return _base->GetError(); }

    soff_t GetLength() const override { return _end - _start; }
    soff_t GetPosition() const override { return _position - _start; }

    size_t Read(void *buffer, size_t len) override;
    int32_t ReadByte() override;
    size_t Write(const void *buffer, size_t len) override;
    int32_t WriteByte(uint8_t b) override;
    soff_t Seek(soff_t offset, StreamSeek origin = kSeekCurrent) override;
    bool   Flush() override { return _base->Flush(); }
    void   Close() override;

private:
    Stream *_base = nullptr;
    soff_t _start = 0;
    soff_t _end = 0;
    soff_t _position = 0;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__DATASTREAM_H
