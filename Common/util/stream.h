
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STREAM_H
#define __AGS_CN_UTIL__STREAM_H

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

class IStream
{
public:
    virtual ~IStream(){}

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
    virtual CString ReadString(int max_chars = 5000000) = 0;

    virtual void    WriteInt8(int8_t val)   = 0;
    virtual void    WriteInt16(int16_t val) = 0;
    virtual void    WriteInt32(int32_t val) = 0;
    virtual void    WriteInt64(int64_t val) = 0;
    virtual int     Write(const void *buffer, int size) = 0;
    virtual void    WriteString(const CString &str) = 0;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__STREAM_H
