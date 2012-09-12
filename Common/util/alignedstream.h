
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
    virtual int     GetLength() const;
    // Current position (if known)
    virtual int     GetPosition() const;
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
    virtual int16_t ReadInt16();
    virtual int32_t ReadInt32();
    virtual int64_t ReadInt64();
    virtual int     Read(void *buffer, int size);
    virtual int     ReadArray(void *buffer, int elem_size, int count);
    virtual String ReadString(int max_chars = 5000000);

    virtual int     WriteByte(byte b);
    inline  void    WriteInt8(int8_t val)
    {
        WriteByte(val);
    }
    virtual void    WriteInt16(int16_t val);
    virtual void    WriteInt32(int32_t val);
    virtual void    WriteInt64(int64_t val);
    virtual int     Write(const void *buffer, int size);
    virtual int     WriteArray(const void *buffer, int elem_size, int count);
    virtual void    WriteString(const String &str);

    virtual int     Seek(StreamSeek seek, int pos);

    inline int ReadArrayOfInt16(int16_t *buffer, int count)
    {
        return ReadArray(buffer, sizeof(int16_t), count);
    }
    inline int ReadArrayOfInt32(int32_t *buffer, int count)
    {
        return ReadArray(buffer, sizeof(int32_t), count);
    }
    inline int ReadArrayOfInt64(int64_t *buffer, int count)
    {
        return ReadArray(buffer, sizeof(int64_t), count);
    }
    inline int WriteArrayOfInt16(const int16_t *buffer, int count)
    {
        return WriteArray(buffer, sizeof(int16_t), count);
    }
    inline int WriteArrayOfInt32(const int32_t *buffer, int count)
    {
        return WriteArray(buffer, sizeof(int32_t), count);
    }
    inline int WriteArrayOfInt64(const int64_t *buffer, int count)
    {
        return WriteArray(buffer, sizeof(int64_t), count);
    }

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
