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
#include "text_writer.h"

// -- Writer Utils

String WriterUtils::Join(const String& sep, const std::vector<String>& words)
{
    if (words.empty())
        return String();
    if (words.size() == 1)
        return words[0];

    String joined;
    bool first = true;
    for (const auto& word : words)
    {
        if (!first) joined.Append(sep);
        joined.Append(word);
        first = false;
    }
    return joined;
}

String WriterUtils::Join(const String& sep, const String& plus_one, const std::vector<String>& words)
{
    String joined = plus_one;
    joined.Append(sep);
    joined.Append(Join(sep, words));
    return joined;
}

// -- Writer

Writer::Writer(std::unique_ptr<Stream> &&out) : _out(std::move(out))
{
}

void Writer::Write(const String &text)
{
    _out.WriteString(text);
}

void Writer::Line(const String& text, int indent)
{
    if (indent > 0)
    {
        String leading_space;
        leading_space.FillString(_indent_char, indent * _indent_multiplier);
        Write(leading_space);
    }
    Write(text);
    Newline();
}

void Writer::Newline()
{
    Write("\n");
}

