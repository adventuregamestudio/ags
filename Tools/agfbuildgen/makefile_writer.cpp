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
#include "makefile_writer.h"

using namespace AGS::Common;
using namespace WriterUtils;

MakefileWriter::MakefileWriter(const String &filename) : Writer(filename)
{
    _indent_char = '\t';
    _indent_multiplier = 1;
}

void MakefileWriter::Comment(const String& text)
{
    Write("# ");
    Write(text);
    Newline();
}

void MakefileWriter::Variable(const String& key, const String& value)
{
    if (!value.IsNullOrSpace())
    {
        Line(String::FromFormat("%s = %s", key.GetCStr(), value.GetCStr()), 0);
    }
}

void MakefileWriter::Rule(const String& target, const std::vector<String>& dependencies, const String& command)
{
    Line(String::FromFormat("%s: %s", target.GetCStr(), Join(" ", dependencies).GetCStr()));
    Line(command, 1);
}

void MakefileWriter::Phony(const String& target)
{
    Line(String::FromFormat(".PHONY: %s", target.GetCStr()));
}

