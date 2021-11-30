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
#include "util/memorystream.h"
#include <algorithm>
#include <string.h>

namespace AGS
{
namespace Common
{

MemoryStream::MemoryStream(const std::vector<char> &cbuf, DataEndianess stream_endianess)
    : DataStream(stream_endianess)
    , _cbuf(&cbuf.front())
    , _len(cbuf.size())
    , _buf(nullptr)
    , _mode(kStream_Read)
    , _pos(0)
{
}

MemoryStream::MemoryStream(const String &cbuf, DataEndianess stream_endianess)
    : DataStream(stream_endianess)
    , _cbuf(cbuf.GetCStr())
    , _len(cbuf.GetLength())
    , _buf(nullptr)
    , _mode(kStream_Read)
    , _pos(0)
{
}

MemoryStream::MemoryStream(std::vector<char> &buf, StreamWorkMode mode, DataEndianess stream_endianess)
    : DataStream(stream_endianess)
    , _len(buf.size())
    , _buf(&buf)
    , _mode(mode)
    , _pos(buf.size())
{
    _cbuf = (mode == kStream_Read) ? &buf.front() : nullptr;
}

MemoryStream::~MemoryStream()
{
}

void MemoryStream::Close()
{
    _cbuf = nullptr;
    _buf = nullptr;
    _pos = -1;
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
    return CanRead(); // TODO: support seeking in writable stream?
}

size_t MemoryStream::Read(void *buffer, size_t size)
{
    if (EOS()) { return 0; }
    soff_t remain = _len - _pos;
    assert(remain > 0);
    size_t read_sz = std::min((size_t)remain, size);
    memcpy(buffer, _cbuf + _pos, read_sz);
    _pos += read_sz;
    return read_sz;
}

int32_t MemoryStream::ReadByte()
{
    if (EOS()) { return -1; }
    return static_cast<uint8_t>(_cbuf[(size_t)(_pos++)]);
}

size_t MemoryStream::Write(const void *buffer, size_t size)
{
    if (!_buf) { return 0; }
    _buf->resize(_buf->size() + size);
    memcpy(_buf->data() + _pos, buffer, size);
    _pos += size;
    _len += size;
    return size;
}

int32_t MemoryStream::WriteByte(uint8_t val)
{
    if (!_buf) { return -1; }
    _buf->push_back(val);
    _pos++; _len++;
    return val;
}

bool MemoryStream::Seek(soff_t offset, StreamSeek origin)
{
    if (!CanSeek()) { return false; }
    switch (origin)
    {
    case kSeekBegin:    _pos = 0 + offset; break;
    case kSeekCurrent:  _pos = _pos + offset; break;
    case kSeekEnd:      _pos = _len + offset; break;
    default:
        return false;
    }
    _pos = std::max<soff_t>(0, _pos);
    _pos = std::min<soff_t>(_len, _pos); // clamp to EOS
    return true;
}

} // namespace Common
} // namespace AGS
