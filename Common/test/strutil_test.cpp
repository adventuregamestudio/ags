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
