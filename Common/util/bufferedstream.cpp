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
#include <cstring>
#include <stdexcept>
#include "util/bufferedstream.h"
#include "util/stdio_compat.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

//-----------------------------------------------------------------------------
// BufferedStream
//-----------------------------------------------------------------------------

const size_t BufferedStream::BufferSize;

void BufferedStream::Open(std::unique_ptr<IStreamBase> &&base_stream)
{
    if (!base_stream)
        throw std::runtime_error("Base stream invalid.");
    soff_t end_pos = base_stream->Seek(0, kSeekEnd);
    if  (end_pos < 0)
        throw std::runtime_error("Error determining stream end.");
    if (base_stream->Seek(0, kSeekBegin) < 0)
        throw std::runtime_error("Error determining stream end.");

    _base = std::move(base_stream);
    _start = 0;
    _end = end_pos;
}

void BufferedStream::OpenSection(std::unique_ptr<IStreamBase> &&base_stream,
    soff_t start_pos, soff_t end_pos)
{
    assert(start_pos <= end_pos);
    Open(std::move(base_stream));
    start_pos = std::min(start_pos, end_pos);
    _start = std::min(start_pos, _end);
    _end = std::min(end_pos, _end);
    Seek(0, kSeekBegin);
}

BufferedStream::~BufferedStream()
{
    Close();
}

void BufferedStream::FillBufferFromPosition(soff_t position)
{
    _base->Seek(position, kSeekBegin);
    // remember to restrict to the end position!
    size_t fill_size = static_cast<size_t>(
        std::min<uint64_t>(BufferSize, static_cast<uint64_t>(_end - position)));
    _buffer.resize(fill_size);
    auto sz = _base->Read(_buffer.data(), fill_size);
    _buffer.resize(sz);
    _bufferPosition = position;
}

void BufferedStream::FlushBuffer(soff_t position)
{
    size_t sz = _buffer.size() > 0 ? _base->Write(_buffer.data(), _buffer.size()) : 0u;
    _buffer.clear(); // will start from the clean buffer next time
    _bufferPosition += sz;
    if (position != _bufferPosition)
    {
        _base->Seek(position, kSeekBegin);
        _bufferPosition = position;
    }
}

bool BufferedStream::EOS() const
{
    return _position == _end;
}

soff_t BufferedStream::GetLength() const
{
    return _end - _start;
}

soff_t BufferedStream::GetPosition() const
{
    return _position - _start;
}

void BufferedStream::Close()
{
    if (CanWrite())
        FlushBuffer(_position);
    _base->Close();
}

bool BufferedStream::Flush()
{
    if (CanWrite())
        FlushBuffer(_position);
    return _base->Flush();
}

size_t BufferedStream::Read(void *buffer, size_t size)
{
    // If the read size is larger than the internal buffer size,
    // then read directly into the user buffer and bail out.
    if (size >= BufferSize)
    {
        _base->Seek(_position, kSeekBegin);
        // remember to restrict to the end position!
        size_t fill_size = static_cast<size_t>(
            std::min<uint64_t>(size, static_cast<uint64_t>(_end - _position)));
        size_t sz = _base->Read(buffer, fill_size);
        _position += sz;
        return sz;
    }

    auto *to = static_cast<uint8_t*>(buffer);
    while(size > 0)
    {
        if (_position < _bufferPosition ||
            static_cast<uint64_t>(_position) >= static_cast<uint64_t>(_bufferPosition + _buffer.size()))
        {
            FillBufferFromPosition(_position);
        }
        if (_buffer.empty()) { break; } // reached EOS
        assert(_position >= _bufferPosition &&
            static_cast<uint64_t>(_position) < static_cast<uint64_t>(_bufferPosition + _buffer.size()));

        soff_t bufferOffset = _position - _bufferPosition;
        assert(bufferOffset >= 0);
        size_t bytesLeft = _buffer.size() - (size_t)bufferOffset;
        size_t chunkSize = std::min<size_t>(bytesLeft, size);

        std::memcpy(to, _buffer.data() + bufferOffset, chunkSize);

        to += chunkSize;
        _position += chunkSize;
        size -= chunkSize;
    }
    return to - static_cast<uint8_t*>(buffer);
}

int32_t BufferedStream::ReadByte()
{
    uint8_t ch;
    auto bytesRead = Read(&ch, 1);
    if (bytesRead != 1) { return EOF; }
    return ch;
}

size_t BufferedStream::Write(const void *buffer, size_t size)
{
    const uint8_t *from = static_cast<const uint8_t*>(buffer);
    while (size > 0)
    {
        if (_position < _bufferPosition || // seeked before buffer pos
            _position > _bufferPosition + static_cast<soff_t>(_buffer.size()) || // seeked beyond buffer pos
            _position >= _bufferPosition + static_cast<soff_t>(BufferSize)) // seeked, or exceeded buffer limit
        {
            FlushBuffer(_position);
        }
        size_t pos_in_buff = static_cast<size_t>(_position - _bufferPosition);
        size_t chunk_sz = std::min(size, BufferSize - pos_in_buff);
        if (_buffer.size() < pos_in_buff + chunk_sz)
            _buffer.resize(pos_in_buff + chunk_sz);
        memcpy(_buffer.data() + pos_in_buff, from, chunk_sz);
        _position += chunk_sz;
        from += chunk_sz;
        size -= chunk_sz;
    }

    _end = std::max(_end, _position);
    return from - static_cast<const uint8_t*>(buffer);
}

int32_t BufferedStream::WriteByte(uint8_t val)
{
    auto sz = Write(&val, 1);
    if (sz != 1) { return -1; }
    return val;
}

soff_t BufferedStream::Seek(soff_t offset, StreamSeek origin)
{
    soff_t want_pos = -1;
    switch(origin)
    {
        case StreamSeek::kSeekCurrent:  want_pos = _position   + offset; break;
        case StreamSeek::kSeekBegin:    want_pos = _start      + offset; break;
        case StreamSeek::kSeekEnd:      want_pos = _end        + offset; break;
        default: return -1;
    }
    // clamp to the valid range
    _position = std::min(std::max(want_pos, _start), _end);
    return _position - _start; // convert to a stream section pos
}

} // namespace Common
} // namespace AGS
