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
#include "game/data_helpers.h"
#include "util/string_utils.h"

using namespace AGS;
using namespace AGS::Common;

TEST(DataHelpers, PreprocessLineForOldStyleLinebreaks)
{
    ASSERT_TRUE(PreprocessLineForOldStyleLinebreaks("TextText") == "TextText");
    ASSERT_TRUE(PreprocessLineForOldStyleLinebreaks("Text[Text") == "Text[Text");
    ASSERT_TRUE(PreprocessLineForOldStyleLinebreaks("Text\\[Text") == "Text\\\\[Text");
    ASSERT_TRUE(PreprocessLineForOldStyleLinebreaks("Text\\\\[Text") == "Text\\\\[Text");
    ASSERT_TRUE(PreprocessLineForOldStyleLinebreaks("Text\\\\\\[Text") == "Text\\\\\\\\[Text");
    ASSERT_TRUE(PreprocessLineForOldStyleLinebreaks("Text\\\\\\[[Text") == "Text\\\\\\\\[[Text");
    ASSERT_TRUE(PreprocessLineForOldStyleLinebreaks("Text\\nText") == "Text\\nText");
}

TEST(DataHelpers, PreprocessLineForOldStyleLinebreaks_WithUnescape)
{
    // This tests a result of chaining "preprocess" with "unescape",
    // just to make sure they suit each other
    ASSERT_TRUE(StrUtil::Unescape(PreprocessLineForOldStyleLinebreaks("TextText")) == "TextText");
    ASSERT_TRUE(StrUtil::Unescape(PreprocessLineForOldStyleLinebreaks("Text[Text")) == "Text[Text");
    ASSERT_TRUE(StrUtil::Unescape(PreprocessLineForOldStyleLinebreaks("Text\\[Text")) == "Text\\[Text");
    // NOTE: there's no difference in the end result compared to the previous test
    // where the '[' is escaped, therefore in practice the engine will still think
    // that the '[' is escaped at runtime.
    // This cannot be resolved without using '\n' instead.
    ASSERT_TRUE(StrUtil::Unescape(PreprocessLineForOldStyleLinebreaks("Text\\\\[Text")) == "Text\\[Text");
    ASSERT_TRUE(StrUtil::Unescape(PreprocessLineForOldStyleLinebreaks("Text\\\\\\[Text")) == "Text\\\\[Text");
    ASSERT_TRUE(StrUtil::Unescape(PreprocessLineForOldStyleLinebreaks("Text\\\\\\[[Text")) == "Text\\\\[[Text");
    ASSERT_TRUE(StrUtil::Unescape(PreprocessLineForOldStyleLinebreaks("Text\\nText")) == "Text\nText");
}
