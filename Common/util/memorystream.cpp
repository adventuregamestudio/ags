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
#include "util/memorystream.h"
#include <algorithm>
#include <string.h>

namespace AGS
{
namespace Common
{

MemoryStream::MemoryStream(const uint8_t *cbuf, size_t buf_sz, DataEndianess stream_endianess)
    : DataStream(stream_endianess)
    , _cbuf(cbuf)
    , _buf_sz(buf_sz)
    , _len(buf_sz)
    , _pos(0)
    , _buf(nullptr)
{
    if (cbuf)
        _mode = static_cast<StreamMode>(kStream_Read | kStream_Seek);
}

MemoryStream::MemoryStream(uint8_t *buf, size_t buf_sz, StreamMode mode, DataEndianess stream_endianess)
    : DataStream(stream_endianess)
    , _cbuf(nullptr)
    , _buf_sz(buf_sz)
    , _len(0)
    , _pos(0)
    , _buf(nullptr)
{
    if ((mode & kStream_Write) != 0)
    {
        _buf = buf;
    }
    else
    {
        _cbuf = buf;
        _len = buf_sz;
    }

    if (buf)
        _mode = static_cast<StreamMode>(mode | kStream_Seek);
}

void MemoryStream::Close()
{
    _cbuf = nullptr;
    _buf = nullptr;
    _buf_sz = 0;
    _len = 0;
    _pos = 0;
    _mode = kStream_None;
}

bool MemoryStream::Flush()
{
    return true;
}

bool MemoryStream::EOS() const
{
    return _pos >= _len;
}

soff_t MemoryStream::GetLength() const
{
    return _len;
}

soff_t MemoryStream::GetPosition() const
{
    return _pos;
}

size_t MemoryStream::Read(void *buffer, size_t size)
{
    if (EOS()) { return 0; }
    assert(_len > _pos);
    size_t remain = _len - _pos;
    size_t read_sz = std::min(remain, size);
    memcpy(buffer, _cbuf + _pos, read_sz);
    _pos += read_sz;
    return read_sz;
}

int32_t MemoryStream::ReadByte()
{
    if (EOS()) { return -1; }
    return _cbuf[_pos++];
}

soff_t MemoryStream::Seek(soff_t offset, StreamSeek origin)
{
    soff_t want_pos = -1;
    switch (origin)
    {
    case kSeekBegin:    want_pos = 0 + offset; break;
    case kSeekCurrent:  want_pos = _pos + offset; break;
    case kSeekEnd:      want_pos = _len + offset; break;
    default: return -1;
    }
    // clamp to a valid range
    _pos = static_cast<size_t>(std::max<soff_t>(0, want_pos));
    _pos = std::min(_len, _pos);
    return static_cast<soff_t>(_pos);
}

size_t MemoryStream::Write(const void *buffer, size_t size)
{
    if (!_buf || (_pos >= _buf_sz)) { return 0; }
    size = std::min(size, _buf_sz - _pos);
    memcpy(_buf + _pos, buffer, size);
    _pos += size;
    // will increase len if writing after eos, otherwise = overwrite at pos
    _len = std::max(_len, _pos);
    return size;
}

int32_t MemoryStream::WriteByte(uint8_t val)
{
    if (!_buf || (_pos >= _buf_sz)) { return -1; }
    *(_buf + _pos) = val;
    _pos++;
    // will increase len if writing after eos, otherwise = overwrite at pos
    _len = std::max(_len, _pos);
    return val;
}


VectorStream::VectorStream(const std::vector<uint8_t> &cbuf, DataEndianess stream_endianess)
    : MemoryStream(&cbuf.front(), cbuf.size(), stream_endianess)
    , _vec(nullptr)
{
    _mode = static_cast<StreamMode>(kStream_Read | kStream_Seek);
}

VectorStream::VectorStream(std::vector<uint8_t> &buf, StreamMode mode, DataEndianess stream_endianess)
    : MemoryStream((((mode & kStream_ReadWrite) == kStream_Read) && (buf.size() > 0) ?
            &buf.front() : nullptr),
        buf.size(), mode, stream_endianess)
    , _vec(&buf)
{
    _mode = static_cast<StreamMode>(mode | kStream_Seek);
}

void VectorStream::Close()
{
    _vec = nullptr;
    MemoryStream::Close();
}

size_t VectorStream::Write(const void *buffer, size_t size)
{
    if (_pos + size > _len)
    {
        _vec->resize(_pos + size);
        _len = _pos + size;
    }
    memcpy(_vec->data() + _pos, buffer, size);
    _pos += size;
    return size;
}

int32_t VectorStream::WriteByte(uint8_t val)
{
    if (_pos == _len)
    {
        _vec->push_back(val);
        _len++;
    }
    else
    {
        (*_vec)[_pos] = val;
    }
    _pos++;
    return val;
}

} // namespace Common
} // namespace AGS
