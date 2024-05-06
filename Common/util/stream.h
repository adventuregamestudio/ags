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

#include <memory>
#include "util/bbop.h"
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


// IStreamBase is a contract for the basic stream class.
// IMPORTANT: currently this is a match for IAGSStream in plugin API.
class IStreamBase
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

    // Virtual destructor must be located after the IAGSStream interface
    virtual ~IStreamBase() = default;

protected:
    IStreamBase() = default;
};


// Base implementation for the IStreamBase interface.
// This class provides only those methods that are optional in stream API.
// Actual stream "devices" should inherit StreamBase and provide full implementation.
class StreamBase : public IStreamBase
{
public:
    StreamBase() = default;
    StreamBase(const String &path)
        : _path(path) {}
    virtual ~StreamBase() = default;

    //-----------------------------------------------------
    // Helpers for learning the stream's state and capabilities
    //-----------------------------------------------------
    // Tells if stream is functional, and which operations are supported
    bool        IsValid() const  { return GetMode() != kStream_None; }
    bool        CanRead() const  { return (GetMode() & kStream_Read) != 0; }
    bool        CanWrite() const { return (GetMode() & kStream_Write) != 0; }
    bool        CanSeek() const  { return (GetMode() & kStream_Seek) != 0; }

    // Boolean operator, equivalent to IsValid(), for convenience
    operator bool() const { return IsValid(); }

    //-----------------------------------------------------
    // Default implementation of IStreamBase
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

protected:
    String _path; // optional name of the stream's source (e.g. filepath)

private:
    // Standard streams do not expose this method to avoid incorrect use.
    void Dispose() override { delete this; }
};


// Class Stream is a wrapper over IStreamBase api.
// Besides common operations, provides a multitude of helper methods
// for reading and writing basic data types independently from system's endianess.
// This class assumes little-endian data by default.
// TODO: add big-endian read-write methods for the sake of completeness.
class Stream final
{
public:
    Stream() = default;
    Stream(std::unique_ptr<IStreamBase> &&base)
        : _base(std::move(base)) {}
    Stream(Stream &&other)
        : _base(std::move(other._base)) {}
    ~Stream() = default;

    IStreamBase *GetStreamBase() { return _base.get(); }
    std::unique_ptr<IStreamBase> ReleaseStreamBase() { return std::move(_base); }

    //-----------------------------------------------------
    // Helpers for learning the stream's state and capabilities
    //-----------------------------------------------------
    // Tells if stream is functional, and which operations are supported
    bool        IsValid() const  { return GetMode() != kStream_None; }
    bool        CanRead() const  { return (GetMode() & kStream_Read) != 0; }
    bool        CanWrite() const { return (GetMode() & kStream_Write) != 0; }
    bool        CanSeek() const  { return (GetMode() & kStream_Seek) != 0; }

    // Boolean operator, equivalent to IsValid(), for convenience
    operator bool() const { return IsValid(); }

    Stream &operator =(Stream &&other)
    {
        _base = std::move(other._base);
        return *this;
    }

    //-----------------------------------------------------
    // IStreamBase helper accessors
    //-----------------------------------------------------
    // Returns an optional path of a stream's source, such as a filepath;
    // primarily for diagnostic purposes
    const char  *GetPath() const { return _base->GetPath(); }
    // Tells which mode the stream is working in, which defines
    // supported io operations, such as reading, writing, seeking, etc.
    // Invalid streams return kStream_None to indicate that they are not functional.
    StreamMode  GetMode() const { return _base->GetMode(); }
    // Tells whether this stream's position is at its end;
    // note that unlike standard C feof this does not wait for a read attempt
    // past the stream end, and reports positive when position = length.
    bool        EOS() const { return _base->EOS(); }
    // Tells if there were errors during previous io operation(s);
    // the call to GetError() *resets* the error record.
    bool        GetError() const { return _base->GetError(); }
    // Returns the total stream's length in bytes
    soff_t      GetLength() const { return _base->GetLength(); }
    // Returns stream's position
    soff_t      GetPosition() const { return _base->GetPosition(); }

    // Reads number of bytes into the provided buffer
    size_t      Read(void *buffer, size_t len)
    {
        return _base->Read(buffer, len);
    }
    // ReadByte conforms to standard C fgetc behavior:
    // - on success returns an *unsigned char* packed in the int32
    // - on failure (EOS or other error), returns -1
    int32_t     ReadByte() { return _base->ReadByte(); }
    // Writes number of bytes from the provided buffer
    size_t      Write(const void *buffer, size_t len)
    {
        return _base->Write(buffer, len);
    }
    // WriteByte conforms to standard C fputc behavior:
    // - on success, returns the written unsigned char packed in the int32
    // - on failure, returns -1
    int32_t     WriteByte(uint8_t b) { return _base->WriteByte(b); };
    // Seeks to offset from the origin;
    // returns the new position in stream, or -1 on error.
    soff_t      Seek(soff_t offset, StreamSeek origin = kSeekCurrent)
    {
        return _base->Seek(offset, origin);
    }
    // Flush stream buffer to the underlying device
    bool        Flush() { return _base->Flush(); }
    // Closes the stream
    void        Close() { return _base->Close(); }

    //-------------------------------------------------------------------------
    // Following are helper methods for reading & writing particular values.
    //-------------------------------------------------------------------------
    int8_t ReadInt8()
    {
        int8_t val = 0;
        Read(&val, sizeof(int8_t));
        return val;
    }
    int16_t ReadInt16()
    {
        int16_t val = 0;
        Read(&val, sizeof(int16_t));
        return BBOp::Int16FromLE(val);
    }
    int32_t ReadInt32()
    {
        int32_t val = 0;
        Read(&val, sizeof(int32_t));
        return BBOp::Int32FromLE(val);
    }
    int64_t ReadInt64()
    {
        int64_t val = 0;
        Read(&val, sizeof(int64_t));
        return BBOp::Int64FromLE(val);
    }
    float ReadFloat32()
    {
        float val = 0.f;
        Read(&val, sizeof(float));
        return BBOp::Float32FromLE(val);
    }
    bool ReadBool()
    {
        return ReadInt8() != 0;
    }
 
    size_t WriteInt8(int8_t val)
    {
        return Write(&val, sizeof(int8_t));
    }
    size_t WriteInt16(int16_t val)
    {
        val = BBOp::Int16FromLE(val);
        return Write(&val, sizeof(int16_t));
    }
    size_t WriteInt32(int32_t val)
    {
        val = BBOp::Int32FromLE(val);
        return Write(&val, sizeof(int32_t));
    }
    size_t WriteInt64(int64_t val)
    {
        val = BBOp::Int64FromLE(val);
        return Write(&val, sizeof(int64_t));
    }
    size_t WriteFloat32(float val)
    {
        val = BBOp::Float32FromLE(val);
        return Write(&val, sizeof(float));
    }
    size_t WriteBool(bool val)
    {
        return WriteInt8(val ? 1 : 0);
    }

    //-------------------------------------------------------------------------
    // Following are helper methods for reading & writing arrays of basic types.
    // NOTE: ReadArray and WriteArray methods return number of full elements
    // (NOT bytes) read or written.
    //-------------------------------------------------------------------------
    size_t ReadArray(void *buffer, size_t elem_size, size_t count)
    {
        return Read(buffer, elem_size * count) / elem_size;
    }
    size_t ReadArrayOfInt8(int8_t *buffer, size_t count)
    {
        return Read(buffer, count);
    }
    size_t ReadArrayOfInt16(int16_t *buffer, size_t count)
    {
    #if defined (BITBYTE_BIG_ENDIAN)
        return ReadAndConvertArrayOfInt16(buffer, count);
    #else
        return ReadArray(buffer, sizeof(int16_t), count);
    #endif
    }
    size_t ReadArrayOfInt32(int32_t *buffer, size_t count)
    {
    #if defined (BITBYTE_BIG_ENDIAN)
        return ReadAndConvertArrayOfInt32(buffer, count);
    #else
        return ReadArray(buffer, sizeof(int32_t), count);
    #endif
    }
    size_t ReadArrayOfInt64(int64_t *buffer, size_t count)
    {
    #if defined (BITBYTE_BIG_ENDIAN)
        return ReadAndConvertArrayOfInt64(buffer, count);
    #else
        return ReadArray(buffer, sizeof(int64_t), count);
    #endif
    }
    size_t ReadArrayOfFloat32(float *buffer, size_t count)
    {
    #if defined (BITBYTE_BIG_ENDIAN)
        return ReadAndConvertArrayOfFloat32(buffer, count);
    #else
        return ReadArray(buffer, sizeof(float), count);
    #endif
    }
    
    size_t WriteArray(const void *buffer, size_t elem_size, size_t count)
    {
        return Write(buffer, elem_size * count) / elem_size;
    }
    size_t WriteArrayOfInt8(const int8_t *buffer, size_t count)
    {
        return Write(buffer, count);
    }
    size_t WriteArrayOfInt16(const int16_t *buffer, size_t count)
    {
    #if defined (BITBYTE_BIG_ENDIAN)
        return WriteAndConvertArrayOfInt16(buffer, count);
    #else
        return WriteArray(buffer, sizeof(int16_t), count);
    #endif
    }
    size_t WriteArrayOfInt32(const int32_t *buffer, size_t count)
    {
    #if defined (BITBYTE_BIG_ENDIAN)
        return WriteAndConvertArrayOfInt32(buffer, count);
    #else
        return WriteArray(buffer, sizeof(int32_t), count);
    #endif
    }
    size_t WriteArrayOfInt64(const int64_t *buffer, size_t count)
    {
    #if defined (BITBYTE_BIG_ENDIAN)
        return WriteAndConvertArrayOfInt64(buffer, count);
    #else
        return WriteArray(buffer, sizeof(int64_t), count);
    #endif
    }
    size_t WriteArrayOfFloat32(const float *buffer, size_t count)
    {
    #if defined (BITBYTE_BIG_ENDIAN)
        return WriteAndConvertArrayOfFloat32(buffer, count);
    #else
        return WriteArray(buffer, sizeof(float), count);
    #endif
    }

    // Fill the requested number of bytes with particular value
    size_t WriteByteCount(uint8_t b, size_t count);

private:
    std::unique_ptr<IStreamBase> _base;

    // Helper methods for reading/writing arrays of basic types and
    // converting their elements to opposite endianess (swapping bytes).
    size_t  ReadAndConvertArrayOfInt16(int16_t *buffer, size_t count);
    size_t  ReadAndConvertArrayOfInt32(int32_t *buffer, size_t count);
    size_t  ReadAndConvertArrayOfInt64(int64_t *buffer, size_t count);
    size_t  ReadAndConvertArrayOfFloat32(float *buffer, size_t count);
    size_t  WriteAndConvertArrayOfInt16(const int16_t *buffer, size_t count);
    size_t  WriteAndConvertArrayOfInt32(const int32_t *buffer, size_t count);
    size_t  WriteAndConvertArrayOfInt64(const int64_t *buffer, size_t count);
    size_t  WriteAndConvertArrayOfFloat32(const float *buffer, size_t count);
};


// StreamSection wraps another streambase and restricts its access to
// a particular range of offsets of the base stream.
// StreamSection may own or not own the base stream, depending on
// constructor you used to create one.
// For a non-owning StreamSection, the base stream must stay in memory
// for as long as there's at least one "section" refering it.
//
// TODO: consider making this a parent of BufferedStream;
// but be aware of impl differences in Read/Write/Seek.
// right now StreamSection looks like a lighter version of BufferedStream.
class StreamSection : public StreamBase
{
public:
    // Constructs a owning StreamSection over a base stream,
    // restricting working range to [start, end), i.e. end offset is
    // +1 past allowed position.
    StreamSection(std::unique_ptr<IStreamBase> &&base, soff_t start, soff_t end);
    // Constructs a non-owning StreamSection over a base stream.
    StreamSection(IStreamBase *base, soff_t start, soff_t end);

    StreamMode  GetMode() const override { return _base->GetMode(); }
    const char *GetPath() const override { return _base->GetPath(); }
    bool        EOS() const override     { return _position >= _end; }
    bool        GetError() const override { return _base->GetError(); }

    soff_t GetLength() const override { return _end - _start; }
    soff_t GetPosition() const override { return _position - _start; }

    size_t Read(void *buffer, size_t len) override;
    int32_t ReadByte() override;
    size_t Write(const void *buffer, size_t len) override;
    int32_t WriteByte(uint8_t b) override;
    soff_t Seek(soff_t offset, StreamSeek origin = kSeekCurrent) override;
    bool   Flush() override { return _base->Flush(); }
    void   Close() override;

private:
    void Open(IStreamBase *base);
    void OpenSection(IStreamBase *base, soff_t start_pos, soff_t end_pos);

    // Stores IStreamBase object in case of a given ownership
    std::unique_ptr<IStreamBase> _ownBase;
    // TODO: use smart pointer without deletion? e.g. unique_ptr with no-op deleter
    IStreamBase *_base = nullptr;
    soff_t _start = 0;
    soff_t _end = 0;
    soff_t _position = 0;
};


// Copies N bytes from one stream into another;
// returns number of bytes actually written
soff_t CopyStream(Stream *in, Stream *out, soff_t length);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__STREAM_H
