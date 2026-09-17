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
#include "ac/runtime_defines.h"
#include "data/data_helpers.h"
#include "util/string_utils.h"

using namespace AGS::Common;

// This is a reimplementation of the Engine's replace_message_tokens()
// TODO: Had to make a clone of the function here, because original one
// brings too many dependencies, which are difficult to resolve here.
// Revise this later.
const int GameNumItems = 100;
int PlayerInvQ[GameNumItems];
int GlobalInts[MAXGSVALUES];
String replace_message_tokens(const String &text)
{
    std::function<bool(const String&, String&)> fn_resolve = [](const String &macro, String &result)->bool
        { 
            // Inventory quantity
            if (macro.StartsWith("IN"))
            {
                int index = atoi(&macro[2]);
                if ((index < 1) || (index >= GameNumItems))
                {
                    result = "error";
                    return true; // this is for the sake of this test only FIXME later
                }
                result = StrUtil::IntToString(PlayerInvQ[index]);
                return true;
            }
            // Global integer value
            else if (macro.StartsWith("GI"))
            {
                int index = atoi(&macro[2]);
                if ((index < 0) || (index >= MAXGSVALUES))
                {
                    result = "error";
                    return true; // this is for the sake of this test only FIXME later
                }
                result = StrUtil::IntToString(GlobalInts[index]);
                return true;
            }
            return false;
        };
    return ResolveMacroTokens(text, fn_resolve);
}

TEST(GameMessages, MessageMacros) {
    PlayerInvQ[0] = 0; // inventory index 0 cannot be used in 3.x
    PlayerInvQ[1] = 1;
    PlayerInvQ[2] = 2;
    PlayerInvQ[3] = 3;
    GlobalInts[0] = 100;
    GlobalInts[1] = 101;
    GlobalInts[2] = 102;
    GlobalInts[3] = -103;

    struct MacroTestCase
    {
        String Input;
        String Output;
    };

    const std::array<MacroTestCase, 30> findmacrotest = {{
        // Empty string
        {"", ""},
        // Random no-macro string
        {"abcdefghijk", "abcdefghijk"},
        // Simple macro strings
        {"@IN1@", "1"}, {"@GI0@", "100"}, {"@IN2@", "2"}, {"@GI1@", "101"}, {"@IN3@", "3"}, {"@GI2@", "102"},
        // Invalid indexes
        {"@IN-1@", "error"}, {"@IN101@", "error"}, {"@GI-1@", "error"}, {"@GI501@", "error"},
        // Combination of macros
        {"@IN3@;@GI3@", "3;-103"},
        // Literal macro-symbols
        {"@", "@"}, {"@@", "@@"}, {"a@@@@b", "a@@@@b"},
        // Macros surrounded by literal macro-symbols
        {"@@IN2@", "@2"}, {"@@@GI2@@", "@@102@"},
        // Macros surrounded by other text
        {"before@IN2@after", "before2after"},
        {"before@IN2@mid@GI2@after", "before2mid102after"},
        // Not a macro
        {"@abc@", "@abc@"},
        // Misspelled macro
        {"@IX@", "@IX@"}, {"@GX@", "@GX@"},
        // Malformed macro
        {"@IN3", "@IN3"}, {"GI3@", "GI3@"},
        // Macros surrounded by non-macros
        {"@before@IN2@after@", "@before2after@"},
        {"@before@IN2@mid@GI2@after@", "@before2mid102after@"},
    }};

    for (const auto &test : findmacrotest)
    {
        const String res2 = replace_message_tokens(test.Input);
        ASSERT_TRUE(res2 == test.Output) << "input text: " << test.Input.GetCStr();
    }
}
