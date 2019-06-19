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

#include "util/stream.h"

#include <cerrno>
#include <cstdio>
#include <algorithm>
#include "util/stdio_compat.h"

namespace AGS {
namespace Common {

StreamError::StreamError(const AGS::Common::String &what) : std::runtime_error(what.GetCStr()) {}

// --------------------------------------------------------------------------------------------------------------------
// FileStream
// --------------------------------------------------------------------------------------------------------------------

FileStream::FileStream(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode) 
    : _file_name(file_name), _open_mode(open_mode), _work_mode(work_mode), _mode(File::GetCMode(open_mode, work_mode))
{
    if (_mode.IsEmpty()) {
        throw StreamError("Error determining file open mode");
    }

    _file = std::fopen(file_name.GetCStr(), _mode.GetCStr());
    if (_file == nullptr) {
        throw StreamError("Error opening file");
    }
}

FileStream::~FileStream()
{
    if (_file != nullptr) {
        std::fclose(_file);
    }
    _file = nullptr;
}

void FileStream::Flush()
{
    auto result = std::fflush(_file);
    if (result != 0) {
        throw StreamError(std::strerror(errno));
    }
}

bool FileStream::EOS() const
{
    return std::feof(_file);
}


file_off_t FileStream::GetPosition() const
{
    auto result = ags_ftell(_file);
    if (result == -1L) {
        // we don't check for the success of GetPosition anywhere, so raise an exception in this case.
        throw StreamError(std::strerror(errno));
    }
    return result;
}


size_t FileStream::Read(void *buffer, size_t buffer_size)
{
    if (buffer_size <= 0) { return 0; }
    if (buffer == nullptr) { return 0; }

    return std::fread(buffer, 1, buffer_size, _file);
}

size_t FileStream::Write(const void *buffer, size_t buffer_size)
{
    if (buffer_size <= 0) { return 0; }
    if (buffer == nullptr) { return 0; }

    return std::fwrite(buffer, 1, buffer_size, _file);
}

void FileStream::Seek(file_off_t offset, StreamSeek origin)
{
    int fseek_origin;
    switch(origin) {
        case kSeekBegin:    fseek_origin = SEEK_SET; break;
        case kSeekCurrent:  fseek_origin = SEEK_CUR; break;
        case kSeekEnd:      fseek_origin = SEEK_END; break;
    }

    if (ags_fseek(_file, offset, fseek_origin) != 0) {
        // we don't check for the success of Seek anywhere, so raise an exception in this case.
        throw StreamError(std::strerror(errno));
    }
}



// --------------------------------------------------------------------------------------------------------------------
// BufferedStream
// --------------------------------------------------------------------------------------------------------------------

BufferedStream::BufferedStream(std::unique_ptr<ICoreStream> stream) :
    _stream(std::move(stream)), _bufferPosition(0), _buffer(0), _position(0)
{
    _stream->Seek(0, kSeekEnd);
    _end = _stream->GetPosition();

    _stream->Seek(0, kSeekBegin);
}

void BufferedStream::FillBufferFromPosition(file_off_t position)
{
    _stream->Seek(position, kSeekBegin);

    _buffer.resize(BufferStreamSize);
    auto sz = _stream->Read(_buffer.data(), BufferStreamSize);
    _buffer.resize(sz);

    _bufferPosition = position;
}

bool BufferedStream::EOS() const
{
    return _position == _end;
}

size_t BufferedStream::Read(void *toBuffer, size_t toSize) {
    auto to = static_cast<char *>(toBuffer);

    while (toSize > 0)
    {
        if (_position < _bufferPosition || _position >= _bufferPosition + _buffer.size())
        {
            FillBufferFromPosition(_position);
        }
        if (_buffer.size() <= 0) { break; } // reached EOS
        assert(_position >= _bufferPosition && _position < _bufferPosition + _buffer.size());  // sanity check only, should be checked by above.

        file_off_t bufferOffset = _position - _bufferPosition;
        assert(bufferOffset >= 0);
        size_t bytesLeft = _buffer.size() - (size_t)bufferOffset;
        size_t chunkSize = std::min<size_t>(bytesLeft, toSize);

        std::memcpy(to, _buffer.data() + bufferOffset, chunkSize);

        to += chunkSize;
        _position += chunkSize;
        toSize -= chunkSize;
    }

    return to - (char*)toBuffer;
}

size_t BufferedStream::Write(const void *buffer, size_t size)
{
    _stream->Seek(_position, kSeekBegin);
    auto sz = _stream->Write(buffer, size);
    if (_position == _end)
        _end += sz;
    _position += sz;
    return sz;
}

void BufferedStream::Flush() { _stream->Flush(); }

file_off_t BufferedStream::GetPosition() const
{
    return _position;
}

void BufferedStream::Seek(file_off_t offset, StreamSeek origin)
{
    file_off_t want_pos = -1;
    switch(origin)
    {
        case StreamSeek::kSeekCurrent:  want_pos = _position   + offset; break;
        case StreamSeek::kSeekBegin:    want_pos = 0           + offset; break;
        case StreamSeek::kSeekEnd:      want_pos = _end        + offset; break;
        break;
    }

    _position = std::min(std::max(want_pos, (file_off_t)0), _end);
}


// --------------------------------------------------------------------------------------------------------------------
// MemoryStream
// --------------------------------------------------------------------------------------------------------------------

MemoryStream::MemoryStream(std::vector<char> buffer) : _buffer(buffer), _position(0) {}

bool MemoryStream::EOS() const { return _position >= _buffer.size(); }

size_t MemoryStream::Read(void *buffer, size_t size)
{
    if (_position >= _buffer.size()) { return 0; }
    size_t remaining = _buffer.size() - (size_t)_position;
    if (remaining <= 0) { return 0; }
    assert(remaining > 0);
    // size_t should be okay because we check for negative `remaining` above.
    auto read_sz = std::min<size_t>(remaining, size);
    memcpy(buffer, _buffer.data() + _position, read_sz);
    _position += read_sz;
    return read_sz;
}

size_t MemoryStream::Write(const void *buffer, size_t size) { return 0; }
void MemoryStream::Flush() { }

file_off_t MemoryStream::GetPosition() const { return _position; }
void MemoryStream::Seek(file_off_t offset, StreamSeek origin)
{
    switch (origin) {
    case kSeekBegin:    _position = 0 + offset;  break;
    case kSeekCurrent:  _position = _position + offset;  break;
    case kSeekEnd:      _position = _buffer.size() + offset; break;
    }
    if (_position < 0) { _position = 0; }
    // end of stream is always one past the last byte.
    if (_position > _buffer.size()) { _position = _buffer.size(); }
}


// --------------------------------------------------------------------------------------------------------------------
// DataStream
// --------------------------------------------------------------------------------------------------------------------

DataStream::DataStream(
    std::unique_ptr<ICoreStream> stream, 
    DataEndianess stream_endianess
) : 
    _stream(std::move(stream)), 
    _stream_endianess(stream_endianess) 
{}

bool DataStream::EOS() const { return _stream->EOS(); }
file_off_t DataStream::GetPosition() const { return _stream->GetPosition(); }


bool DataStream::HasErrors() const { return false; };

file_off_t DataStream::GetLength() const {
    auto pos = _stream->GetPosition();

    _stream->Seek(0, AGS::Common::kSeekEnd);
    auto end = _stream->GetPosition();

    _stream->Seek(pos, AGS::Common::kSeekBegin);

    return end;
}

file_off_t DataStream::Seek(file_off_t offset, StreamSeek origin) {
    _stream->Seek(offset, origin);
    return _stream->GetPosition();
}

bool DataStream::Flush() { 
    try {
        _stream->Flush();
    } catch (StreamError) {
        return false;
    }
    return true;
}



// Reading

size_t DataStream::Read(void *buffer, size_t size) { return _stream->Read(buffer, size); }

// return unsigned value or -1
int32_t DataStream::ReadByte() {
    using T = uint8_t;
    T val;
    auto bytesRead = Read(&val, sizeof(T));
    if (bytesRead != sizeof(T)) { return EOF; }
    return val;
}

int8_t DataStream::ReadInt8() { 
    using T = int8_t;
    T val;
    auto bytesRead = Read(&val, sizeof(T));
    if (bytesRead != sizeof(T)) { return EOF; }
    return val;
}

int16_t DataStream::ReadInt16()
{
    using T = int16_t;
    T val;
    auto bytesRead = Read(&val, sizeof(T));
    if (bytesRead != sizeof(T)) { return EOF; }
    ConvertInt16(val);
    return val;
}

int32_t DataStream::ReadInt32()
{
    using T = int32_t;
    T val;
    auto bytesRead = Read(&val, sizeof(T));
    if (bytesRead != sizeof(T)) { return EOF; }
    ConvertInt32(val);
    return val;
}

int64_t DataStream::ReadInt64()
{
    using T = int64_t;
    T val;
    auto bytesRead = Read(&val, sizeof(T));
    if (bytesRead != sizeof(T)) { return EOF; }
    ConvertInt64(val);
    return val;
}

bool DataStream::ReadBool() { return ReadInt8() != 0; }

size_t DataStream::ReadArray(void *buffer, size_t elem_size, size_t count)
{
    return Read(buffer, elem_size * count) / elem_size;
}

size_t DataStream::ReadArrayOfInt8(int8_t *buffer, size_t count) 
{
    using T = int8_t;
    auto result = ReadArray(buffer, sizeof(T), count);
    return result;
}

size_t DataStream::ReadArrayOfInt16(int16_t *buffer, size_t count)
{
    using T = int16_t;
    auto result = ReadArray(buffer, sizeof(T), count);
    if (MustSwapBytes()) {
        for (size_t i = 0; i < count; i++) {
            buffer[i] = BBOp::SwapBytesInt16(buffer[i]);
        }
    }
    return result;
}

size_t DataStream::ReadArrayOfInt32(int32_t *buffer, size_t count)
{
    using T = int32_t;
    auto result = ReadArray(buffer, sizeof(T), count);
    if (MustSwapBytes()) {
        for (size_t i = 0; i < count; i++) {
            buffer[i] = BBOp::SwapBytesInt32(buffer[i]);
        }
    }
    return result;
}

size_t DataStream::ReadArrayOfInt64(int64_t *buffer, size_t count)
{
    using T = int64_t;
    auto result = ReadArray(buffer, sizeof(T), count);
    if (MustSwapBytes()) {
        for (size_t i = 0; i < count; i++) {
            buffer[i] = BBOp::SwapBytesInt64(buffer[i]);
        }
    }
    return result;
}


// Writing

size_t DataStream::Write(const void *buffer, size_t size) { return _stream->Write(buffer, size); }

// return unsigned value or -1
int32_t DataStream::WriteByte(uint8_t val)
{
    using T = uint8_t;
    T ch = val;
    auto bytesWritten = _stream->Write(&ch, sizeof(T));
    return bytesWritten == sizeof(T) ? ch : EOF;
}

size_t DataStream::WriteInt8(int8_t val) 
{
    using T = int8_t;
    T valb = val;
    auto bytesWritten = _stream->Write(&valb, sizeof(T));
    return bytesWritten == sizeof(T) ? 1 : 0;
}

size_t DataStream::WriteInt16(int16_t val)
{
    using T = int16_t;
    T valb = val;
    ConvertInt16(valb);
    auto bytesWritten = _stream->Write(&valb, sizeof(T));
    return bytesWritten == sizeof(T) ? 1 : 0;
}

size_t DataStream::WriteInt32(int32_t val)
{
    using T = int32_t;
    T valb = val;
    ConvertInt32(valb);
    auto bytesWritten = _stream->Write(&valb, sizeof(T));
    return bytesWritten == sizeof(T) ? 1 : 0;
}

size_t DataStream::WriteInt64(int64_t val)
{
    using T = int64_t;
    T valb = val;
    ConvertInt64(valb);
    auto bytesWritten = _stream->Write(&valb, sizeof(T));
    return bytesWritten == sizeof(T) ? 1 : 0;
}

size_t DataStream::WriteBool(bool val) { return WriteInt8(val ? 1 : 0); }

size_t DataStream::WriteByteCount(uint8_t b, size_t count)
{
    size_t writeCount = 0;
    for (size_t i = 0; i < count; i++) {
        auto result = WriteByte(b);
        if (result < 0) { break; }
        writeCount += 1;
    }
    return writeCount;
}

size_t DataStream::WriteArray(const void *buffer, size_t elem_size, size_t count)
{
    return Write(buffer, elem_size * count) / elem_size;
}

size_t DataStream::WriteArrayOfInt8(const int8_t *buffer, size_t count) 
{
    using T = int8_t;
    return WriteArray(buffer, sizeof(T), count);
}

size_t DataStream::WriteArrayOfInt16(const int16_t *buffer, size_t count)
{
    using T = int16_t;
    if (!MustSwapBytes()) {
        return WriteArray(buffer, sizeof(T), count);
    }

    size_t writeCount = 0;
    for (size_t i = 0; i < count; i++) {
        auto valsWritten = WriteInt16(buffer[i]);
        if (valsWritten <= 0) { break; }
        writeCount += 1;
    }
    return writeCount;
}

size_t DataStream::WriteArrayOfInt32(const int32_t *buffer, size_t count)
{
    using T = int32_t;
    if (!MustSwapBytes()) {
        return WriteArray(buffer, sizeof(T), count);
    }

    size_t writeCount = 0;
    for (size_t i = 0; i < count; i++) {
        auto valsWritten = WriteInt32(buffer[i]);
        if (valsWritten <= 0) { break; }
        writeCount += 1;
    }
    return writeCount;
}

size_t DataStream::WriteArrayOfInt64(const int64_t *buffer, size_t count)
{
    using T = int64_t;
    if (!MustSwapBytes()) {
        return WriteArray(buffer, sizeof(T), count);
    }

    size_t writeCount = 0;
    for (size_t i = 0; i < count; i++) {
        auto valsWritten = WriteInt64(buffer[i]);
        if (valsWritten <= 0) { break; }
        writeCount += 1;
    }
    return writeCount;
}



bool DataStream::MustSwapBytes()
{
    return kDefaultSystemEndianess != _stream_endianess;
}

void DataStream::ConvertInt16(int16_t &val)
{
    if (MustSwapBytes()) val = BBOp::SwapBytesInt16(val);
}
void DataStream::ConvertInt32(int32_t &val)
{
    if (MustSwapBytes()) val = BBOp::SwapBytesInt32(val);
}
void DataStream::ConvertInt64(int64_t &val)
{
    if (MustSwapBytes()) val = BBOp::SwapBytesInt64(val);
}


// --------------------------------------------------------------------------------------------------------------------
// AlignedStream
// --------------------------------------------------------------------------------------------------------------------


AlignedStream::AlignedStream(Stream *stream,
    AlignedStreamMode mode,
    ObjectOwnershipPolicy stream_ownership_policy,
    size_t base_alignment
) :
    _stream(stream),
    _streamOwnershipPolicy(stream_ownership_policy),
    _mode(mode), 
    _baseAlignment(base_alignment), 
    _maxAlignment(0), 
    _block(0) 
{}

AlignedStream::~AlignedStream() 
{ 
    AlignedStream::FinalizeBlock(); 

    if (_streamOwnershipPolicy == kDisposeAfterUse) {
        delete _stream;
        _stream = nullptr;
    }
}


bool AlignedStream::EOS() const { return _stream->EOS(); };
bool AlignedStream::Flush() { return _stream->Flush(); };
file_off_t AlignedStream::GetLength() const { return _stream->GetLength(); };
file_off_t AlignedStream::GetPosition() const { return _stream->GetPosition(); };
file_off_t AlignedStream::Seek(file_off_t offset, StreamSeek origin) { throw std::logic_error("Not implemented"); }
bool AlignedStream::HasErrors() const { return false; }
void AlignedStream::Reset() { FinalizeBlock(); }


// Read

size_t AlignedStream::Read(void *buffer, size_t size)
{
    ReadPadding(sizeof(int8_t));
    size = _stream->Read(buffer, size);
    _block += size;
    return size;
}

int32_t AlignedStream::ReadByte()
{
    uint8_t b = 0;
    ReadPadding(sizeof(uint8_t));
    b = _stream->ReadByte();
    _block += sizeof(uint8_t);
    return b;
}

bool AlignedStream::ReadBool() { 
    return ReadInt8() != 0;
};

int8_t AlignedStream::ReadInt8()
{
    int8_t val = 0;
    ReadPadding(sizeof(int8_t));
    val = _stream->ReadInt8();
    _block += sizeof(int8_t);
    return val;
}

int16_t AlignedStream::ReadInt16()
{
    int16_t val = 0;
    ReadPadding(sizeof(int16_t));
    val = _stream->ReadInt16();
    _block += sizeof(int16_t);
    return val;
}

int32_t AlignedStream::ReadInt32()
{
    int32_t val = 0;
    ReadPadding(sizeof(int32_t));
    val = _stream->ReadInt32();
    _block += sizeof(int32_t);
    return val;
}

int64_t AlignedStream::ReadInt64()
{
    int64_t val = 0;
    ReadPadding(sizeof(int64_t));
    val = _stream->ReadInt64();
    _block += sizeof(int64_t);
    return val;
}

size_t AlignedStream::ReadArray(void *buffer, size_t elem_size, size_t count)
{
    ReadPadding(elem_size);
    count = _stream->ReadArray(buffer, elem_size, count);
    _block += count * elem_size;
    return count;
}

size_t AlignedStream::ReadArrayOfInt8(int8_t *buffer, size_t count)
{
    ReadPadding(sizeof(int8_t));
    count = _stream->ReadArrayOfInt8(buffer, count);
    _block += count * sizeof(int8_t);
    return count;
}

size_t AlignedStream::ReadArrayOfInt16(int16_t *buffer, size_t count)
{
    ReadPadding(sizeof(int16_t));
    count = _stream->ReadArrayOfInt16(buffer, count);
    _block += count * sizeof(int16_t);
    return count;
}

size_t AlignedStream::ReadArrayOfInt32(int32_t *buffer, size_t count)
{
    ReadPadding(sizeof(int32_t));
    count = _stream->ReadArrayOfInt32(buffer, count);
    _block += count * sizeof(int32_t);
    return count;
}

size_t AlignedStream::ReadArrayOfInt64(int64_t *buffer, size_t count)
{
    ReadPadding(sizeof(int64_t));
    count = _stream->ReadArrayOfInt64(buffer, count);
    _block += count * sizeof(int64_t);
    return count;
}

// Write

size_t AlignedStream::Write(const void *buffer, size_t size)
{
    WritePadding(sizeof(int8_t));
    size = _stream->Write(buffer, size);
    _block += size;
    return size;
}

int32_t AlignedStream::WriteByte(uint8_t b)
{
    WritePadding(sizeof(uint8_t));
    b = _stream->WriteByte(b);
    _block += sizeof(uint8_t);
    return b;
}

size_t AlignedStream::WriteByteCount(uint8_t b, size_t count) { 
    WritePadding(sizeof(uint8_t));
    auto bytesWritten = _stream->WriteByteCount(b, count); 
    _block += bytesWritten;
    return bytesWritten;
};

size_t AlignedStream::WriteInt8(int8_t val)
{
    WritePadding(sizeof(int8_t));
    size_t size = _stream->WriteInt8(val);
    _block += sizeof(int8_t);
    return size;
}

size_t AlignedStream::WriteBool(bool val)
{
    return WriteInt8(val ? 1 : 0);
}

size_t AlignedStream::WriteInt16(int16_t val)
{
    WritePadding(sizeof(int16_t));
    size_t size = _stream->WriteInt16(val);
    _block += sizeof(int16_t);
    return size;
}

size_t AlignedStream::WriteInt32(int32_t val)
{
    WritePadding(sizeof(int32_t));
    size_t size = _stream->WriteInt32(val);
    _block += sizeof(int32_t);
    return size;
}

size_t AlignedStream::WriteInt64(int64_t val)
{
    WritePadding(sizeof(int64_t));
    size_t size = _stream->WriteInt64(val);
    _block += sizeof(int64_t);
    return size;
}

size_t AlignedStream::WriteArray(const void *buffer, size_t elem_size, size_t count)
{
    WritePadding(elem_size);
    count = _stream->WriteArray(buffer, elem_size, count);
    _block += count * elem_size;
    return count;
}

size_t AlignedStream::WriteArrayOfInt8(const int8_t *buffer, size_t count)
{
    WritePadding(sizeof(int8_t));
    count = _stream->WriteArrayOfInt8(buffer, count);
    _block += count * sizeof(int8_t);
    return count;
}

size_t AlignedStream::WriteArrayOfInt16(const int16_t *buffer, size_t count)
{
    WritePadding(sizeof(int16_t));
    count = _stream->WriteArrayOfInt16(buffer, count);
    _block += count * sizeof(int16_t);
    return count;
}

size_t AlignedStream::WriteArrayOfInt32(const int32_t *buffer, size_t count)
{
    WritePadding(sizeof(int32_t));
    count = _stream->WriteArrayOfInt32(buffer, count);
    _block += count * sizeof(int32_t);
    return count;
}

size_t AlignedStream::WriteArrayOfInt64(const int64_t *buffer, size_t count)
{
    WritePadding(sizeof(int64_t));
    count = _stream->WriteArrayOfInt64(buffer, count);
    _block += count * sizeof(int64_t);
    return count;
}


// Padding

void AlignedStream::ReadPadding(size_t next_type)
{
    if (next_type == 0)
    {
        return;
    }

    // The next is going to be evenly aligned data type,
    // therefore a padding check must be made
    if (next_type % _baseAlignment == 0)
    {
        int pad = _block % next_type;
        // Read padding only if have to
        if (pad)
        {
            // We do not know and should not care if the underlying stream
            // supports seek, so use read to skip the padding instead.
            for (size_t i = next_type - pad; i > 0; --i)
                _stream->ReadByte();
            _block += next_type - pad;
        }

        _maxAlignment = std::max(_maxAlignment, next_type);
        // Data is evenly aligned now
        if (_block % LargestPossibleType == 0)
        {
            _block = 0;
        }
    }
}

void AlignedStream::WritePadding(size_t next_type)
{
    if (next_type == 0)
    {
        return;
    }

    // The next is going to be evenly aligned data type,
    // therefore a padding check must be made
    if (next_type % _baseAlignment == 0)
    {
        int pad = _block % next_type;
        // Write padding only if have to
        if (pad)
        {
            _stream->WriteByteCount(0, next_type - pad);
            _block += next_type - pad;
        }

        _maxAlignment = std::max(_maxAlignment, next_type);
        // Data is evenly aligned now
        if (_block % LargestPossibleType == 0)
        {
            _block = 0;
        }
    }
}

void AlignedStream::FinalizeBlock()
{
    // Force the stream to read or write remaining padding to match the alignment
    if (_mode == kAligned_Read)
    {
        ReadPadding(_maxAlignment);
    }
    else if (_mode == kAligned_Write)
    {
        WritePadding(_maxAlignment);
    }

    _maxAlignment = 0;
    _block = 0;
}

} // namespace Common
} // namespace AGS
