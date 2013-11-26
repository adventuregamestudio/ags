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

DataStream::DataStream(DataEndianess stream_endianess)
    : _streamEndianess(stream_endianess)
{
}

DataStream::~DataStream()
{
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

size_t DataStream::ReadArrayOfIntPtr32(intptr_t *buffer, size_t count)
{
    if (!CanRead() || !buffer)
    {
        return 0;
    }

    int32_t* buf = sizeof(int32_t) == sizeof(intptr_t) ? (int32_t*) buffer \
                     : (int32_t*) malloc(count * sizeof(int32_t));
    if(!buf) return 0;

    count = ReadArrayOfInt32(buf, count);

    if (count == 0)
    {
        if(sizeof(int32_t) != sizeof(intptr_t)) free(buf);
        return 0;
    }
    if(sizeof(int32_t) != sizeof(intptr_t)) {
       for(size_t i = 0; i < count; i++) buffer[i] = buf[i];
       free(buf);
    }

    return count;
}

size_t DataStream::ReadAndConvertArrayOfInt16(int16_t *buffer, size_t count)
{
    if (!CanRead() || !buffer)
    {
        return 0;
    }

    count = ReadArray(buffer, sizeof(int16_t), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        BBOp::SwapBytesInt16(*buffer);
    }
    return count;
}

size_t DataStream::ReadAndConvertArrayOfInt32(int32_t *buffer, size_t count)
{
    if (!CanRead() || !buffer)
    {
        return 0;
    }

    count = ReadArray(buffer, sizeof(int32_t), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        BBOp::SwapBytesInt32(*buffer);
    }
    return count;
}

size_t DataStream::ReadAndConvertArrayOfInt64(int64_t *buffer, size_t count)
{
    if (!CanRead() || !buffer)
    {
        return 0;
    }

    count = ReadArray(buffer, sizeof(int64_t), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        BBOp::SwapBytesInt64(*buffer);
    }
    return count;
}

size_t DataStream::WriteAndConvertArrayOfInt16(const int16_t *buffer, size_t count)
{
    if (!CanWrite() || !buffer)
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
    if (!CanWrite() || !buffer)
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
    if (!CanWrite() || !buffer)
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

} // namespace Common
} // namespace AGS
