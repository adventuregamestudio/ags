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
#ifndef __AGS_CN_API__IAGSSTREAM2_H
#define __AGS_CN_API__IAGSSTREAM2_H

#include <stdexcept>
#include <memory>
#include <array>

#include <stdio.h>


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

// class IAGSStream2Error : public std::runtime_error {
//     IAGSStream2Error(String message) : std::runtime_error(message.GetCStr()) {}
// };

class ICoreStream
{
public:
    virtual ~ICoreStream() = default;

    virtual bool        EOS() const = 0;

    virtual size_t      Read(void *buffer, size_t size) = 0;

    virtual size_t      Write(const void *buffer, size_t size) = 0;
    virtual void        Flush() = 0;

    virtual soff_t      GetPosition() const = 0;
    virtual void        Seek(soff_t offset, StreamSeek origin = kSeekCurrent) = 0;
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

    virtual soff_t      GetPosition() const override;
    virtual void        Seek(soff_t offset, StreamSeek origin = kSeekCurrent) override;


private:
    const String &file_name_;
    const FileOpenMode  open_mode_;
    const FileWorkMode  work_mode_;

    FILE                *file_;
};


const auto BufferStreamSize = 64*1024;

class BufferedStream final : public ICoreStream
{
public:
    BufferedStream(std::unique_ptr<ICoreStream> stream);

    virtual bool        EOS() const override;

    virtual size_t      Read(void *buffer, size_t size) override;

    virtual size_t      Write(const void *buffer, size_t size) override;
    virtual void        Flush() override;

    virtual soff_t      GetPosition() const override;
    virtual void        Seek(soff_t offset, StreamSeek origin = kSeekCurrent) override;


private:

private:

    void UndoBuffer();

    std::unique_ptr<ICoreStream> stream_;

    std::array<char, BufferStreamSize> buffer_;
    size_t bytesAvail_ = 0;
    size_t bytesAhead_ = 0;

};


// Classic AGS stream

class Stream
{
public:
    virtual             ~Stream() = default;

    virtual bool        HasErrors() const = 0;
    virtual bool        EOS() const = 0;
    virtual soff_t      GetLength() const = 0;
    virtual soff_t      GetPosition() const = 0;
    virtual soff_t      Seek(soff_t offset, StreamSeek origin = kSeekCurrent) = 0;
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


class DataStream final : public Stream
{
public:
    DataStream(
        std::unique_ptr<ICoreStream> stream, 
        DataEndianess stream_endianess = kLittleEndian
    );

    bool        HasErrors() const override;
    bool        EOS() const override;
    soff_t      GetLength() const override;
    soff_t      GetPosition() const override;
    soff_t      Seek(soff_t offset, StreamSeek origin = kSeekCurrent) override;
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

    std::unique_ptr<ICoreStream> stream_;
    DataEndianess stream_endianess_;

    bool MustSwapBytes();
    void ConvertInt16(int16_t &val);
    void ConvertInt32(int32_t &val);
    void ConvertInt64(int64_t &val);
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

    AlignedStream(
        std::shared_ptr<Stream> stream, 
        AlignedStreamMode mode,
        size_t base_alignment = sizeof(int16_t)
    );
    ~AlignedStream() override;
    AlignedStream(const AlignedStream& other) = delete; // copy constructor
    AlignedStream& operator=(const AlignedStream& right) = delete; // copy assignment

    // Read/Write cumulated padding and reset block counter
    void        Reset();

    bool        HasErrors() const override;
    bool        EOS() const override;
    soff_t      GetLength() const override;
    soff_t      GetPosition() const override;
    soff_t      Seek(soff_t offset, StreamSeek origin = kSeekCurrent) override;
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

    std::shared_ptr<Stream> stream_;

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


#endif // __AGS_CN_API__IAGSSTREAM2_H
