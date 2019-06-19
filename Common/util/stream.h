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
/*
AGS Streams

Stream classes are split up into two different types. The CoreStream type which
has a minimal API exposed and the Steam type which is considered the "classic"
stream with many helper methods.

Most AGS code expects classic Stream classes. However, for implementing new
input sources, it is best to implement a new ICoreStream.

ICoreStream is designed to be composed. Instead of inheritence, a core stream
should accept another stream for it to wrap. unique_ptrs ensure only one owner.
*/
//=============================================================================

#ifndef __AGS_CN_UTIL__STREAM_H
#define __AGS_CN_UTIL__STREAM_H

#include <stdexcept>
#include <string>
#include <memory>
#include <vector>

#include <stdio.h>
#include "util/stdio_compat.h"
#include "core/types.h"
#include "util/string.h"
#include "util/file.h"
#include "util/bbop.h"

namespace AGS {
namespace Common {

enum StreamSeek
{
   kSeekBegin,
   kSeekCurrent,
   kSeekEnd
};

class StreamError : public std::runtime_error
{
public:
    explicit StreamError(const AGS::Common::String &what);
};

class ICoreStream
{
public:
    virtual ~ICoreStream() = default;

    virtual bool        EOS() const = 0;

    virtual size_t      Read(void *buffer, size_t size) = 0;

    virtual size_t      Write(const void *buffer, size_t size) = 0;
    virtual void        Flush() = 0;

    virtual file_off_t      GetPosition() const = 0;
    virtual void        Seek(file_off_t offset, StreamSeek origin = kSeekCurrent) = 0;
};


class FileStream final : public ICoreStream
{
public:
    FileStream(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode);
    ~FileStream() override;
    FileStream(const FileStream& other) = delete; // copy constructor
    FileStream& operator=(const FileStream& right) = delete; // copy assignment

    virtual bool        EOS() const override;

    virtual size_t      Read(void *buffer, size_t size) override;

    virtual size_t      Write(const void *buffer, size_t size) override;
    virtual void        Flush() override;

    virtual file_off_t      GetPosition() const override;
    virtual void        Seek(file_off_t offset, StreamSeek origin = kSeekCurrent) override;


private:
    const String _file_name;
    const FileOpenMode  _open_mode;
    const FileWorkMode  _work_mode;
    const String _mode;

    FILE                *_file;
};


const auto BufferStreamSize = 2*512*1024;

class BufferedStream final : public ICoreStream
{
public:
    BufferedStream(std::unique_ptr<ICoreStream> stream);

    bool        EOS() const override;

    size_t      Read(void *buffer, size_t size) override;

    size_t      Write(const void *buffer, size_t size) override;
    void        Flush() override;

    file_off_t      GetPosition() const override;
    void        Seek(file_off_t offset, StreamSeek origin = kSeekCurrent) override;


private:

    std::unique_ptr<ICoreStream> _stream;

    file_off_t _bufferPosition;
    std::vector<char> _buffer;
    file_off_t _position;
    file_off_t _end;

    void FillBufferFromPosition(file_off_t position);
};



class MemoryStream final : public ICoreStream
{
public:
    MemoryStream(std::vector<char> buffer);

    MemoryStream(const FileStream& other) = delete; // copy constructor
    MemoryStream& operator=(const FileStream& right) = delete; // copy assignment

    virtual bool        EOS() const override;

    virtual size_t      Read(void *buffer, size_t size) override;

    virtual size_t      Write(const void *buffer, size_t size) override;
    virtual void        Flush() override;

    virtual file_off_t      GetPosition() const override;
    virtual void        Seek(file_off_t offset, StreamSeek origin = kSeekCurrent) override;


private:
    std::vector<char> _buffer;
    file_off_t _position;
};



// Classic AGS stream

class Stream
{
public:
    virtual             ~Stream() = default;

    virtual bool        HasErrors() const = 0;
    virtual bool        EOS() const = 0;
    virtual file_off_t      GetLength() const = 0;
    virtual file_off_t      GetPosition() const = 0;
    virtual file_off_t      Seek(file_off_t offset, StreamSeek origin = kSeekCurrent) = 0;
    virtual bool        Flush() = 0;

    virtual size_t      Read(void *buffer, size_t size) = 0;
    virtual int32_t     ReadByte() = 0;

    virtual int8_t      ReadInt8() = 0;
    virtual int16_t     ReadInt16() = 0;
    virtual int32_t     ReadInt32() = 0;
    virtual int64_t     ReadInt64() = 0;
    virtual bool        ReadBool() = 0;
    virtual size_t      ReadArray(void *buffer, size_t elem_size, size_t count) = 0;
    virtual size_t      ReadArrayOfInt8(int8_t *buffer, size_t count) = 0;
    virtual size_t      ReadArrayOfInt16(int16_t *buffer, size_t count) = 0;
    virtual size_t      ReadArrayOfInt32(int32_t *buffer, size_t count) = 0;
    virtual size_t      ReadArrayOfInt64(int64_t *buffer, size_t count) = 0;


    virtual size_t      Write(const void *buffer, size_t size) = 0;
    virtual int32_t     WriteByte(uint8_t b) = 0;

    virtual size_t      WriteInt8(int8_t val) = 0;
    virtual size_t      WriteInt16(int16_t val) = 0;
    virtual size_t      WriteInt32(int32_t val) = 0;
    virtual size_t      WriteInt64(int64_t val) = 0;
    virtual size_t      WriteBool(bool val) = 0;

    virtual size_t      WriteByteCount(uint8_t b, size_t count) = 0;

    virtual size_t      WriteArray(const void *buffer, size_t elem_size, size_t count) = 0;
    virtual size_t      WriteArrayOfInt8(const int8_t *buffer, size_t count) = 0;
    virtual size_t      WriteArrayOfInt16(const int16_t *buffer, size_t count) = 0;
    virtual size_t      WriteArrayOfInt32(const int32_t *buffer, size_t count) = 0;
    virtual size_t      WriteArrayOfInt64(const int64_t *buffer, size_t count) = 0;
};


// Standard AGS stream implementation for reading raw data with support for
// converting to opposite endianess. Most I/O devices should inherit this
// class and provide implementation for basic reading and writing only.

class DataStream final : public Stream
{
public:
    DataStream(
        std::unique_ptr<ICoreStream> stream, 
        DataEndianess stream_endianess = kLittleEndian
    );

    bool        HasErrors() const override;
    bool        EOS() const override;
    file_off_t      GetLength() const override;
    file_off_t      GetPosition() const override;
    file_off_t      Seek(file_off_t offset, StreamSeek origin = kSeekCurrent) override;
    bool        Flush() override;

    size_t      Read(void *buffer, size_t size) override;
    int32_t     ReadByte() override;

    int8_t      ReadInt8() override;
    int16_t     ReadInt16() override;
    int32_t     ReadInt32() override;
    int64_t     ReadInt64() override;
    bool        ReadBool() override;
    size_t      ReadArray(void *buffer, size_t elem_size, size_t count) override;
    size_t      ReadArrayOfInt8(int8_t *buffer, size_t count) override;
    size_t      ReadArrayOfInt16(int16_t *buffer, size_t count) override;
    size_t      ReadArrayOfInt32(int32_t *buffer, size_t count) override;
    size_t      ReadArrayOfInt64(int64_t *buffer, size_t count) override;


    size_t      Write(const void *buffer, size_t size) override;
    int32_t     WriteByte(uint8_t b) override;

    size_t      WriteInt8(int8_t val) override;
    size_t      WriteInt16(int16_t val) override;
    size_t      WriteInt32(int32_t val) override;
    size_t      WriteInt64(int64_t val) override;
    size_t      WriteBool(bool val) override;

    size_t      WriteByteCount(uint8_t b, size_t count) override;

    size_t      WriteArray(const void *buffer, size_t elem_size, size_t count) override;
    size_t      WriteArrayOfInt8(const int8_t *buffer, size_t count) override;
    size_t      WriteArrayOfInt16(const int16_t *buffer, size_t count) override;
    size_t      WriteArrayOfInt32(const int32_t *buffer, size_t count) override;
    size_t      WriteArrayOfInt64(const int64_t *buffer, size_t count) override;


private:

    std::unique_ptr<ICoreStream> _stream;
    DataEndianess _stream_endianess;

    bool MustSwapBytes();
    void ConvertInt16(int16_t &val);
    void ConvertInt32(int32_t &val);
    void ConvertInt64(int64_t &val);
};


// Class AlignedStream
//
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

// TODO: replace with std::shared_ptr!!!
enum ObjectOwnershipPolicy
{
    kReleaseAfterUse,
    kDisposeAfterUse
};

enum AlignedStreamMode
{
    kAligned_Read,
    kAligned_Write
};


// this is more of an adapter like text reader
class AlignedStream final : public Stream
{
public:

    AlignedStream(  Stream *stream, 
                    AlignedStreamMode mode,
                    ObjectOwnershipPolicy stream_ownership_policy = kReleaseAfterUse,
                    size_t base_alignment = sizeof(int16_t)
                    );
    ~AlignedStream() override;

    AlignedStream(const AlignedStream& other) = delete; // copy constructor
    AlignedStream& operator=(const AlignedStream& right) = delete; // copy assignment

    // Read/Write cumulated padding and reset block counter
    void        Reset();

    bool        HasErrors() const override;
    bool        EOS() const override;
    file_off_t      GetLength() const override;
    file_off_t      GetPosition() const override;
    file_off_t      Seek(file_off_t offset, StreamSeek origin = kSeekCurrent) override;
    bool        Flush() override;

    size_t      Read(void *buffer, size_t size) override;
    int32_t     ReadByte() override;

    int8_t      ReadInt8() override;
    int16_t     ReadInt16() override;
    int32_t     ReadInt32() override;
    int64_t     ReadInt64() override;
    bool        ReadBool() override;
    size_t      ReadArray(void *buffer, size_t elem_size, size_t count) override;
    size_t      ReadArrayOfInt8(int8_t *buffer, size_t count) override;
    size_t      ReadArrayOfInt16(int16_t *buffer, size_t count) override;
    size_t      ReadArrayOfInt32(int32_t *buffer, size_t count) override;
    size_t      ReadArrayOfInt64(int64_t *buffer, size_t count) override;


    size_t      Write(const void *buffer, size_t size) override;
    int32_t     WriteByte(uint8_t b) override;

    size_t      WriteInt8(int8_t val) override;
    size_t      WriteInt16(int16_t val) override;
    size_t      WriteInt32(int32_t val) override;
    size_t      WriteInt64(int64_t val) override;
    size_t      WriteBool(bool val) override;

    size_t      WriteByteCount(uint8_t b, size_t count) override;

    size_t      WriteArray(const void *buffer, size_t elem_size, size_t count) override;
    size_t      WriteArrayOfInt8(const int8_t *buffer, size_t count) override;
    size_t      WriteArrayOfInt16(const int16_t *buffer, size_t count) override;
    size_t      WriteArrayOfInt32(const int32_t *buffer, size_t count) override;
    size_t      WriteArrayOfInt64(const int64_t *buffer, size_t count) override;


private:

    static const size_t LargestPossibleType = sizeof(int64_t);

    Stream                  *_stream;
    ObjectOwnershipPolicy   _streamOwnershipPolicy;

    AlignedStreamMode   _mode;
    size_t              _baseAlignment;
    size_t              _maxAlignment;
    int64_t             _block;

    void            ReadPadding(size_t next_type);
    void            WritePadding(size_t next_type);
    void            FinalizeBlock();

};


} // namespace Common
} // namespace AGS


#endif // __AGS_CN_UTIL__STREAM_H
