//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
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
    , _mode(kStream_Read)
    , _pos(0)
    , _buf(nullptr)
{
}

MemoryStream::MemoryStream(uint8_t *buf, size_t buf_sz, StreamWorkMode mode, DataEndianess stream_endianess)
    : DataStream(stream_endianess)
    , _cbuf(nullptr)
    , _buf_sz(buf_sz)
    , _len(0)
    , _mode(mode)
    , _pos(0)
    , _buf(nullptr)
{
    if (mode == kStream_Read)
    {
        _cbuf = buf;
        _len = buf_sz;
    }
    else
    {
        _buf = buf;
    }
}

void MemoryStream::Close()
{
    _cbuf = nullptr;
    _buf = nullptr;
    _buf_sz = 0;
    _len = 0;
    _pos = 0;
}

bool MemoryStream::Flush()
{
    return true;
}

bool MemoryStream::IsValid() const
{
    return _cbuf != nullptr || _buf != nullptr;
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

bool MemoryStream::CanRead() const
{
    return (_cbuf != nullptr) && (_mode == kStream_Read);
}

bool MemoryStream::CanWrite() const
{
    return (_buf != nullptr) && (_mode == kStream_Write);
}

bool MemoryStream::CanSeek() const
{
    return true;
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

bool MemoryStream::Seek(soff_t offset, StreamSeek origin)
{
    if (!CanSeek()) { return false; }
    soff_t pos = 0;
    switch (origin)
    {
    case kSeekBegin:    pos = 0 + offset; break;
    case kSeekCurrent:  pos = _pos + offset; break;
    case kSeekEnd:      pos = _len + offset; break;
    default:
        return false;
    }
    _pos = static_cast<size_t>(std::max<soff_t>(0, pos));
    _pos = std::min(_len, _pos); // clamp to EOS
    return true;
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
}

VectorStream::VectorStream(std::vector<uint8_t> &buf, StreamWorkMode mode, DataEndianess stream_endianess)
    : MemoryStream(((mode == kStream_Read) && (buf.size() > 0)) ? &buf.front() : nullptr,
        buf.size(), mode, stream_endianess)
    , _vec(&buf)
{
}

void VectorStream::Close()
{
    _vec = nullptr;
    MemoryStream::Close();
}

bool VectorStream::CanRead() const
{
    return _mode == kStream_Read;
}

bool VectorStream::CanWrite() const
{
    return _mode == kStream_Write;
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
