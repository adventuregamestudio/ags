
#include "util/textstreamreader.h"
#include "util/stream.h"
#include "util/math.h"

namespace AGS
{
namespace Common
{

CTextStreamReader::CTextStreamReader(IStream *stream)
    : _stream(stream)
{
}

CTextStreamReader::~CTextStreamReader()
{
    // TODO: use shared ptr
    delete _stream;
}

bool CTextStreamReader::IsValid() const
{
    return _stream && _stream->CanRead();
}

const IStream *CTextStreamReader::GetStream() const
{
    return _stream;
}

void CTextStreamReader::ReleaseStream()
{
    _stream = NULL;
}

bool CTextStreamReader::EOS() const
{
    return _stream ? _stream->EOS() : true;
}

char CTextStreamReader::ReadChar()
{
    if (_stream)
    {
        // Skip carriage-returns
        char c;
        do
        {
            c = _stream->ReadByte();
        }
        while (!_stream->EOS() && c == '\r');
        return c;
    }
    return '\0';
}

CString CTextStreamReader::ReadString(int length)
{
    if (!_stream)
    {
        return "";
    }

    CString str;
    char *buffer    = str.GetBuffer(length);
    int chars_read  = _stream->Read(buffer, length);
    str.ReleaseBuffer(chars_read);
    // TODO: remove carriage-return characters
    return str;
}

CString CTextStreamReader::ReadLine()
{
    // TODO
    // Probably it is possible to group DataStream::ReadString with this,
    // both use similar algorythm, difference is only in terminator chars

    if (!_stream)
    {
        return "";
    }

    CString str;
    int chars_read_last = 0;
    int line_break_position = -1;
    // Read a chunk of memory to buffer and seek for null-terminator,
    // if not found, repeat until EOS
    const int single_chunk_length = 3000;
    const int max_chars = 5000000;
    char char_buffer[single_chunk_length + 1];
    do
    {
        chars_read_last = _stream->Read(char_buffer, single_chunk_length);
        char *seek_ptr = char_buffer;
        // CHECKME: is finding '\n' is enough to deduce line end?
        // Should it be strict "\r\n" instead? Or platform-dependant line-break?
        for (int c = 0; c < chars_read_last && *seek_ptr != '\n'; ++c, ++seek_ptr);

        int append_length = 0;
        int str_len = str.GetLength();
        if (*seek_ptr == '\n')
        {
            line_break_position = seek_ptr - char_buffer;
            if (str_len < max_chars)
            {
                append_length = Math::Min(line_break_position, max_chars - str_len);
            }
        }
        else
        {
            append_length = Math::Min(chars_read_last, max_chars - str_len);
        }

        if (append_length > 0)
        {
            char_buffer[append_length] = '\0';
            str.Append(char_buffer);
        }
    }
    while (!EOS() && line_break_position < 0);

    // If null-terminator was found make sure stream is positioned at the next
    // byte after line end
    if (line_break_position >= 0)
    {
        // CHECKME: what if stream does not support seek? need an algorythm fork for that
        // the seek offset should be negative
        _stream->Seek(kSeekCurrent, line_break_position - chars_read_last + 1 /* beyond line feed */);
    }

    return str;

    // TODO: remove carriage-return characters
}

CString CTextStreamReader::ReadAll()
{
    if (_stream)
    {
        return ReadString(_stream->GetLength() - _stream->GetPosition());
    }
    return "";
}

} // namespace Common
} // namespace AGS
