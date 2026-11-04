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
#include <cstdio>
#include <fstream>
#include <tinyxml2.h>
#include "gtest/gtest.h"
#include "data/agfreader.h"

using namespace AGS;

// Writes a minimal Game.agf with the given root attributes and tries to open it
static Common::HError OpenAgfWithRoot(const char *root_attributes)
{
    const char *path = "agfreader_test_version.agf";
    {
        std::ofstream f(path);
        f << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<AGSEditorDocument " << root_attributes
          << ">\n  <Game />\n</AGSEditorDocument>\n";
    }
    AGF::AGFReader reader;
    Common::HError err = reader.Open(path);
    reader.Close();
    std::remove(path);
    return err;
}

TEST(AGFReader, OpenAcceptsCurrentProjectFormat)
{
    Common::HError err = OpenAgfWithRoot("Version=\"4.0.0.33\" EditorVersion=\"4.00.00.33\"");
    EXPECT_TRUE(err) << (err ? "" : err->FullMessage().GetCStr());
}

TEST(AGFReader, OpenRejectsFormatsThatNeedAnUpgrade)
{
    // Legacy tag (Editor up to 4.0.0.19), and a version-string format older than the readable minimum:
    // both have to be opened and saved in the Editor first
    EXPECT_FALSE(OpenAgfWithRoot("Version=\"3.0.3.2\" VersionIndex=\"26\""));
    EXPECT_FALSE(OpenAgfWithRoot("Version=\"4.0.0.32\""));
}

TEST(AGFReader, OpenRejectsNewerFormats)
{
    EXPECT_FALSE(OpenAgfWithRoot("Version=\"4.0.0.34\""));
    EXPECT_FALSE(OpenAgfWithRoot("Version=\"4.1.0.0\""));
}

// Parses a single XML element and reads its script name with the given parser
static Common::String ReadScriptNameOf(AGF::EntityParser &parser, const char *xml)
{
    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml) != tinyxml2::XML_SUCCESS)
        return "<parse error>";
    return parser.ReadScriptName(doc.RootElement());
}

TEST(AGFReader, AudioClipInheritsFolderSettings)
{
    // A folder keeps its settings in a child element of its own name (AudioClipFolder(XmlNode))
    const char *xml = "<AudioClipFolder Name=\"Music\"><SubFolders /><AudioClips><AudioClip>"
                      "<DefaultPriority>Inherit</DefaultPriority><DefaultRepeat>Inherit</DefaultRepeat>"
                      "<DefaultVolume>-1</DefaultVolume><ID>0</ID><ScriptName>aTheme</ScriptName>"
                      "</AudioClip></AudioClips>"
                      "<AudioClipFolder><DefaultPriority>High</DefaultPriority><DefaultRepeat>True</DefaultRepeat>"
                      "<DefaultVolume>70</DefaultVolume></AudioClipFolder></AudioClipFolder>";
    tinyxml2::XMLDocument doc;
    ASSERT_EQ(tinyxml2::XML_SUCCESS, doc.Parse(xml));
    AGF::AudioClip parser;
    DataUtil::AudioClipData clip;
    parser.ReadAllData(doc.RootElement()->FirstChildElement("AudioClips")->FirstChildElement("AudioClip"), clip);
    EXPECT_TRUE(clip.Repeat);
    EXPECT_EQ(70, clip.Volume);
    EXPECT_EQ(75, clip.Priority); // High
}

TEST(AGFReader, GUIsAreInIDOrder)
{
    // Folders may list GUIs in any order; game data and the script header index them by ID
    const char *path = "agfreader_test_guis.agf";
    {
        std::ofstream f(path);
        f << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
             "<AGSEditorDocument Version=\"4.0.0.33\" EditorVersion=\"4.00.00.33\"><Game><Settings /><GUIs>"
             "<GUIFolder Name=\"Main\"><SubFolders><GUIFolder Name=\"Sub\"><SubFolders /><GUIs>"
             "<GUIMain><NormalGUI><ID>1</ID><Name>gSecond</Name><Controls /></NormalGUI></GUIMain>"
             "</GUIs></GUIFolder></SubFolders><GUIs>"
             "<GUIMain><NormalGUI><ID>0</ID><Name>gFirst</Name><Controls /></NormalGUI></GUIMain>"
             "</GUIs></GUIFolder></GUIs></Game></AGSEditorDocument>\n";
    }
    AGF::AGFReader reader;
    ASSERT_TRUE(reader.Open(path));
    DataUtil::GameRef game;
    AGF::ReadGameRef(game, reader);
    reader.Close();
    std::remove(path);
    ASSERT_EQ(2u, game.GUI.size());
    EXPECT_STREQ("gFirst", game.GUI[0].ScriptName.GetCStr());
    EXPECT_STREQ("gSecond", game.GUI[1].ScriptName.GetCStr());
}

TEST(AGFReader, EntityScriptNames)
{
    // These entities are saved with a <ScriptName> element since the AGS 4 project format;
    // the script header generator declares them by it
    AGF::Dialog dialog;
    AGF::GUIControl control;
    AGF::InventoryItem item;
    AGF::View view;
    EXPECT_STREQ("dDialog1",
                 ReadScriptNameOf(dialog, "<Dialog><ID>0</ID><ScriptName>dDialog1</ScriptName></Dialog>").GetCStr());
    EXPECT_STREQ(
        "Button2",
        ReadScriptNameOf(control, "<GUIButton><ID>1</ID><ScriptName>Button2</ScriptName></GUIButton>").GetCStr());
    EXPECT_STREQ("iKey", ReadScriptNameOf(item, "<InventoryItem><ID>1</ID><DisplayName /><ScriptName>iKey</ScriptName>"
                                                "</InventoryItem>")
                             .GetCStr());
    EXPECT_STREQ("VWALK", ReadScriptNameOf(view, "<View><ID>2</ID><ScriptName>VWALK</ScriptName></View>").GetCStr());
}

TEST(AGFReader, EnumReadSkipSpeech)
{
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOrKeyboardOrTimer,
        AGF::ReadSkipSpeech("MouseOrKeyboardOrTimer"));
    EXPECT_EQ(DataUtil::kSkipSpeech_KeyboardOrTimer,
        AGF::ReadSkipSpeech("KeyboardOnly"));
    EXPECT_EQ(DataUtil::kSkipSpeech_TimerOnly,
        AGF::ReadSkipSpeech("TimerOnly"));
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOrKeyboard,
        AGF::ReadSkipSpeech("MouseOrKeyboard"));
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOrTimer,
        AGF::ReadSkipSpeech("MouseOnly"));
    EXPECT_EQ(DataUtil::kSkipSpeech_KeyboardOnly,
        AGF::ReadSkipSpeech("KeyboardOnlyStrict"));
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOnly,
        AGF::ReadSkipSpeech("MouseOnlyStrict"));
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOrKeyboardOrTimer,
        AGF::ReadSkipSpeech("NotARealValue"));
}

TEST(AGFReader, EnumReadLipSyncType)
{
    EXPECT_EQ(DataUtil::kLipSync_None, AGF::ReadLipSyncType("None"));
    EXPECT_EQ(DataUtil::kLipSync_Text, AGF::ReadLipSyncType("Text"));
    EXPECT_EQ(DataUtil::kLipSync_PamelaVoiceFiles,
        AGF::ReadLipSyncType("PamelaVoiceFiles"));
    EXPECT_EQ(DataUtil::kLipSync_None,
        AGF::ReadLipSyncType("NotARealValue"));
}
