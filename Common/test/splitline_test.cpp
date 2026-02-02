//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "gtest/gtest.h"
#include "font/fonts.h"

using namespace AGS::Common;

TEST(SplitLines, WrapAtLinebreak) {
    SplitLines lines;
    split_lines("TextText", lines, INT32_MAX, 0);
    ASSERT_TRUE(lines.Count() == 1);
    ASSERT_TRUE(lines[0] == "TextText");

    // Standard linebreak ('\n')
    split_lines("Text\nText", lines, INT32_MAX, 0);
    ASSERT_TRUE(lines.Count() == 2);
    ASSERT_TRUE(lines[0] == "Text");
    ASSERT_TRUE(lines[1] == "Text");

    split_lines("Text\n\nText", lines, INT32_MAX, 0);
    ASSERT_TRUE(lines.Count() == 3);
    ASSERT_TRUE(lines[0] == "Text");
    ASSERT_TRUE(lines[1] == "");
    ASSERT_TRUE(lines[2] == "Text");
}

TEST(SplitLines, WrapAtOldStyleLinebreak) {
    SplitLines lines;
    // Old-style linebreak ('[')
    split_lines("Text[Text", lines, INT32_MAX, 0);
    ASSERT_TRUE(lines.Count() == 2);
    ASSERT_TRUE(lines[0] == "Text");
    ASSERT_TRUE(lines[1] == "Text");

    split_lines("Text[[Text", lines, INT32_MAX, 0);
    ASSERT_TRUE(lines.Count() == 3);
    ASSERT_TRUE(lines[0] == "Text");
    ASSERT_TRUE(lines[1] == "");
    ASSERT_TRUE(lines[2] == "Text");

    // Escaped old-style linebreak ('[')
    split_lines("Text\\[Text", lines, INT32_MAX, 0);
    ASSERT_TRUE(lines.Count() == 1);
    ASSERT_TRUE(lines[0] == "Text[Text");

    // NOTE: compiled game strings handle '\\' as a standalone character,
    // not affecting surrounding chars, with a *single exception* of "\\["
    // sequence which is treated as a literal '['.
    split_lines("Text\\\\[Text", lines, INT32_MAX, 0);
    ASSERT_TRUE(lines.Count() == 1);
    ASSERT_TRUE(lines[0] == "Text\\[Text");

    split_lines("Text\\\\[[Text", lines, INT32_MAX, 0);
    ASSERT_TRUE(lines.Count() == 2);
    ASSERT_TRUE(lines[0] == "Text\\[");
    ASSERT_TRUE(lines[1] == "Text");
}
