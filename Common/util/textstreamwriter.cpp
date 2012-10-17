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

#include <stdarg.h>
#include <stdio.h>
#include "util/textstreamwriter.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

TextStreamWriter::TextStreamWriter(Stream *stream)
    : _stream(stream)
{
}

TextStreamWriter::~TextStreamWriter()
{
    // TODO use shared ptr
    delete _stream;
}

bool TextStreamWriter::IsValid() const
{
    return _stream && _stream->CanWrite();
}

const Stream *TextStreamWriter::GetStream() const
{
    return _stream;
}

void TextStreamWriter::ReleaseStream()
{
    _stream = NULL;
}

bool TextStreamWriter::EOS() const
{
    return _stream ? _stream->EOS() : true;
}

void TextStreamWriter::WriteChar(char c)
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

void TextStreamWriter::WriteString(const String &str)
{
    if (_stream)
    {
        // TODO: replace line-feed characters in string with platform-specific line break
        _stream->Write(str.GetCStr(), str.GetLength());
    }
}

void TextStreamWriter::WriteLine(const String &str)
{
    if (!_stream)
    {
        return;
    }

    // TODO: replace line-feed characters in string with platform-specific line break
    _stream->Write(str.GetCStr(), str.GetLength());
    WriteLineBreak();
}

void TextStreamWriter::WriteFormat(const char *fmt, ...)
{
    if (!_stream)
    {
        return;
    }

    // TODO: replace line-feed characters in format string with platform-specific line break

    String str;
    va_list argptr;
    va_start(argptr, fmt);
    int need_length = vsnprintf(NULL, 0, fmt, argptr);
    va_start(argptr, fmt); // Reset argptr
    char *buffer    = str.GetBuffer(need_length);
    vsprintf(buffer, fmt, argptr);
    va_end(argptr);
    str.ReleaseBuffer();

    WriteString(str);
}

void TextStreamWriter::WriteLineBreak()
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
