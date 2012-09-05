
//=============================================================================
//
// Standard AGS stream with support for converting data to opposite endianess.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__AGSSTREAM_H
#define __AGS_CN_UTIL__AGSSTREAM_H

#include "util/bbop.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

class CAGSStream : public IStream
{
public:
    CAGSStream(DataEndianess caller_endianess, DataEndianess stream_endianess)
        : _callerEndianess(caller_endianess)
        , _streamEndianess(stream_endianess)
    {

    }
    virtual ~CAGSStream(){}

    virtual int Read(void *buffer, int size) = 0;
    inline virtual int ReadArray(void *buffer, int elem_size, int count)
    {
        return Read(buffer, elem_size * count);
    }
    
    virtual int Write(const void *buffer, int size) = 0;
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

#endif // __AGS_CN_UTIL__AGSSTREAM_H
