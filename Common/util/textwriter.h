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
// Specialized interface for writing plain text to the underlying source
//
//=============================================================================
#ifndef __AGS_CN_UTIL__TEXTWRITER_H
#define __AGS_CN_UTIL__TEXTWRITER_H

#include "util/string.h"

namespace AGS
{
namespace Common
{

class TextWriter
{
public:
    virtual ~TextWriter() = default;

    virtual bool    IsValid() const                         = 0;

    // Write single character
    virtual void    WriteChar(char c)                       = 0;
    // Write string as a plain text (without null-terminator)
    virtual void    WriteString(const String &str)          = 0;
    // Write string and add line break at the end
    virtual void    WriteLine(const String &str)            = 0;
    // Write formatted string (see *printf)
    virtual void    WriteFormat(const char *fmt, ...)       = 0;
    virtual void    WriteLineBreak()                        = 0;

    // Flushes the underlying device
    virtual void    Flush() = 0;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__TEXTWRITER_H
