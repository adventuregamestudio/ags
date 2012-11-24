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
// Class AlignedStream
// A simple wrapper around stream that controls data padding.
// 
// Originally, a number of objects in AGS were read and written directly
// as a data struct in a whole. In order to support backwards compatibility
// with games made by older versions of AGS, some of the game objects must
// be read having automatic data alignment in mind.
//-----------------------------------------------------------------------------
//
// AlignedStream uses the underlying stream, it overrides the reading and
// writing, and inserts extra data padding when needed.
//
// Aligned stream works either in read or write mode, it cannot be opened in
// combined mode.
//
// AlignedStream does not support seek, hence moving stream pointer to random
// position will break padding count logic.
//
// A Close() method must be called either explicitly by user or implicitly by
// stream destructor in order to read/write remaining padding bytes.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__ALIGNEDSTREAM_H
#define __AGS_CN_UTIL__ALIGNEDSTREAM_H

#include "util/stream.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class DataStream;

enum AlignedStreamMode
{
    kAligned_Read,
    kAligned_Write
};

class AlignedStream : public Stream
{
public:
    // TODO: use shared ptr
    AlignedStream(DataStream *stream, AlignedStreamMode mode, size_t alignment = sizeof(int32_t));
    virtual ~AlignedStream();

    // Is stream valid (underlying data initialized properly)
    virtual bool    IsValid() const;
    // Is end of stream
    virtual bool    EOS() const;
    // Total length of stream (if known)
    virtual size_t  GetLength() const;
    // Current position (if known)
    virtual size_t  GetPosition() const;
    virtual bool    CanRead() const;
    virtual bool    CanWrite() const;
    virtual bool    CanSeek() const;

    // Free the underlying stream (to prevent automatic dispose on close)
    // TODO: use shared ptr instead
    void            ReleaseStream();
    // Close() MUST be called at the end of aligned stream work so that the
    // stream could read/write the necessary padding at the end of data chunk;
    // Close() will be called automatically from stream's destructor, but is
    // exposed in case user would like to explicitly end aligned read/write at
    // certain point.
    virtual void    Close();

    virtual int     ReadByte();
    inline  int8_t  ReadInt8()
    {
        return ReadByte();
    }
    int16_t ReadInt16();
    int32_t ReadInt32();
    int64_t ReadInt64();
    virtual size_t  Read(void *buffer, size_t size);
    size_t  ReadArray(void *buffer, size_t elem_size, size_t count);
    virtual String  ReadString(size_t max_chars = 5000000);

    virtual int     WriteByte(uint8_t b);
    inline  void    WriteInt8(int8_t val)
    {
        WriteByte(val);
    }
    size_t  WriteInt16(int16_t val);
    size_t  WriteInt32(int32_t val);
    size_t  WriteInt64(int64_t val);
    virtual size_t  Write(const void *buffer, size_t size);
    size_t  WriteArray(const void *buffer, size_t elem_size, size_t count);
    size_t  WriteString(const String &str);

    virtual size_t  Seek(StreamSeek seek, int pos);

    size_t ReadArrayOfInt16(int16_t *buffer, size_t count);
    size_t ReadArrayOfInt32(int32_t *buffer, size_t count);
    size_t ReadArrayOfInt64(int64_t *buffer, size_t count);
    size_t WriteArrayOfInt16(const int16_t *buffer, size_t count);
    size_t WriteArrayOfInt32(const int32_t *buffer, int count);
    size_t WriteArrayOfInt64(const int64_t *buffer, int count);

protected:
    void            ReadPadding(size_t next_type);
    void            WritePadding(size_t next_type);

private:
    DataStream         *_stream;
    AlignedStreamMode   _mode;
    size_t              _alignment;
    int64_t             _block;
    int8_t              _paddingBuffer[sizeof(int64_t)];
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__ALIGNEDSTREAM_H
