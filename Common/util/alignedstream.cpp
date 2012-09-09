
#include "util/alignedstream.h"
#include "util/datastream.h"

namespace AGS
{
namespace Common
{

CAlignedStream::CAlignedStream(CDataStream *stream, AlignedStreamMode mode, size_t alignment)
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
    if (!_stream)
    {
        return;
    }

    // Force the stream to read or write remaining padding to match the alignment
    if (_mode == kAligned_Read)
    {
        ReadPadding(_alignment);
    }
    else if (_mode == kAligned_Write)
    {
        WritePadding(_alignment);
    }

    // TODO: use shared ptr
    delete _stream;
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

void CAlignedStream::ReleaseStream()
{
    if (!_stream)
    {
        return;
    }

    // Force the stream to read or write remaining padding to match the alignment
    if (_mode == kAligned_Read)
    {
        ReadPadding(_alignment);
    }
    else if (_mode == kAligned_Write)
    {
        WritePadding(_alignment);
    }

    _stream = NULL;
}

int CAlignedStream::Seek(StreamSeek seek, int pos)
{
    // Not supported
    return 0;
}

int CAlignedStream::ReadByte()
{
    byte b = 0;
    if (_stream)
    {
        ReadPadding(sizeof(byte));
        b = _stream->ReadByte();
        _block += sizeof(byte);
    }
    return b;
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
    if (_stream)
    {
        ReadPadding(sizeof(int8_t));
        size = _stream->Read(buffer, size);
        _block += size;
        return size;
    }
    return 0;
}

int CAlignedStream::ReadArray(void *buffer, int elem_size, int count)
{
    if (_stream)
    {
        ReadPadding(sizeof(elem_size));
        count = _stream->ReadArray(buffer, elem_size, count);
        _block += count * elem_size;
        return count;
    }
    return 0;
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

int CAlignedStream::WriteByte(byte b)
{
    if (_stream)
    {
        WritePadding(sizeof(byte));
        b = _stream->WriteByte(b);
        _block += sizeof(byte);
        return b;
    }
    return 0;
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
    if (_stream)
    {
        WritePadding(sizeof(int8_t));
        size = _stream->Write(buffer, size);
        _block += size;
        return size;
    }
    return 0;
}

int CAlignedStream::WriteArray(const void *buffer, int elem_size, int count)
{
    if (_stream)
    {
        WritePadding(sizeof(elem_size));
        count = _stream->WriteArray(buffer, elem_size, count);
        _block += count * elem_size;
        return count;
    }
    return 0;
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
            _stream->Read(_paddingBuffer, _alignment - pad);
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
            _stream->Write(_paddingBuffer, _alignment - pad);
        }
        // Data is evenly aligned now
        _block = 0;
    }
}

} // namespace Common
} // namespace AGS

