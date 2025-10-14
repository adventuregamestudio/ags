//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Class for writing plain text to the stream
//
//=============================================================================
#ifndef __AGS_CN_UTIL__TEXTSTREAMWRITER_H
#define __AGS_CN_UTIL__TEXTSTREAMWRITER_H

#include <memory>
#include "util/stream.h"
#include "util/textwriter.h"

namespace AGS
{
namespace Common
{

class TextStreamWriter : public TextWriter
{
public:
    TextStreamWriter(std::unique_ptr<Stream> &&stream)
        : _stream(std::move(stream)) {}
    ~TextStreamWriter() override = default;

    std::unique_ptr<Stream> ReleaseStream() { return std::move(_stream); }

    bool    IsValid() const override;
    bool    EOS() const;

    // Write single character
    void    WriteChar(char c) override;
    // Write string as a plain text (without null-terminator)
    void    WriteString(const String &str) override;
    // Write string and add line break at the end
    void    WriteLine(const String &str) override;
    // Write formatted string (see *printf)
    void    WriteFormat(const char *fmt, ...) override;
    void    WriteLineBreak() override;

    // Flushes the underlying device
    void    Flush() override;

private:
    std::unique_ptr<Stream> _stream;
    String  _buf; // formatting string buffer
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__TEXTSTREAMWRITER_H
