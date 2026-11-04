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

TEST(StrUtil, SplitPreservingDoublequoted)
{
    String s = "\"path with, comma\":\"value with, comma\",\"second path\":\"second value\",\"third, comma\":\"third value,\"";
    auto result = StrUtil::SplitPreservingDoublequoted(s, ',');
    EXPECT_EQ(3, result.size());
    EXPECT_STREQ("\"path with, comma\":\"value with, comma\"", result[0].GetCStr());
    EXPECT_STREQ("\"second path\":\"second value\"", result[1].GetCStr());
    EXPECT_STREQ("\"third, comma\":\"third value,\"", result[2].GetCStr());

    // Additional test with extra commas
    s = ",,\"path with, comma\":\"value with, comma\",\"second path\":\"second value\",,\"third, comma\":\"third value,\",,";
    result = StrUtil::SplitPreservingDoublequoted(s, ',');
    EXPECT_EQ(8, result.size());
    EXPECT_STREQ("", result[0].GetCStr());
    EXPECT_STREQ("", result[1].GetCStr());
    EXPECT_STREQ("\"path with, comma\":\"value with, comma\"", result[2].GetCStr());
    EXPECT_STREQ("\"second path\":\"second value\"", result[3].GetCStr());
    EXPECT_STREQ("", result[4].GetCStr());
    EXPECT_STREQ("\"third, comma\":\"third value,\"", result[5].GetCStr());
    EXPECT_STREQ("", result[6].GetCStr());
    EXPECT_STREQ("", result[7].GetCStr());
}
