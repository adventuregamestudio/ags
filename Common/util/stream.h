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
//
// Base stream class.
//
// Provides a stream interface and a few helper methods.
//
// The user is advised to use advanced helper methods, such as Read/WriteX
// and Read/WriteArrayOfX to allow the stream implementation properly control
// endianness conversions when needed.
// 
// Only streams with uncommon behavior should be derived directly from Stream.
// Most I/O devices should inherit DataStream instead.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STREAM_H
#define __AGS_CN_UTIL__STREAM_H

#include "util/string.h"

namespace AGS
{
namespace Common
{

// TODO: merge with FileWorkMode (historical mistake)
enum StreamWorkMode
{
    kStream_Read    = 0x1,
    kStream_Write   = 0x2
};

enum StreamSeek
{
    kSeekBegin,
    kSeekCurrent,
    kSeekEnd
};


// IStream is a contract for the basic stream class.
class IStream
{
public:
    // Reads number of bytes into the provided buffer
    virtual size_t Read(void *buffer, size_t len) = 0;
    // Writes number of bytes from the provided buffer
    virtual size_t Write(const void *buffer, size_t len) = 0;
    // Returns the total stream's length in bytes
    virtual soff_t GetLength() const = 0;
    // Returns stream's position
    virtual soff_t GetPosition() const = 0;
    // Seeks to offset from the origin
    virtual bool   Seek(soff_t offset, StreamSeek origin) = 0;

protected:
    IStream() = default;
    ~IStream() = default;
};


class Stream : public IStream
{
public:
    Stream() = default;
    Stream(const String &path)
        : _path(path) {}
    virtual ~Stream() = default;

    // Returns an optional path of a stream's source, such as a filepath;
    // primarily for diagnostic purposes
    const String &GetPath() const { return _path; }

    //-----------------------------------------------------
    // Stream interface
    //-----------------------------------------------------

    virtual bool        IsValid() const = 0;
    // Tells if the stream has errors
    virtual bool        HasErrors() const { return false; }
    virtual bool        EOS() const = 0;
    virtual soff_t      GetLength() const = 0;
    virtual soff_t      GetPosition() const = 0;
    virtual bool        CanRead() const = 0;
    virtual bool        CanWrite() const = 0;
    virtual bool        CanSeek() const = 0;

    virtual void        Close() = 0;

    // Reads number of bytes in the provided buffer
    virtual size_t      Read(void *buffer, size_t size) = 0;
    // ReadByte conforms to fgetc behavior:
    // - if stream is valid, then returns an *unsigned char* packed in the int
    // - if EOS, then returns -1
    virtual int32_t     ReadByte() = 0;
    // Writes number of bytes from the provided buffer
    virtual size_t      Write(const void *buffer, size_t size) = 0;
    // WriteByte conforms to fputc behavior:
    // - on success, returns the unsigned char packed in the int
    // - on failure, returns -1
    virtual int32_t     WriteByte(uint8_t b) = 0;

    // Convenience methods for reading values of particular size
    virtual int8_t      ReadInt8() = 0;
    virtual int16_t     ReadInt16() = 0;
    virtual int32_t     ReadInt32() = 0;
    virtual int64_t     ReadInt64() = 0;
    virtual float       ReadFloat32() = 0;
    virtual size_t      ReadArray(void *buffer, size_t elem_size, size_t count) = 0;
    virtual size_t      ReadArrayOfInt16(int16_t *buffer, size_t count) = 0;
    virtual size_t      ReadArrayOfInt32(int32_t *buffer, size_t count) = 0;
    virtual size_t      ReadArrayOfInt64(int64_t *buffer, size_t count) = 0;
    virtual size_t      ReadArrayOfFloat32(float *buffer, size_t count) = 0;

    // Convenience methods for writing values of particular size
    virtual size_t      WriteInt8(int8_t val) = 0;
    virtual size_t      WriteInt16(int16_t val) = 0;
    virtual size_t      WriteInt32(int32_t val) = 0;
    virtual size_t      WriteInt64(int64_t val) = 0;
    virtual size_t      WriteFloat32(float val) = 0;
    virtual size_t      WriteArray(const void *buffer, size_t elem_size, size_t count) = 0;
    virtual size_t      WriteArrayOfInt16(const int16_t *buffer, size_t count) = 0;
    virtual size_t      WriteArrayOfInt32(const int32_t *buffer, size_t count) = 0;
    virtual size_t      WriteArrayOfInt64(const int64_t *buffer, size_t count) = 0;
    virtual size_t      WriteArrayOfFloat32(const float *buffer, size_t count) = 0;

    virtual bool        Seek(soff_t offset, StreamSeek origin = kSeekCurrent) = 0;

    // Flush stream buffer to the underlying device
    virtual bool        Flush() = 0;

    //-----------------------------------------------------
    // Helper methods
    //-----------------------------------------------------
    bool ReadBool()
    {
        return ReadInt8() != 0;
    }

    size_t WriteBool(bool val)
    {
        return WriteInt8(val ? 1 : 0);
    }

    // Practically identical to Read() and Write(), these two helpers' only
    // meaning is to underline the purpose of data being (de)serialized
    size_t ReadArrayOfInt8(int8_t *buffer, size_t count)
    {
        return Read(buffer, count);
    }
    size_t WriteArrayOfInt8(const int8_t *buffer, size_t count)
    {
        return Write(buffer, count);
    }

    // Fill the requested number of bytes with particular value
    size_t WriteByteCount(uint8_t b, size_t count);

protected:
    String _path; // optional name of the stream's source (e.g. filepath)
};

// Copies N bytes from one stream into another;
// returns number of bytes actually written
soff_t CopyStream(Stream *in, Stream *out, soff_t length);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__STREAM_H
