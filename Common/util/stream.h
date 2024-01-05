//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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

// The mode in which the stream is opened,
// defines acceptable operations and capabilities.
// TODO: merge with FileWorkMode (historical mistake)
enum StreamMode
{
    // kStream_None defines an invalid (non-functional) stream
    kStream_None    = 0,
    // Following capabilities should be passed as request
    // when opening a stream subclass
    kStream_Read    = 0x01,
    kStream_Write   = 0x02,
    kStream_ReadWrite = kStream_Read | kStream_Write,
    
    // Following capabilities should not be requested,
    // but are defined by each particular stream subclass
    kStream_Seek    = 0x04
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
    // Tells which mode the stream is working in, which defines
    // supported io operations, such as reading, writing, seeking, etc.
    // Invalid streams return kStream_None to indicate that they are not functional.
    virtual StreamMode GetMode() const = 0;
    // Returns an optional path of a stream's source, such as a filepath;
    // this is purely for diagnostic purposes
    virtual const char *GetPath() const = 0;
    // Tells whether this stream's position is at its end;
    // note that unlike standard C feof this does not wait for a read attempt
    // past the stream end, and reports positive when position = length.
    virtual bool   EOS() const = 0;
    // Tells if there were errors during previous io operation(s);
    // the call to GetError() *resets* the error record.
    virtual bool   GetError() const = 0;
    // Returns the total stream's length in bytes
    virtual soff_t GetLength() const = 0;
    // Returns stream's position
    virtual soff_t GetPosition() const = 0;

    // Reads number of bytes into the provided buffer
    virtual size_t Read(void *buffer, size_t len) = 0;
    // ReadByte conforms to standard C fgetc behavior:
    // - on success returns an *unsigned char* packed in the int32
    // - on failure (EOS or other error), returns -1
    virtual int32_t ReadByte() = 0;
    // Writes number of bytes from the provided buffer
    virtual size_t Write(const void *buffer, size_t len) = 0;
    // WriteByte conforms to standard C fputc behavior:
    // - on success, returns the written unsigned char packed in the int32
    // - on failure, returns -1
    virtual int32_t WriteByte(uint8_t b) = 0;
    // Seeks to offset from the origin;
    // returns the new position in stream, or -1 on error.
    virtual soff_t Seek(soff_t offset, StreamSeek origin = kSeekCurrent) = 0;
    // Flush stream buffer to the underlying device
    virtual bool   Flush() = 0;
    // Closes the stream
    virtual void   Close() = 0;

    // Closes the stream and deallocates the object memory.
    // NOTE: this effectively deletes the stream object, making it unusable.
    // This method is purposed for the plugin API, it should be used instead
    // of regular *delete* on stream objects that may have been provided
    // by plugins.
    virtual void   Dispose() = 0;

protected:
    IStream() = default;
    virtual ~IStream() = default;
};


class Stream : public IStream
{
public:
    Stream() = default;
    Stream(const String &path)
        : _path(path) {}
    virtual ~Stream() = default;

    //-----------------------------------------------------
    // Helpers for learning the stream's state and capabilities
    //-----------------------------------------------------
    // Tells if stream is functional, and which operations are supported
    bool        IsValid() const  { return GetMode() != kStream_None; }
    bool        CanRead() const  { return (GetMode() & kStream_Read) != 0; }
    bool        CanWrite() const { return (GetMode() & kStream_Write) != 0; }
    bool        CanSeek() const  { return (GetMode() & kStream_Seek) != 0; }

    //-----------------------------------------------------
    // IStream interface
    //-----------------------------------------------------
    // Returns an optional path of a stream's source, such as a filepath;
    // primarily for diagnostic purposes
    const char         *GetPath() const override { return _path.GetCStr(); }
    // Tells which mode the stream is working in, which defines
    // supported io operations, such as reading, writing, seeking, etc.
    // Invalid streams return kStream_None to indicate that they are not functional.
    StreamMode          GetMode() const override { return kStream_None; }
    // Tells if there were errors during previous io operation(s);
    // the call to GetError() *resets* the error record.
    bool                GetError() const override { return false; }

    //-----------------------------------------------------
    // Values reading and writing interface
    //-----------------------------------------------------
    // Convenience methods for reading values of particular size
    virtual int8_t      ReadInt8() = 0;
    virtual int16_t     ReadInt16() = 0;
    virtual int32_t     ReadInt32() = 0;
    virtual int64_t     ReadInt64() = 0;
    virtual size_t      ReadArray(void *buffer, size_t elem_size, size_t count) = 0;
    virtual size_t      ReadArrayOfInt16(int16_t *buffer, size_t count) = 0;
    virtual size_t      ReadArrayOfInt32(int32_t *buffer, size_t count) = 0;
    virtual size_t      ReadArrayOfInt64(int64_t *buffer, size_t count) = 0;

    // Convenience methods for writing values of particular size
    virtual size_t      WriteInt8(int8_t val) = 0;;
    virtual size_t      WriteInt16(int16_t val) = 0;
    virtual size_t      WriteInt32(int32_t val) = 0;
    virtual size_t      WriteInt64(int64_t val) = 0;
    virtual size_t      WriteArray(const void *buffer, size_t elem_size, size_t count) = 0;
    virtual size_t      WriteArrayOfInt16(const int16_t *buffer, size_t count) = 0;
    virtual size_t      WriteArrayOfInt32(const int32_t *buffer, size_t count) = 0;
    virtual size_t      WriteArrayOfInt64(const int64_t *buffer, size_t count) = 0;

    //-----------------------------------------------------
    // Helper methods for read and write
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

private:
    // Standard streams do not expose this method to avoid incorrect use.
    void Dispose() override { delete this; }
};

// Copies N bytes from one stream into another;
// returns number of bytes actually written
soff_t CopyStream(Stream *in, Stream *out, soff_t length);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__STREAM_H
