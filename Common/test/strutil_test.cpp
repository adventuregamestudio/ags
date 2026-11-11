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
#include "util/string_utils.h"

using namespace AGS::Common;

TEST(StrUtil, Unescape) {
    String s = "Text\\\\Text";
    ASSERT_TRUE(StrUtil::Unescape(s) == "Text\\Text");

    s = "Text\\\\nText";
    ASSERT_TRUE(StrUtil::Unescape(s) == "Text\\nText");

    s = "Text\\nText";
    ASSERT_TRUE(StrUtil::Unescape(s) == "Text\nText");

    s = "Text\\rText";
    ASSERT_TRUE(StrUtil::Unescape(s) == "Text\rText");

    // up to 3 octal digits, max value 255
    s = "\\101\\1\\0012 \\400";
    ASSERT_TRUE(StrUtil::Unescape(s) == "A\x01\x01" "2  0");
    // a zero code is not unescaped
    s = "\\0 \\00";
    ASSERT_TRUE(StrUtil::Unescape(s) == "0 00");
    // unknown sequence loses its backslash
    s = "\\[\\x41";
    ASSERT_TRUE(StrUtil::Unescape(s) == "[x41");
    // a backslash at the end of the string stays
    s = "Text\\";
    ASSERT_TRUE(StrUtil::Unescape(s) == "Text\\");
    s = "\\";
    ASSERT_TRUE(StrUtil::Unescape(s) == "\\");
}

TEST(StrUtil, Escape) {
    String s = "Text";
    ASSERT_TRUE(StrUtil::Escape(s) == "Text");

    s = "Back\\slash \"quoted\"\r\n\t\x01 end";
    ASSERT_TRUE(StrUtil::Escape(s) == "Back\\\\slash \\\"quoted\\\"\\r\\n\\t\\001 end");

    // Unescape is the reverse
    s = "a\\b \"c\"\a\b\f\n\r\t\v\x01\x1F d";
    ASSERT_TRUE(StrUtil::Unescape(StrUtil::Escape(s)) == s);
}

TEST(StrUtil, GetKeyValue)
{
    auto kv = StrUtil::GetKeyValue(" Font1 = Size=12 ");
    EXPECT_STREQ("Font1", kv.first.GetCStr());
    EXPECT_STREQ("Size=12", kv.second.GetCStr());
    // the given separator, not '='
    kv = StrUtil::GetKeyValue("PARSERWORD:5", ':');
    EXPECT_STREQ("PARSERWORD", kv.first.GetCStr());
    EXPECT_STREQ("5", kv.second.GetCStr());
    kv = StrUtil::GetKeyValue("NoSeparator");
    EXPECT_STREQ("NoSeparator", kv.first.GetCStr());
    EXPECT_TRUE(kv.second.IsEmpty());
}
