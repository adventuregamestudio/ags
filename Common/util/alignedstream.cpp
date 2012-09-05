
#include "util/alignedstream.h"

namespace AGS
{
namespace Common
{

CAlignedStream::CAlignedStream(IStream *stream, AlignedStreamMode mode, size_t alignment)
    : _stream(stream)
    , _mode(mode)
    , _alignment(alignment)
    , _block(0)
{
}

CAlignedStream::~CAlignedStream()
{
    Close();
}

void CAlignedStream::Close()
{
    // Force the stream to read or write remaining padding to match the alignment
    if (_mode == kAligned_Read)
    {
        ReadPadding(_alignment);
    }
    else if (_mode == kAligned_Write)
    {
        WritePadding(_alignment);
    }

    // Release stream
    _stream = NULL;
}

bool CAlignedStream::IsValid() const
{
    return _stream && _stream->IsValid();
}

bool CAlignedStream::EOS() const
{
    return _stream ? _stream->EOS() : true;
}

int CAlignedStream::GetLength() const
{
    return _stream ? _stream->GetLength() : 0;
}

int CAlignedStream::GetPosition() const
{
    return _stream ? _stream->GetPosition() : 0;
}

bool CAlignedStream::CanRead() const
{
    return _stream ? (_mode == kAligned_Read && _stream->CanRead()) : false;
}

bool CAlignedStream::CanWrite() const
{
    return _stream ? (_mode == kAligned_Write && _stream->CanWrite()) : false;
}

bool CAlignedStream::CanSeek() const
{
    // Aligned stream does not support seeking, hence that will break padding count
    return false;
}

int CAlignedStream::Seek(StreamSeek seek, int pos)
{
    // Not supported
    return 0;
}

int8_t CAlignedStream::ReadInt8()
{
    int8_t val = 0;
    if (_stream)
    {
        ReadPadding(sizeof(int8_t));
        val = _stream->ReadInt8();
        _block += sizeof(int8_t);
    }
    return val;
}

int16_t CAlignedStream::ReadInt16()
{
    int16_t val = 0;
    if (_stream)
    {
        ReadPadding(sizeof(int16_t));
        val = _stream->ReadInt16();
        _block += sizeof(int16_t);
    }
    return val;
}

int32_t CAlignedStream::ReadInt32()
{
    int32_t val = 0;
    if (_stream)
    {
        ReadPadding(sizeof(int32_t));
        val = _stream->ReadInt32();
        _block += sizeof(int32_t);
    }
    return val;
}

int64_t CAlignedStream::ReadInt64()
{
    int64_t val = 0;
    if (_stream)
    {
        ReadPadding(sizeof(int64_t));
        val = _stream->ReadInt64();
        _block += sizeof(int64_t);
    }
    return val;
}

int CAlignedStream::Read(void *buffer, int size)
{
    int read_size = 0;
    if (_stream)
    {
        ReadPadding(sizeof(int8_t));
        read_size = _stream->Read(buffer, size);
        _block += size;
    }
    return read_size;
}

int CAlignedStream::ReadArray(void *buffer, int elem_size, int count)
{
    int read_size = 0;
    if (_stream)
    {
        ReadPadding(sizeof(elem_size));
        read_size = _stream->Read(buffer, elem_size * count);
        _block += elem_size * count;
    }
    return read_size;
}

CString CAlignedStream::ReadString(int max_chars)
{
    CString str;
    if (_stream)
    {
        ReadPadding(sizeof(char));
        str = _stream->ReadString(max_chars);
        _block += str.GetLength() + 1; // TODO: the 1 last byte is not guaranteed here, do otherwise
    }
    return "";
}

void CAlignedStream::WriteInt8(int8_t val)
{
    if (_stream)
    {
        WritePadding(sizeof(int8_t));
        _stream->WriteInt8(val);
        _block += sizeof(int8_t);
    }
}

void CAlignedStream::WriteInt16(int16_t val)
{
    if (_stream)
    {
        WritePadding(sizeof(int16_t));
        _stream->WriteInt16(val);
        _block += sizeof(int16_t);
    }
}

void CAlignedStream::WriteInt32(int32_t val)
{
    if (_stream)
    {
        WritePadding(sizeof(int32_t));
        _stream->WriteInt32(val);
        _block += sizeof(int32_t);
    }
}

void CAlignedStream::WriteInt64(int64_t val)
{
    if (_stream)
    {
        WritePadding(sizeof(int64_t));
        _stream->WriteInt64(val);
        _block += sizeof(int64_t);
    }
}

int CAlignedStream::Write(const void *buffer, int size)
{
    int write_size = 0;
    if (_stream)
    {
        WritePadding(sizeof(int8_t));
        write_size = _stream->Write(buffer, size);
        _block += size;
    }
    return write_size;
}

int CAlignedStream::WriteArray(const void *buffer, int elem_size, int count)
{
    int write_size = 0;
    if (_stream)
    {
        WritePadding(sizeof(elem_size));
        write_size = _stream->Write(buffer, elem_size * count);
        _block += elem_size * count;
    }
    return write_size;
}

void CAlignedStream::WriteString(const CString &str)
{
    if (_stream)
    {
        WritePadding(sizeof(char));
        _stream->WriteString(str);
        _block += str.GetLength() + 1;
    }
}

void CAlignedStream::ReadPadding(size_t next_type)
{
    if (!IsValid())
    {
        return;
    }

    // The next is going to be evenly aligned data type,
    // therefore a padding check must be made
    if (next_type % _alignment == 0)
    {
        int pad = _block % _alignment;
        // Read padding only if have to
        if (pad)
        {
            // We do not know and should not care if the underlying stream
            // supports seek, so use read to skip the padding instead.
            _stream->Read(_paddingBuffer, _alignment - (_block % _alignment));
        }
        // Data is evenly aligned now
        _block = 0;
    }
}

void CAlignedStream::WritePadding(size_t next_type)
{
    if (!IsValid())
    {
        return;
    }

    // The next is going to be evenly aligned data type,
    // therefore a padding check must be made
    if (next_type % _alignment == 0)
    {
        int pad = _block % _alignment;
        // Write padding only if have to
        if (pad)
        {
            _stream->Write(_paddingBuffer, _alignment - (_block % _alignment));
        }
        // Data is evenly aligned now
        _block = 0;
    }
}

} // namespace Common
} // namespace AGS

