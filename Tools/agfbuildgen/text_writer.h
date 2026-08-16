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
#ifndef AGS_TOOL_AGFBUILDGEN__TEXT_WRITER_H
#define AGS_TOOL_AGFBUILDGEN__TEXT_WRITER_H

#include <memory>
#include <vector>
#include "util/stream.h"
#include "util/string.h"
#include "util/textstreamwriter.h"

using namespace AGS::Common;

namespace WriterUtils
{
    String Join(const String &sep, const std::vector<String> &words);
    String Join(const String& sep, const String& plus_one, const std::vector<String>& words);
}

class Writer
{
public:
    explicit Writer(std::unique_ptr<Stream> &&out);
    virtual ~Writer() = default;

private:
    TextStreamWriter _out;

protected:
    void Write(const String& text);
    void Newline();
    void Line(const String &text, int indent = 0);

    size_t _indent_multiplier = 2;
    char _indent_char = ' ';
};

#endif // AGS_TOOL_AGFBUILDGEN__TEXT_WRITER_H
