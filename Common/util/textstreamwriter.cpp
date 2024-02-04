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
#include <stdarg.h>
#include "core/platform.h"
#include "util/textstreamwriter.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

// TODO: perhaps let configure line break character per TextWriter object?
#if AGS_PLATFORM_OS_WINDOWS
static const char Endl[2] = {'\r', '\n'};
#else
static const char Endl[1] = {'\n'};
#endif


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
    _stream = nullptr;
}

bool TextStreamWriter::EOS() const
{
    return _stream->EOS();
}

void TextStreamWriter::WriteChar(char c)
{
    _stream->WriteByte(c);
}

void TextStreamWriter::WriteString(const String &str)
{
    _stream->Write(str.GetCStr(), str.GetLength());
}

void TextStreamWriter::WriteLine(const String &str)
{
    // TODO: perhaps let configure line break character?
    _stream->Write(str.GetCStr(), str.GetLength());
    _stream->Write(Endl, sizeof(Endl));
}

void TextStreamWriter::WriteFormat(const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    _buf.FormatV(fmt, argptr);
    va_end(argptr);
    _stream->Write(_buf.GetCStr(), _buf.GetLength());
}

void TextStreamWriter::WriteLineBreak()
{
    _stream->Write(Endl, sizeof(Endl));
}

} // namespace Common
} // namespace AGS
