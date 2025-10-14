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
#include <array>
#include "gtest/gtest.h"
#include "gui/guimain.h"

using namespace AGS::Common;

TEST(GUI, LabelMacros) {
    GUI::DataVersion = kGameVersion_Current;
    GUI::GameGuiVersion = kGuiVersion_Current;
    GUI::Context.GameTitle = "My Game";
    GUI::Context.Overhotspot = "Location Name";
    GUI::Context.Score = 100;
    GUI::Context.TotalScore = 200;

    struct MacroTestCase
    {
        String Input;
        String Output;
        int LabelMacroType = kLabelMacro_None;
    };

    const std::array<MacroTestCase, 30> findmacrotest = {{
        // Empty string
        {"", "", kLabelMacro_None},
        // Random no-macro string
        {"abcdefghijk", "abcdefghijk", kLabelMacro_None},
        // Simple macro strings
        {"@gamename@", "My Game", kLabelMacro_Gamename}, {"@GAMENAME@", "My Game", kLabelMacro_Gamename},
        {"@overhotspot@", "Location Name", kLabelMacro_Overhotspot}, {"@OVERHOTSPOT@", "Location Name", kLabelMacro_Overhotspot},
        {"@score@", "100", kLabelMacro_Score}, {"@SCORE@", "100", kLabelMacro_Score},
        {"@totalscore@", "200", kLabelMacro_TotalScore}, {"@TOTALSCORE@", "200", kLabelMacro_TotalScore},
        {"@scoretext@", "100 of 200", kLabelMacro_ScoreText}, {"@SCORETEXT@","100 of 200", kLabelMacro_ScoreText},
        // Combination of macros
        {"@gamename@;@overhotspot@", "My Game;Location Name", kLabelMacro_Gamename | kLabelMacro_Overhotspot},
        {"@score@/@totalscore@", "100/200", kLabelMacro_Score | kLabelMacro_TotalScore},
        // Literal macro-symbols
        {"@", "@", kLabelMacro_None}, {"@@", "@@", kLabelMacro_None}, {"a@@@@b", "a@@@@b", kLabelMacro_None},
        // Macros surrounded by literal macro-symbols
        {"@@gamename@", "@My Game", kLabelMacro_Gamename}, {"@@@overhotspot@@", "@@Location Name@", kLabelMacro_Overhotspot},
        // Macros surrounded by other text
        {"before@gamename@after", "beforeMy Gameafter", kLabelMacro_Gamename},
        {"before@overhotspot@mid@gamename@after", "beforeLocation NamemidMy Gameafter", kLabelMacro_Gamename | kLabelMacro_Overhotspot},
        // Not a macro
        {"@abc@", "@abc@", kLabelMacro_None},
        // Misspelled macro
        {"@gamenam@", "@gamenam@", kLabelMacro_None}, {"@gamename1@", "@gamename1@", kLabelMacro_None},
        // Malformed macro
        {"@gamename", "@gamename", kLabelMacro_None}, {"gamename@", "gamename@", kLabelMacro_None},
        // Macros surrounded by non-macros
        {"@before@gamename@after@", "@beforeMy Gameafter@", kLabelMacro_Gamename},
        {"@before@overhotspot@mid@gamename@after@", "@beforeLocation NamemidMy Gameafter@", kLabelMacro_Gamename | kLabelMacro_Overhotspot},
        {"@before@@gamename@@after@", "@before@My Game@after@", kLabelMacro_Gamename}, {"@before@@overhotspot@@mid@@gamename@@after@", "@before@Location Name@mid@My Game@after@", kLabelMacro_Gamename | kLabelMacro_Overhotspot},
    }};

    for (const auto &test : findmacrotest)
    {
        const GUILabelMacro res = GUI::FindLabelMacros(test.Input);
        const String res2 = GUI::ResolveMacroTokens(test.Input);
        ASSERT_TRUE(res == test.LabelMacroType) << "input text: " << test.Input.GetCStr();
        ASSERT_TRUE(res2 == test.Output) << "input text: " << test.Input.GetCStr();
    }
}
