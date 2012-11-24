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
// Standard AGS stream base for reading raw data with support for converting to
// opposite endianess.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__DATASTREAM_H
#define __AGS_CN_UTIL__DATASTREAM_H

#include "util/bbop.h"
#include "util/stream.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class DataStream : public Stream
{
public:
    DataStream(DataEndianess caller_endianess, DataEndianess stream_endianess);
    virtual ~DataStream();

    // Methods for reading basic types - implementation-specific
    inline  int8_t  ReadInt8()
    {
        return ReadByte();
    }
    int16_t ReadInt16();
    int32_t ReadInt32();
    int64_t ReadInt64();

    inline int WriteInt8(int8_t val)
    {
        return WriteByte(val);
    }
    size_t  WriteInt16(int16_t val);
    size_t  WriteInt32(int32_t val);
    size_t  WriteInt64(int64_t val);

    //-------------------------------------------------------------------------
    // Helper methods
    //-------------------------------------------------------------------------
    inline bool ReadBool()
    {
        return ReadInt8() != 0;
    }

    inline int WriteBool(bool val)
    {
        return WriteInt8(val ? 1 : 0);
    }

    //
    // Read- and WriteArray methods return number of full elements (NOT bytes)
    // read or written, or -1 if end of stream is reached
    //
    // Note that ReadArray and WriteArray do NOT convert byte order even when
    // work with data of different endianess; they are meant for optimal
    // reading and writing blocks of raw bytes

    inline virtual size_t ReadArray(void *buffer, size_t elem_size, size_t count)
    {
        return Read(buffer, elem_size * count) / elem_size;
    }

    inline virtual size_t WriteArray(const void *buffer, size_t elem_size, size_t count)
    {
        return Write(buffer, elem_size * count) / elem_size;
    }

    // Practically identical to Read(), this helper's only advantage is
    // that it makes the meaning of operation more clear to the user
    inline size_t ReadArrayOfInt8(int8_t *buffer, size_t count)
    {
        return Read(buffer, count);
    }
    inline size_t ReadArrayOfInt16(int16_t *buffer, size_t count)
    {
        return MustSwapBytes() ?
            ReadAndConvertArrayOfInt16(buffer, count) : ReadArray(buffer, sizeof(int16_t), count);
    }
    inline size_t ReadArrayOfInt32(int32_t *buffer, size_t count)
    {
        return MustSwapBytes() ?
            ReadAndConvertArrayOfInt32(buffer, count) : ReadArray(buffer, sizeof(int32_t), count);
    }
    inline size_t ReadArrayOfInt64(int64_t *buffer, size_t count)
    {
        return MustSwapBytes() ?
            ReadAndConvertArrayOfInt64(buffer, count) : ReadArray(buffer, sizeof(int64_t), count);
    }
    // Read array of pointers of build-dependent size
    size_t ReadArrayOfIntPtr(intptr_t *buffer, size_t count);
    // Helper function for easier compatibility with 64-bit platforms
    // reads 32-bit values and stores them in intptr_t array
    size_t ReadArrayOfIntPtr32(intptr_t *buffer, size_t count);

    inline size_t WriteArrayOfInt8(const int8_t *buffer, size_t count)
    {
        return Write(buffer, count);
    }
    inline size_t WriteArrayOfInt16(const int16_t *buffer, size_t count)
    {
        return MustSwapBytes() ?
            WriteAndConvertArrayOfInt16(buffer, count) : WriteArray(buffer, sizeof(int16_t), count);
    }
    inline size_t WriteArrayOfInt32(const int32_t *buffer, size_t count)
    {
        return MustSwapBytes() ?
            WriteAndConvertArrayOfInt32(buffer, count) : WriteArray(buffer, sizeof(int32_t), count);
    }
    inline size_t WriteArrayOfInt64(const int64_t *buffer, size_t count)
    {
        return MustSwapBytes() ?
            WriteAndConvertArrayOfInt64(buffer, count) : WriteArray(buffer, sizeof(int64_t), count);
    }
    // Write array of pointers of build-dependent size
    size_t WriteArrayOfIntPtr(const intptr_t *buffer, size_t count);
    // Helper function for easier compatibility with 64-bit platforms,
    // writes intptr_t array elements as 32-bit values
    size_t WriteArrayOfIntPtr32(const intptr_t *buffer, size_t count);

    // Helper methods for reading and writing null-terminated string,
    // reading implies that string length is initially unknown.
    //
    // max_chars parameter determine the buffer size limit, however it
    // does not limit stream reading; the data will be read until null-
    // terminator or EOS is met, the buffer will contain only leftmost
    // part of the longer string that fits in.
    String  ReadString(int max_chars = 5000000);
    // WriteString returns a length of bytes written
    size_t  WriteString(const String &str);

protected:
    DataEndianess _callerEndianess;
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
        return _callerEndianess != _streamEndianess;
    }

    inline void ConvertInt16(int16_t &val)
    {
        if (MustSwapBytes()) BBOp::SwapBytesInt16(val);
    }
    inline void ConvertInt32(int32_t &val)
    {
        if (MustSwapBytes()) BBOp::SwapBytesInt32(val);
    }
    inline void ConvertInt64(int64_t &val)
    {
        if (MustSwapBytes()) BBOp::SwapBytesInt64(val);
    }
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__DATASTREAM_H
