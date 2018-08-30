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
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__PROXYSTREAM_H
#define __AGS_CN_UTIL__PROXYSTREAM_H

#include "util/stream.h"

namespace AGS
{
namespace Common
{

// TODO: move to more common header
enum ObjectOwnershipPolicy
{
    kReleaseAfterUse,
    kDisposeAfterUse
};

class ProxyStream : public Stream
{
public:
    ProxyStream(Stream *stream, ObjectOwnershipPolicy stream_ownership_policy = kReleaseAfterUse);
    virtual ~ProxyStream();

    virtual void    Close();
    virtual bool    Flush();

    // Is stream valid (underlying data initialized properly)
    virtual bool    IsValid() const;
    // Is end of stream
    virtual bool    EOS() const;
    // Total length of stream (if known)
    virtual soff_t  GetLength() const;
    // Current position (if known)
    virtual soff_t  GetPosition() const;

    virtual bool    CanRead() const;
    virtual bool    CanWrite() const;
    virtual bool    CanSeek() const;

    virtual size_t  Read(void *buffer, size_t size);
    virtual int32_t ReadByte();
    virtual int16_t ReadInt16();
    virtual int32_t ReadInt32();
    virtual int64_t ReadInt64();
    virtual size_t  ReadArray(void *buffer, size_t elem_size, size_t count);
    virtual size_t  ReadArrayOfInt16(int16_t *buffer, size_t count);
    virtual size_t  ReadArrayOfInt32(int32_t *buffer, size_t count);
    virtual size_t  ReadArrayOfInt64(int64_t *buffer, size_t count);

    virtual size_t  Write(const void *buffer, size_t size);
    virtual int32_t WriteByte(uint8_t b);
    virtual size_t  WriteInt16(int16_t val);
    virtual size_t  WriteInt32(int32_t val);
    virtual size_t  WriteInt64(int64_t val);
    virtual size_t  WriteArray(const void *buffer, size_t elem_size, size_t count);
    virtual size_t  WriteArrayOfInt16(const int16_t *buffer, size_t count);
    virtual size_t  WriteArrayOfInt32(const int32_t *buffer, size_t count);
    virtual size_t  WriteArrayOfInt64(const int64_t *buffer, size_t count);

    virtual soff_t  Seek(soff_t offset, StreamSeek origin);

protected:
    Stream                  *_stream;
    ObjectOwnershipPolicy   _streamOwnershipPolicy;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__PROXYSTREAM_H
