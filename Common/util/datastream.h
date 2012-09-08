
//=============================================================================
//
// Standard AGS stream base for reading raw data with support for converting to
// opposite endianess.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__DATASTREAM_H
#define __AGS_CN_UTIL__DATASTREAM_H

#include "util/bbop.h"
#include "util/stream.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class CDataStream : public IStream
{
public:
    CDataStream(DataEndianess caller_endianess, DataEndianess stream_endianess);
    virtual ~CDataStream();

    // Methods for reading basic types - implementation-specific
    inline  int8_t  ReadInt8()
    {
        return ReadByte();
    }
    virtual int16_t ReadInt16()             = 0;
    virtual int32_t ReadInt32()             = 0;
    virtual int64_t ReadInt64()             = 0;

    inline void WriteInt8(int8_t val)
    {
        WriteByte(val);
    }
    virtual void    WriteInt16(int16_t val) = 0;
    virtual void    WriteInt32(int32_t val) = 0;
    virtual void    WriteInt64(int64_t val) = 0;

    //-------------------------------------------------------------------------
    // Helper methods
    //-------------------------------------------------------------------------
    inline bool ReadBool()
    {
        return ReadInt8() != 0;
    }

    inline void WriteBool(bool val)
    {
        WriteInt8(val ? 1 : 0);
    }

    inline virtual int ReadArray(void *buffer, int elem_size, int count)
    {
        return Read(buffer, elem_size * count);
    }

    inline virtual int WriteArray(const void *buffer, int elem_size, int count)
    {
        return Write(buffer, elem_size * count);
    }

    inline int ReadArrayOfInt16(int16_t *buffer, int count)
    {
        return MustSwapBytes() ?
            ReadAndConvertArrayOfInt16(buffer, count) : ReadArray(buffer, sizeof(int16_t), count);
    }
    inline int ReadArrayOfInt32(int32_t *buffer, int count)
    {
        return MustSwapBytes() ?
            ReadAndConvertArrayOfInt32(buffer, count) : ReadArray(buffer, sizeof(int32_t), count);
    }
    inline int ReadArrayOfInt64(int64_t *buffer, int count)
    {
        return MustSwapBytes() ?
            ReadAndConvertArrayOfInt64(buffer, count) : ReadArray(buffer, sizeof(int64_t), count);
    }
    inline int WriteArrayOfInt16(const int16_t *buffer, int count)
    {
        return MustSwapBytes() ?
            WriteAndConvertArrayOfInt16(buffer, count) : WriteArray(buffer, sizeof(int16_t), count);
    }
    inline int WriteArrayOfInt32(const int32_t *buffer, int count)
    {
        return MustSwapBytes() ?
            WriteAndConvertArrayOfInt32(buffer, count) : WriteArray(buffer, sizeof(int32_t), count);
    }
    inline int WriteArrayOfInt64(const int64_t *buffer, int count)
    {
        return MustSwapBytes() ?
            WriteAndConvertArrayOfInt64(buffer, count) : WriteArray(buffer, sizeof(int64_t), count);
    }

    // Helper methods for reading and writing null-terminated string,
    // reading implies that string length is initially unknown.
    // DataStream class provides generic implementation, derived classes
    // may have their own optimized solutions.
    //
    // max_chars parameter determine the buffer size limit, however it
    // does not limit stream reading; the data will be read until null-
    // terminator or EOS is met, the buffer will contain only leftmost
    // part of the longer string that fits in.
    virtual CString ReadString(int max_chars = 5000000);
    virtual void    WriteString(const CString &str);

protected:
    DataEndianess _callerEndianess;
    DataEndianess _streamEndianess;

    // Helper methods for reading/writing arrays of basic types and
    // converting their elements to opposite endianess (swapping bytes).
    // DataStream class provides generic implementation, but derived
    // classes may have their own optimized solutions.
    virtual int     ReadAndConvertArrayOfInt16(int16_t *buffer, int count);
    virtual int     ReadAndConvertArrayOfInt32(int32_t *buffer, int count);
    virtual int     ReadAndConvertArrayOfInt64(int64_t *buffer, int count);
    virtual int     WriteAndConvertArrayOfInt16(const int16_t *buffer, int count);
    virtual int     WriteAndConvertArrayOfInt32(const int32_t *buffer, int count);
    virtual int     WriteAndConvertArrayOfInt64(const int64_t *buffer, int count);

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

#endif // __AGS_CN_UTIL__DATASTREAM_H
