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
#include "util/transformstream.h"
#include <cstring>
#include <stdexcept>

namespace AGS
{
namespace Common
{

const size_t TransformStream::BufferSize; // some compilers require this definition for linking

TransformStream::TransformStream(std::unique_ptr<IStreamBase> &&base_stream, StreamMode mode)
{
    Open(std::move(base_stream), mode);
}

TransformStream::TransformStream(std::unique_ptr<IStreamBase> &&base_stream, soff_t begin_pos, soff_t end_pos)
{
    Open(std::move(base_stream), kStream_Read);
    _baseBeginPos = begin_pos;
    _baseEndPos = std::min(end_pos, _baseEndPos);
    _base->Seek(_baseBeginPos, kSeekBegin);
}

void TransformStream::Open(std::unique_ptr<IStreamBase> &&base_stream, StreamMode mode)
{
    if (!base_stream)
        throw std::runtime_error("Base stream invalid.");
    mode = static_cast<StreamMode>(mode & kStream_ReadWrite);
    if (mode == kStream_ReadWrite)
        throw std::runtime_error("TransformStream cannot work in read/write mode.");

    soff_t cur_pos = base_stream->GetPosition();
    soff_t base_end = cur_pos;
    if (mode == kStream_Read)
    {
        base_end = base_stream->Seek(0, kSeekEnd);
        if (base_end < 0)
            throw std::runtime_error("Error determining stream end.");
        if (base_stream->Seek(cur_pos, kSeekBegin) < 0)
            throw std::runtime_error("Error determining stream end.");
    }

    _base = std::move(base_stream);
    _mode = mode;
    _baseBeginPos = cur_pos;
    _baseEndPos = base_end;
    _inBuffer.resize(BufferSize);
    _outBuffer.resize(BufferSize);
}

TransformStream::~TransformStream()
{
    CloseOnDisposal();
}

std::unique_ptr<IStreamBase> TransformStream::ReleaseStreamBase()
{
    if (_base && CanWrite())
        WriteBuffer(true);
    return std::move(_base);
}

bool TransformStream::EOS() const
{
    // When writing we are always at the end (transform stream does not seek)
    if (_mode == kStream_Write)
        return true;
    // When reading, transform stream is at the end when:
    // - base stream is at the end
    // - output buffer is fully read out
    return _base && (_base->GetPosition() >= _baseEndPos) && (_outBufPos == _outBufEnd);
}

void TransformStream::Close()
{
    if (_base)
    {
        if (CanWrite())
            WriteBuffer(true);
        _base->Close();
    }
}

bool TransformStream::Flush()
{
    if (_base)
    {
        if (CanWrite())
            WriteBuffer(false);
        return _base->Flush();
    }
    return false;
}

void TransformStream::Finalize()
{
    if (_base)
    {
        if (CanWrite())
            WriteBuffer(true);
        _base->Flush();
    }
}

void TransformStream::ReadBuffer()
{
    assert(_base);
    if (!_base)
        return;

    // Reset the output buffer, and try filling it to the max possible
    // by un-transforming data read from the input stream
    _outBufEnd = 0u;
    _outBufPos = 0u;

    do
    {
        // If the input buffer is all read out, then fill the buffer from the input stream
        if (_inBufPos == _inBufEnd)
        {
            size_t fill_size = std::min<size_t>(static_cast<size_t>(_baseEndPos - _base->GetPosition()), BufferSize);
            _inBufEnd = _base->Read(_inBuffer.data(), fill_size);
            _inBufPos = 0u;
        }

        // UN-transform
        size_t in_read = 0u, out_wrote = 0u;
        _lastResult =
            UnTransform(_inBuffer.data() + _inBufPos, _inBufEnd - _inBufPos,
                    _outBuffer.data() + _outBufEnd, BufferSize - _outBufEnd,
                    (_base->GetPosition() >= _baseEndPos), in_read, out_wrote);
        _inBufPos += in_read;
        _outBufEnd += out_wrote;

        // Break early if transformation could not continue,
        // this may also happen if remaining free space in the output buffer is not enough
        if (_lastResult != TransformResult::OK)
            break;
    }
    while ((_outBufEnd < BufferSize) && (_base->GetPosition() < _baseEndPos));
}

void TransformStream::WriteBuffer(bool finalize)
{
    assert(_base);
    if (!_base)
        return;

    if (_lastResult != TransformResult::OK && _lastResult != TransformResult::Buffer)
        return; // writing either complete or there was a error

    do
    {
        // Transform
        size_t in_read = 0u, out_wrote = 0u;
        _lastResult =
            Transform(_inBuffer.data() + _inBufPos, _inBufEnd - _inBufPos,
                  _outBuffer.data() + _outBufEnd, BufferSize - _outBufEnd,
                  finalize, in_read, out_wrote);
        _inBufPos += in_read;
        _outBufEnd += out_wrote;

        // If output buffer was filled, OR we are finalizing and input is
        // all read up, OR if there's not enough output buffer remaining
        // for the transform to continue,
        // then write it to the underlying stream, and reset
        if ((_outBufEnd == BufferSize)
            || (_lastResult != TransformResult::OK))
        {
            _base->Write(_outBuffer.data(), _outBufEnd);
            _outBufEnd = 0u;
        }
    }
    while (_inBufPos < _inBufEnd);

    // Reset input buffer
    _inBufPos = 0;
    _inBufEnd = 0;
}

void TransformStream::CloseOnDisposal()
{
    if (_base)
    {
        if (CanWrite() && (_base->GetMode() & kStream_Write) == kStream_Write)
            _base->Write(_outBuffer.data(), _outBufEnd);
        _base->Close();
    }
}

size_t TransformStream::Read(void *buffer, size_t size)
{
    // TODO: special case if the reading size is >= than our internal buffer,
    // see BufferedStream for example.

    auto *to = static_cast<uint8_t *>(buffer);
    while (size > 0)
    {
        if (_outBufPos >= _outBufEnd)
            ReadBuffer();
        if (_outBufPos == _outBufEnd)
            break; // reached EOS

        size_t bytes_left = _outBufEnd - _outBufPos;
        size_t chunk_sz = std::min<size_t>(bytes_left, size);

        std::memcpy(to, _outBuffer.data() + _outBufPos, chunk_sz);

        to += chunk_sz;
        _outBufPos += chunk_sz;
        size -= chunk_sz;
        _inputProcessed += chunk_sz; // returned untransformed bytes
    }

    return to - static_cast<uint8_t *>(buffer);
}

int32_t TransformStream::ReadByte()
{
    uint8_t ch;
    auto bytesRead = Read(&ch, 1);
    if (bytesRead != 1) { return EOF; }
    return ch;
}

size_t TransformStream::Write(const void *buffer, size_t size)
{
    const uint8_t *from = static_cast<const uint8_t *>(buffer);
    while (size > 0)
    {
        if (_inBufEnd == static_cast<soff_t>(BufferSize))
        {
            WriteBuffer(false);
        }
        size_t chunk_sz = std::min(size, BufferSize - _inBufEnd);
        std::memcpy(_inBuffer.data() + _inBufEnd, from, chunk_sz);
        _inBufEnd += chunk_sz;
        from += chunk_sz;
        size -= chunk_sz;
        _inputProcessed += chunk_sz; // put untransformed bytes
    }

    return from - static_cast<const uint8_t *>(buffer);
}

int32_t TransformStream::WriteByte(uint8_t val)
{
    auto sz = Write(&val, 1);
    if (sz != 1) { return -1; }
    return val;
}

soff_t TransformStream::Seek(soff_t offset, StreamSeek origin)
{
    // Remember that seek position is counted in *untransformed* bytes
    soff_t want_pos = -1;
    switch (origin)
    {
    case StreamSeek::kSeekCurrent:  want_pos = _inputProcessed + offset; break;
    case StreamSeek::kSeekBegin:    want_pos = 0 + offset; break;
    case StreamSeek::kSeekEnd:
    default:
        assert(false);
        return -1;
    }

    assert(want_pos >= _inputProcessed);
    if (want_pos < _inputProcessed)
        return -1;

    for (; want_pos > _inputProcessed; /* _inputProcessed is incremented inside ReadByte() */)
    {
        if (ReadByte() < 0)
            break;
    }
    return _inputProcessed;
}

} // namespace Common
} // namespace AGS
