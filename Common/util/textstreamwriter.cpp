
#include <stdarg.h>
#include <stdio.h>
#include "util/textstreamwriter.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

CTextStreamWriter::CTextStreamWriter(IStream *stream)
    : _stream(stream)
{
}

CTextStreamWriter::~CTextStreamWriter()
{
    // TODO use shared ptr
    delete _stream;
}

bool CTextStreamWriter::IsValid() const
{
    return _stream && _stream->CanWrite();
}

const IStream *CTextStreamWriter::GetStream() const
{
    return _stream;
}

void CTextStreamWriter::ReleaseStream()
{
    _stream = NULL;
}

bool CTextStreamWriter::EOS() const
{
    return _stream ? _stream->EOS() : true;
}

void CTextStreamWriter::WriteChar(char c)
{
    // Substitute line-feed character with platform-specific line break
    if (c == '\n')
    {
        WriteLineBreak();
        return;
    }

    if (_stream)
    {
        _stream->WriteByte(c);
    }
}

void CTextStreamWriter::WriteString(const CString &str)
{
    if (_stream)
    {
        // TODO: replace line-feed characters in string with platform-specific line break
        _stream->Write(str.GetCStr(), str.GetLength());
    }
}

void CTextStreamWriter::WriteLine(const CString &str)
{
    if (!_stream)
    {
        return;
    }

    // TODO: replace line-feed characters in string with platform-specific line break
    _stream->Write(str.GetCStr(), str.GetLength());
    WriteLineBreak();
}

void CTextStreamWriter::WriteFormat(const CString &fmt, ...)
{
    if (!_stream)
    {
        return;
    }

    // TODO: replace line-feed characters in format string with platform-specific line break

    CString str;
    va_list argptr;
    va_start(argptr, fmt);
    int need_length = _vscprintf(fmt.GetCStr(), argptr);
    char *buffer    = str.GetBuffer(need_length);
    vsprintf(buffer, fmt.GetCStr(), argptr);
    va_end(argptr);
    str.ReleaseBuffer();

    WriteString(str);
}

void CTextStreamWriter::WriteLineBreak()
{
    if (_stream)
    {
        // CHECKME: should this be platform-dependant?
        // carriage-return & line-feed
        _stream->Write("\r\n", 2);
    }
}

} // namespace Common
} // namespace AGS
