
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STREAM_H
#define __AGS_CN_UTIL__STREAM_H

#include "util/bbop.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

enum StreamSeek
{
    kSeekBegin,
    kSeekEnd,
    kSeekCurrent
};

class CStream
{
public:
    CStream(DataEndianess caller_endianess, DataEndianess stream_endianess)
        : _callerEndianess(caller_endianess)
        , _streamEndianess(stream_endianess)
    {

    }
    virtual ~CStream(){}

    // Is stream valid (underlying data initialized properly)
    virtual bool    IsValid() const = 0;
    // Is end of stream
    virtual bool    EOS() const = 0;
    // Total length of stream (if known)
    virtual int     GetLength() const = 0;
    // Current position (if known)
    virtual int     GetPosition() const = 0;
    virtual bool    CanRead() const = 0;
    virtual bool    CanWrite() const = 0;
    virtual bool    CanSeek() const = 0;

    virtual int     Seek(StreamSeek seek, int pos) = 0;

    virtual int8_t  ReadInt8()  = 0;
    virtual int16_t ReadInt16() = 0;
    virtual int32_t ReadInt32() = 0;
    virtual int64_t ReadInt64() = 0;
    virtual int     Read(void *buffer, int size) = 0;
    inline virtual int Read(void *buffer, int elem_size, int count)
    {
        return Read(buffer, elem_size * count);
    }
    virtual CString ReadString(int max_chars = 5000000) = 0;

    virtual void    WriteInt8(int8_t val)   = 0;
    virtual void    WriteInt16(int16_t val) = 0;
    virtual void    WriteInt32(int32_t val) = 0;
    virtual void    WriteInt64(int64_t val) = 0;
    virtual int     Write(const void *buffer, int size) = 0;
    inline virtual int Write(const void *buffer, int elem_size, int count)
    {
        return Write(buffer, elem_size * count);
    }
    virtual void    WriteString(const CString &str) = 0;

    inline int ReadArrayOfInt16(int16_t *buffer, int count)
    {
        return MustSwapBytes() ?
            ReadAndConvertArrayOfInt16(buffer, count) : Read(buffer, sizeof(int16_t), count);
    }
    inline int ReadArrayOfInt32(int32_t *buffer, int count)
    {
        return MustSwapBytes() ?
            ReadAndConvertArrayOfInt32(buffer, count) : Read(buffer, sizeof(int32_t), count);
    }
    inline int ReadArrayOfInt64(int64_t *buffer, int count)
    {
        return MustSwapBytes() ?
            ReadAndConvertArrayOfInt64(buffer, count) : Read(buffer, sizeof(int64_t), count);
    }
    inline int WriteArrayOfInt16(const int16_t *buffer, int count)
    {
        return MustSwapBytes() ?
            WriteAndConvertArrayOfInt16(buffer, count) : Write(buffer, sizeof(int16_t), count);
    }
    inline int WriteArrayOfInt32(const int32_t *buffer, int count)
    {
        return MustSwapBytes() ?
            WriteAndConvertArrayOfInt32(buffer, count) : Write(buffer, sizeof(int32_t), count);
    }
    inline int WriteArrayOfInt64(const int64_t *buffer, int count)
    {
        return MustSwapBytes() ?
            WriteAndConvertArrayOfInt64(buffer, count) : Write(buffer, sizeof(int64_t), count);
    }

protected:
    DataEndianess _callerEndianess;
    DataEndianess _streamEndianess;

    virtual int     ReadAndConvertArrayOfInt16(int16_t *buffer, int count) = 0;
    virtual int     ReadAndConvertArrayOfInt32(int32_t *buffer, int count) = 0;
    virtual int     ReadAndConvertArrayOfInt64(int64_t *buffer, int count) = 0;
    virtual int     WriteAndConvertArrayOfInt16(const int16_t *buffer, int count) = 0;
    virtual int     WriteAndConvertArrayOfInt32(const int32_t *buffer, int count) = 0;
    virtual int     WriteAndConvertArrayOfInt64(const int64_t *buffer, int count) = 0;

    inline bool MustSwapBytes()
    {
        return _callerEndianess != _streamEndianess;
    }

    inline void ConvertInt16(int16_t &val)
    {
        if (MustSwapBytes()) BBOp::SwapBytesInt16(val);
    }
    inline void ConvertInt32(int32_t &val)
    {
        if (MustSwapBytes()) BBOp::SwapBytesInt32(val);
    }
    inline void ConvertInt64(int64_t &val)
    {
        if (MustSwapBytes()) BBOp::SwapBytesInt64(val);
    }
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__STREAM_H
