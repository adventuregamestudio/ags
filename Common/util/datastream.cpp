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
#include <algorithm>
#include "util/datastream.h"

namespace AGS
{
namespace Common
{

DataStream::DataStream(DataEndianess stream_endianess)
    : _streamEndianess(stream_endianess)
{
}

DataStream::~DataStream() = default;

int8_t DataStream::ReadInt8()
{
    int8_t val = 0;
    Read(&val, sizeof(int8_t));
    return val;
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

size_t DataStream::WriteInt8(int8_t val)
{
    return Write(&val, sizeof(int8_t));
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

size_t DataStream::ReadAndConvertArrayOfInt16(int16_t *buffer, size_t count)
{
    count = ReadArray(buffer, sizeof(int16_t), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        *buffer = BBOp::SwapBytesInt16(*buffer);
    }
    return count;
}

size_t DataStream::ReadAndConvertArrayOfInt32(int32_t *buffer, size_t count)
{
    count = ReadArray(buffer, sizeof(int32_t), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        *buffer = BBOp::SwapBytesInt32(*buffer);
    }
    return count;
}

size_t DataStream::ReadAndConvertArrayOfInt64(int64_t *buffer, size_t count)
{
    count = ReadArray(buffer, sizeof(int64_t), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        *buffer = BBOp::SwapBytesInt64(*buffer);
    }
    return count;
}

size_t DataStream::WriteAndConvertArrayOfInt16(const int16_t *buffer, size_t count)
{
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


DataStreamSection::DataStreamSection(Stream *base, soff_t start, soff_t end)
    : _base(base)
{
    _start = std::max((soff_t)0, std::min(start, end));
    _end = std::min(std::max((soff_t)0, end), base->GetLength());

    soff_t pos = base->Seek(_start, kSeekBegin);
    if (pos >= 0)
        _position = pos;
    else
        _position = base->GetPosition();
}

size_t DataStreamSection::Read(void *buffer, size_t len)
{
    if (_position >= _end)
        return 0;
    len = std::min(len, static_cast<size_t>(_end - _position));
    _position += _base->Read(buffer, len);
    return len;
}

int32_t DataStreamSection::ReadByte()
{
    if (_position >= _end)
        return -1;
    int32_t b = _base->ReadByte();
    if (b >= 0)
        _position++;
    return b;
}

size_t DataStreamSection::Write(const void *buffer, size_t len)
{
    len = _base->Write(buffer, len);
    _position += len;
    _end = std::max(_end, _position); // we might be overwriting after seeking back
    return len;
}

int32_t DataStreamSection::WriteByte(uint8_t b)
{
    int32_t rb = _base->WriteByte(b);
    if (rb == b)
    {
        _position++;
        _end = std::max(_end, _position); // we might be overwriting after seeking back
    }
    return rb;
}

soff_t DataStreamSection::Seek(soff_t offset, StreamSeek origin)
{
    soff_t want_pos = -1;
    switch(origin)
    {
        case StreamSeek::kSeekCurrent:  want_pos = _position   + offset; break;
        case StreamSeek::kSeekBegin:    want_pos = _start      + offset; break;
        case StreamSeek::kSeekEnd:      want_pos = _end        + offset; break;
        default: return -1;
    }

    want_pos = std::min(std::max(want_pos, _start), _end);
    soff_t new_pos = _base->Seek(want_pos, kSeekBegin);
    if (new_pos >= 0) // the position remains in case of seek error
        _position = want_pos;
    return _position - _start; // convert to a stream section pos
}

void DataStreamSection::Close()
{
    // We do not close nor delete the base stream, but release it from use
    _base = nullptr;
}

} // namespace Common
} // namespace AGS
