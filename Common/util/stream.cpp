//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "util/stream.h"
#include <algorithm>
#include <stdexcept>

namespace AGS
{
namespace Common
{

//-----------------------------------------------------------------------------
// Stream
//-----------------------------------------------------------------------------

size_t Stream::ReadAndConvertArrayOfInt16(int16_t *buffer, size_t count)
{
    count = ReadArray(buffer, sizeof(int16_t), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        *buffer = BBOp::SwapBytesInt16(*buffer);
    }
    return count;
}

size_t Stream::ReadAndConvertArrayOfInt32(int32_t *buffer, size_t count)
{
    count = ReadArray(buffer, sizeof(int32_t), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        *buffer = BBOp::SwapBytesInt32(*buffer);
    }
    return count;
}

size_t Stream::ReadAndConvertArrayOfInt64(int64_t *buffer, size_t count)
{
    count = ReadArray(buffer, sizeof(int64_t), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        *buffer = BBOp::SwapBytesInt64(*buffer);
    }
    return count;
}

size_t Stream::ReadAndConvertArrayOfFloat32(float *buffer, size_t count)
{
    count = ReadArray(buffer, sizeof(float), count);
    for (size_t i = 0; i < count; ++i, ++buffer)
    {
        *buffer = BBOp::SwapBytesFloat32(*buffer);
    }
    return count;
}

size_t Stream::WriteAndConvertArrayOfInt16(const int16_t *buffer, size_t count)
{
    size_t elem;
    for (elem = 0; elem < count && !EOS(); ++elem, ++buffer)
    {
        int16_t val = *buffer;
        val = BBOp::SwapBytesInt16(val);
        if (Write(&val, sizeof(int16_t)) < sizeof(int16_t))
        {
            break;
        }
    }
    return elem;
}

size_t Stream::WriteAndConvertArrayOfInt32(const int32_t *buffer, size_t count)
{
    size_t elem;
    for (elem = 0; elem < count && !EOS(); ++elem, ++buffer)
    {
        int32_t val = *buffer;
        val = BBOp::SwapBytesInt32(val);
        if (Write(&val, sizeof(int32_t)) < sizeof(int32_t))
        {
            break;
        }
    }
    return elem;
}

size_t Stream::WriteAndConvertArrayOfInt64(const int64_t *buffer, size_t count)
{
    size_t elem;
    for (elem = 0; elem < count && !EOS(); ++elem, ++buffer)
    {
        int64_t val = *buffer;
        val = BBOp::SwapBytesInt64(val);
        if (Write(&val, sizeof(int64_t)) < sizeof(int64_t))
        {
            break;
        }
    }
    return elem;
}

size_t Stream::WriteAndConvertArrayOfFloat32(const float *buffer, size_t count)
{
    size_t elem;
    for (elem = 0; elem < count && !EOS(); ++elem, ++buffer)
    {
        float val = *buffer;
        val = BBOp::SwapBytesFloat32(val);
        if (Write(&val, sizeof(float)) < sizeof(float))
        {
            break;
        }
    }
    return elem;
}

size_t Stream::WriteByteCount(uint8_t b, size_t count)
{
    if (!CanWrite())
        return 0;
    size_t size = 0;
    for (; count > 0; --count, ++size)
    {
        if (WriteByte(b) < 0)
            break;
    }
    return size;
}

//-----------------------------------------------------------------------------
// StreamSection
//-----------------------------------------------------------------------------

StreamSection::StreamSection(std::unique_ptr<IStreamBase> &&base, soff_t start, soff_t end)
{
    OpenSection(base.get(), start, end);
    _ownBase = std::move(base);
    _base = _ownBase.get();
}

StreamSection::StreamSection(IStreamBase *base, soff_t start, soff_t end)
{
    OpenSection(base, start, end);
    _base = base;
}

void StreamSection::Open(IStreamBase *base)
{
    if (!base)
        throw std::runtime_error("Base stream invalid.");
    soff_t end_pos = base->Seek(0, kSeekEnd);
    if (end_pos < 0)
        throw std::runtime_error("Error determining stream end.");

    _start = 0;
    _end = end_pos;
}

void StreamSection::OpenSection(IStreamBase *base, soff_t start_pos, soff_t end_pos)
{
    assert(start_pos <= end_pos);
    Open(base);
    start_pos = std::min(start_pos, end_pos);
    _start = std::min(start_pos, _end);
    _end = std::min(end_pos, _end);

    soff_t pos = base->Seek(_start, kSeekBegin);
    if (pos >= 0)
        _position = pos;
    else
        _position = base->GetPosition();
}

void StreamSection::Close()
{
    // Only close the base stream if owning one
    if (_ownBase)
        _ownBase->Close();
}

size_t StreamSection::Read(void *buffer, size_t len)
{
    if (_position >= _end)
        return 0;
    len = std::min(len, static_cast<size_t>(_end - _position));
    _position += _base->Read(buffer, len);
    return len;
}

int32_t StreamSection::ReadByte()
{
    if (_position >= _end)
        return -1;
    int32_t b = _base->ReadByte();
    if (b >= 0)
        _position++;
    return b;
}

size_t StreamSection::Write(const void *buffer, size_t len)
{
    len = _base->Write(buffer, len);
    _position += len;
    _end = std::max(_end, _position); // we might be overwriting after seeking back
    return len;
}

int32_t StreamSection::WriteByte(uint8_t b)
{
    int32_t rb = _base->WriteByte(b);
    if (rb == b)
    {
        _position++;
        _end = std::max(_end, _position); // we might be overwriting after seeking back
    }
    return rb;
}

soff_t StreamSection::Seek(soff_t offset, StreamSeek origin)
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

//-----------------------------------------------------------------------------
// Stream helpers
//-----------------------------------------------------------------------------

soff_t CopyStream(Stream *in, Stream *out, soff_t length)
{
    char buf[4096];
    soff_t wrote_num = 0;
    while (length > 0)
    {
        size_t to_read = (size_t)std::min((soff_t)sizeof(buf), length);
        size_t was_read = in->Read(buf, to_read);
        if (was_read == 0)
            return wrote_num;
        length -= was_read;
        size_t to_write = was_read;
        while (to_write > 0)
        {
            size_t wrote = out->Write(buf + was_read - to_write, to_write);
            if (wrote == 0)
                return wrote_num;
            to_write -= wrote;
            wrote_num += wrote;
        }
    }
    return wrote_num;
}

} // namespace Common
} // namespace AGS
