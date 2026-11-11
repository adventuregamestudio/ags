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
#include "data/scriptgen.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

static void SetEntity(EntityRef &ent, int id, const char *script_name, const char *type_name = "")
{
    ent.ID = id;
    ent.ScriptName = script_name;
    ent.TypeName = type_name;
}

static EntityRef MakeEntity(int id, const char *script_name, const char *type_name = "")
{
    EntityRef ent;
    SetEntity(ent, id, script_name, type_name);
    return ent;
}

// The expected texts are what the Editor's Tasks.RegenerateScriptHeader writes
// for the same game (with "\n" line ends, where the Editor writes "\r\n")
TEST(ScriptGen, GameAutoScriptHeader)
{
    GameRef game;
    game.Cursors.push_back(MakeEntity(0, "eModeWalkto"));
    game.Cursors.push_back(MakeEntity(1, "eModeLookat"));
    game.Cursors.push_back(MakeEntity(2, "")); // no script name
    game.Fonts.push_back(MakeEntity(0, "eFontNormal"));
    game.Characters.push_back(MakeEntity(0, "cEgo"));
    game.Characters.push_back(MakeEntity(1, "c500")); // no macro for it
    game.AudioTypes.push_back(MakeEntity(1, "eAudioTypeMusic"));
    game.AudioClips.push_back(MakeEntity(0, "aMusic"));
    game.Inventory.push_back(MakeEntity(1, "iKey"));
    DialogRef dialog;
    SetEntity(dialog, 0, "dIntro");
    game.Dialogs.push_back(dialog);
    GUIRef gui;
    SetEntity(gui, 0, "gStatus");
    gui.Controls.push_back(MakeEntity(0, "lblScore", "Label"));
    game.GUI.push_back(gui);
    game.Views.push_back(MakeEntity(1, "EGOWALK"));

    const char *expected =
        "enum CursorMode {\n"
        "  eModeWalkto = 0,\n"
        "  eModeLookat = 1\n"
        "};\n"
        "enum FontType {\n"
        "  eUndefinedFont = -2,  eNullFont = -1,\n"
        "  eFontNormal = 0\n"
        "};\n"
        "import readonly Character *character[2];\n"
        "#define EGO 0\n"
        "import readonly Character *cEgo;\n"
        "import readonly Character *c500;\n"
        "enum AudioType {\n"
        "  eAudioTypeMusic = 1\n"
        "};\n"
        "import AudioClip *aMusic;\n"
        "import readonly InventoryItem *inventory[2];\n"
        "import readonly InventoryItem *iKey;\n"
        "import readonly Dialog *dialog[1];\n"
        "import readonly Dialog *dIntro;\n"
        "import readonly GUI *gui[1];\n"
        "import readonly GUI *gStatus;\n"
        "#define STATUS FindGUIID(\"STATUS\")\n"
        "import readonly Label *lblScore;\n"
        "#define EGOWALK 1\n";
    EXPECT_STREQ(expected, MakeGameAutoScriptHeader(game).GetCStr());
}

TEST(ScriptGen, GameAutoScriptHeaderEmpty)
{
    GameRef game;

    const char *expected =
        "enum CursorMode {\n"
        "eDummyCursor__ = 99  // $AUTOCOMPLETEIGNORE$ \n"
        "};\n"
        "enum FontType {\n"
        "  eUndefinedFont = -2,  eNullFont = -1\n"
        "};\n"
        "import readonly Character *character[1];\n"
        "enum AudioType {\n"
        "eDummyAudioType__ = 99  // $AUTOCOMPLETEIGNORE$ \n"
        "};\n"
        "import readonly InventoryItem *inventory[1];\n"
        "import readonly Dialog *dialog[1];\n"
        "import readonly GUI *gui[1];\n";
    EXPECT_STREQ(expected, MakeGameAutoScriptHeader(game).GetCStr());
}
