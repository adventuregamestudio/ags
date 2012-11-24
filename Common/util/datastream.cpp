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

#include "core/types.h"
#include "util/datastream.h"
#include "util/math.h"

namespace AGS
{
namespace Common
{

DataStream::DataStream(DataEndianess caller_endianess, DataEndianess stream_endianess)
    : _callerEndianess(caller_endianess)
    , _streamEndianess(stream_endianess)
{
}

DataStream::~DataStream()
{
}

String DataStream::ReadString(int max_chars)
{
    if (!CanRead())
    {
        return "";
    }

    String str;
    int chars_read_last = 0;
    int null_t_position = -1;
    // Read a chunk of memory to buffer and seek for null-terminator,
    // if not found, repeat until EOS
    const int single_chunk_length = 3000;
    char char_buffer[single_chunk_length + 1];
    do
    {
        chars_read_last = Read(char_buffer, single_chunk_length);
        char *seek_ptr = char_buffer;
        for (int c = 0; c < chars_read_last && *seek_ptr != '\0'; ++c, ++seek_ptr);

        int append_length = 0;
        int str_len = str.GetLength();
        if (*seek_ptr == '\0')
        {
            null_t_position = seek_ptr - char_buffer;
            if (str_len < max_chars)
            {
                append_length = Math::Min(null_t_position, max_chars - str_len);
            }
        }
        else
        {
            append_length = Math::Min(chars_read_last, max_chars - str_len);
        }

        if (append_length > 0)
        {
            char_buffer[append_length] = '\0';
            str.Append(char_buffer);
        }
    }
    while (!EOS() && null_t_position < 0);

    // If null-terminator was found make sure stream is positioned at the next
    // byte after terminator
    if (null_t_position >= 0)
    {
        // the seek offset should be negative
        Seek(kSeekCurrent, null_t_position - chars_read_last + 1 /* beyond null-terminator */);
    }

    return str;
}

size_t DataStream::WriteString(const String &str)
{
    if (CanWrite())
    {
        int bytes_written = Write(str.GetCStr(), sizeof(char) * str.GetLength());
        WriteInt8(0);
        return bytes_written;
    }
    return 0;
}

size_t DataStream::ReadArrayOfIntPtr(intptr_t *buffer, size_t count)
{
#if defined (AGS_64BIT) || defined (TEST_64BIT)
    return MustSwapBytes() ? ReadAndConvertArrayOfInt64(buffer, count) : ReadArrayOfInt64(buffer, count);
#else
    return MustSwapBytes() ? ReadAndConvertArrayOfInt32((int32_t*)buffer, count) : ReadArrayOfInt32((int32_t*)buffer, count);
#endif
}

size_t DataStream::ReadArrayOfIntPtr32(intptr_t *buffer, size_t count)
{
    // Read 32-bit values to array; this will always be safe,
    // because array is either 32-bit or 64-bit; in the last
    // case only first half of array will be used.
    count = ReadArrayOfInt32((int32_t*)buffer, count);

    if (count == 0)
    {
        return 0;
    }

#if defined (AGS_64BIT) || defined (TEST_64BIT)
    {
        // If we need 64-bit array, then copy 32-bit values to their
        // correct 64-bit slots, starting from the last element and
        // moving towards array head.
        int32_t *buffer32 = (int32_t*)buffer;
        buffer   += count - 1;
        buffer32 += count - 1;
        for (int i = count - 1; i >= 0; --i, --buffer, --buffer32)
        {
            // Ensure correct endianess-dependent positions; note that the
            // value bytes are already properly set by ReadArrayOfInt32
            if (_callerEndianess == kBigEndian)
            {
                *buffer = ((int64_t)*buffer32) << 32;
            }
            else
            {
                *buffer = *buffer32 & 0xFFFFFFFF;
            }
        }
    }
#endif // AGS_64BIT
    return count;
}

size_t DataStream::WriteArrayOfIntPtr(const intptr_t *buffer, size_t count)
{
#if defined (AGS_64BIT) || defined (TEST_64BIT)
    return MustSwapBytes() ? WriteAndConvertArrayOfInt64(buffer, count) : WriteArrayOfInt64(buffer, count);
#else
    return MustSwapBytes() ? WriteAndConvertArrayOfInt32((const int32_t*)buffer, count) : WriteArrayOfInt32((const int32_t*)buffer, count);
#endif
}

size_t DataStream::WriteArrayOfIntPtr32(const intptr_t *buffer, size_t count)
{
    if (!CanWrite())
    {
        return 0;
    }

    size_t elem;
    for (elem = 0; elem < count && !EOS(); ++elem, ++buffer)
    {
        int32_t val = (int32_t)*buffer;
        if (WriteInt32(val) < sizeof(int32_t))
        {
            break;
        }
    }
    return elem;
}

size_t DataStream::ReadAndConvertArrayOfInt16(int16_t *buffer, size_t count)
{
    count = ReadArray(buffer, sizeof(int16_t), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        BBOp::SwapBytesInt16(*buffer);
    }
    return count;
}

size_t DataStream::ReadAndConvertArrayOfInt32(int32_t *buffer, size_t count)
{
    count = ReadArray(buffer, sizeof(int32_t), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        BBOp::SwapBytesInt32(*buffer);
    }
    return count;
}

size_t DataStream::ReadAndConvertArrayOfInt64(int64_t *buffer, size_t count)
{
    count = ReadArray(buffer, sizeof(int64_t), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        BBOp::SwapBytesInt64(*buffer);
    }
    return count;
}

size_t DataStream::WriteAndConvertArrayOfInt16(const int16_t *buffer, size_t count)
{
    if (!CanWrite())
    {
        return 0;
    }

    size_t elem;
    for (elem = 0; elem < count && !EOS(); ++elem, ++buffer)
    {
        int16_t val = *buffer;
        ConvertInt16(val);
        if (Write(&val, sizeof(int16_t)) < sizeof(int16_t))
        {
            break;
        }
    }
    return elem;
}

size_t DataStream::WriteAndConvertArrayOfInt32(const int32_t *buffer, size_t count)
{
    if (!CanWrite())
    {
        return 0;
    }

    size_t elem;
    for (elem = 0; elem < count && !EOS(); ++elem, ++buffer)
    {
        int32_t val = *buffer;
        ConvertInt32(val);
        if (Write(&val, sizeof(int32_t)) < sizeof(int32_t))
        {
            break;
        }
    }
    return elem;
}

size_t DataStream::WriteAndConvertArrayOfInt64(const int64_t *buffer, size_t count)
{
    if (!CanWrite())
    {
        return 0;
    }

    size_t elem;
    for (elem = 0; elem < count && !EOS(); ++elem, ++buffer)
    {
        int64_t val = *buffer;
        ConvertInt64(val);
        if (Write(&val, sizeof(int64_t)) < sizeof(int64_t))
        {
            break;
        }
    }
    return elem;
}

int16_t DataStream::ReadInt16()
{
    int16_t val = 0;
    Read(&val, sizeof(int16_t));
    ConvertInt16(val);
    return val;
}

int32_t DataStream::ReadInt32()
{
    int32_t val = 0;
    Read(&val, sizeof(int32_t));
    ConvertInt32(val);
    return val;
}

int64_t DataStream::ReadInt64()
{
    int64_t val = 0;
    Read(&val, sizeof(int64_t));
    ConvertInt64(val);
    return val;
}

size_t DataStream::WriteInt16(int16_t val)
{
    ConvertInt16(val);
    return Write(&val, sizeof(int16_t));
}

size_t DataStream::WriteInt32(int32_t val)
{
    ConvertInt32(val);
    return Write(&val, sizeof(int32_t));
}

size_t DataStream::WriteInt64(int64_t val)
{
    ConvertInt64(val);
    return Write(&val, sizeof(int64_t));
}

} // namespace Common
} // namespace AGS
