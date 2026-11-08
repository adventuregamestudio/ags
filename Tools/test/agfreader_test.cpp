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
#include <string>
#include <tinyxml2.h>
#include "gtest/gtest.h"
#include "data/agfreader.h"
#include "data/scriptgen.h"

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

// The Version attribute for a project format: the given one with a step added to one of its four
// numbers, so that the tests follow the versions the reader is built with
static std::string VersionAttribute(const char *version, int number = 3, int step = 0)
{
    int v[4] = {};
    EXPECT_EQ(4, sscanf(version, "%d.%d.%d.%d", &v[0], &v[1], &v[2], &v[3])) << version;
    v[number] += step;
    for (int i = number + 1; step != 0 && i < 4; ++i)
        v[i] = 0;
    char attribute[64];
    snprintf(attribute, sizeof(attribute), "Version=\"%d.%d.%d.%d\"", v[0], v[1], v[2], v[3]);
    return attribute;
}

TEST(AGFReader, OpenAcceptsCurrentProjectFormat)
{
    Common::HError err = OpenAgfWithRoot(VersionAttribute(AGF::LATEST_XML_VERSION).c_str());
    EXPECT_TRUE(err) << (err ? "" : err->FullMessage().GetCStr());
    err = OpenAgfWithRoot(VersionAttribute(AGF::OLDEST_READABLE_XML_VERSION).c_str());
    EXPECT_TRUE(err) << (err ? "" : err->FullMessage().GetCStr());
}

TEST(AGFReader, OpenRejectsFormatsThatNeedAnUpgrade)
{
    // Legacy tag (Editor up to 4.0.0.19), and a version-string format older than the readable minimum:
    // both have to be opened and saved in the Editor first
    EXPECT_FALSE(OpenAgfWithRoot("Version=\"3.0.3.2\" VersionIndex=\"26\""));
    EXPECT_FALSE(OpenAgfWithRoot(VersionAttribute(AGF::OLDEST_READABLE_XML_VERSION, 3, -1).c_str()));
}

TEST(AGFReader, OpenRejectsNewerFormats)
{
    EXPECT_FALSE(OpenAgfWithRoot(VersionAttribute(AGF::LATEST_XML_VERSION, 3, +1).c_str()));
    EXPECT_FALSE(OpenAgfWithRoot(VersionAttribute(AGF::LATEST_XML_VERSION, 1, +1).c_str()));
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

TEST(AGFReader, GlobalVariablesMayBeArrays)
{
    tinyxml2::XMLDocument doc;
    ASSERT_EQ(tinyxml2::XML_SUCCESS, doc.Parse(
        "<Game><GlobalVariables><Variables>"
        "<GlobalVariable><ArraySize>0</ArraySize><ArrayType>None</ArrayType><DefaultValue>True</DefaultValue>"
        "<Name>flag</Name><Type>bool</Type></GlobalVariable>"
        "<GlobalVariable><ArraySize>5</ArraySize><ArrayType>Array</ArrayType><DefaultValue></DefaultValue>"
        "<Name>list</Name><Type>int</Type></GlobalVariable>"
        "<GlobalVariable><ArraySize>0</ArraySize><ArrayType>DynamicArray</ArrayType><DefaultValue>x</DefaultValue>"
        "<Name>names</Name><Type>String</Type></GlobalVariable>"
        "</Variables></GlobalVariables></Game>"));
    std::vector<DataUtil::Variable> vars;
    AGF::ReadGlobalVariables(vars, doc.RootElement());
    ASSERT_EQ(3u, vars.size());

    // as the editor's GlobalVariablesComponent writes them
    EXPECT_STREQ("import bool flag;\nimport int list[5];\nimport String names[];\n",
        DataUtil::MakeVariablesScriptHeader(vars).GetCStr());
    EXPECT_STREQ("bool flag = true;\nexport flag;\nint list[5];\nexport list;\nString names[];\nexport names;\n"
        "function game_start() {\n}\n",
        DataUtil::MakeVariablesScriptBody(vars).GetCStr());
}

TEST(AGFReader, CharacterAGS4Properties)
{
    tinyxml2::XMLDocument doc;
    ASSERT_EQ(tinyxml2::XML_SUCCESS, doc.Parse(
        "<Characters>"
        "<Character><BlendMode>Add</BlendMode><DisplayName>Ego</DisplayName><Enabled>False</Enabled>"
        "<GraphicAnchor>0.25, 2</GraphicAnchor><GraphicOffset>3, -4</GraphicOffset><ID>0</ID>"
        "<Interactions><Event Index=\"0\">cEgo_Look</Event></Interactions><OnAnyClick>cEgo_AnyClick</OnAnyClick>"
        "<ScriptModule>Ego.asc</ScriptModule><ScriptName>cEgo</ScriptName><SpeechColor>0xFFA8A8A8</SpeechColor>"
        "<Visible>False</Visible></Character>"
        "<Character><ID>1</ID><ScriptName>cOther</ScriptName></Character>"
        "</Characters>"));
    AGF::Character parser;
    DataUtil::CharacterData ego;
    parser.ReadAllData(doc.RootElement()->FirstChildElement("Character"), ego);
    EXPECT_STREQ("Ego", ego.RealName.GetCStr());
    EXPECT_FALSE(ego.Enabled);
    EXPECT_FALSE(ego.Visible);
    EXPECT_EQ(Common::kBlend_Add, ego.BlendMode);
    EXPECT_FLOAT_EQ(0.25f, ego.GraphicAnchor.X);
    EXPECT_FLOAT_EQ(1.f, ego.GraphicAnchor.Y); // clamped to 0-1
    EXPECT_EQ(3, ego.GraphicOffset.X);
    EXPECT_EQ(-4, ego.GraphicOffset.Y);
    EXPECT_STREQ("cEgo_AnyClick", ego.OnAnyClick.GetCStr());
    // the colour has all 32 bits, as Convert.ToInt32(s, 16) reads it
    EXPECT_EQ(static_cast<int>(0xFFA8A8A8u), ego.SpeechColor);
    // the character's own script module is the module of its interactions
    EXPECT_STREQ("Ego.asc", ego.ScriptModule.GetCStr());
    ASSERT_EQ(1u, ego.InteractionEvents.size());
    EXPECT_STREQ("cEgo_Look", ego.InteractionEvents[0].GetCStr());

    // the editor's defaults
    DataUtil::CharacterData other;
    parser.ReadAllData(doc.RootElement()->LastChildElement("Character"), other);
    EXPECT_TRUE(other.Enabled);
    EXPECT_TRUE(other.Visible);
    EXPECT_FLOAT_EQ(0.5f, other.GraphicAnchor.X);
    EXPECT_FLOAT_EQ(1.f, other.GraphicAnchor.Y);
    EXPECT_STREQ("GlobalScript.asc", other.ScriptModule.GetCStr());
}

TEST(AGFReader, OtherEntitiesAGS4Properties)
{
    tinyxml2::XMLDocument doc;
    ASSERT_EQ(tinyxml2::XML_SUCCESS, doc.Parse(
        "<Game>"
        "<MouseCursor><CreateEvent>True</CreateEvent><ID>4</ID><Name>Use</Name><StandardRole>UseInv</StandardRole>"
        "</MouseCursor>"
        "<InventoryItem><DisplayName>Key</DisplayName><HotspotAlignment>BottomRight</HotspotAlignment><ID>1</ID>"
        "<OnAnyClick>iKey_AnyClick</OnAnyClick><ScriptName>iKey</ScriptName></InventoryItem>"
        "<View><ID>1</ID><Loops><Loop><Frames>"
        "<ViewFrame><Delay>2</Delay><EventName>step</EventName><Flip>Both</Flip><Image>-1</Image>"
        "<XOffset>2</XOffset><YOffset>-3</YOffset></ViewFrame>"
        "</Frames></Loop></Loops><ScriptName>VEGO</ScriptName></View>"
        "<Sprites><Sprite Slot=\"1\"><Source><ImportColorDepth>Indexed8Bit</ImportColorDepth></Source></Sprite>"
        "<Sprite Slot=\"2\"><Source><ImportColorDepth>GameDefault</ImportColorDepth></Source></Sprite></Sprites>"
        "<Font><FontFileName>Fonts/Arial.ttf</FontFileName><ID>3</ID><Name>Arial</Name></Font>"
        "<FontFiles><FontFile><FileName>Fonts/Arial.ttf</FileName><SourceFilename>C:/Arial.ttf</SourceFilename>"
        "</FontFile><FontFile><FileName /></FontFile></FontFiles>"
        "</Game>"));
    AGF::DocElem root = doc.RootElement();

    DataUtil::CursorData cursor;
    AGF::Cursor().ReadAllData(root->FirstChildElement("MouseCursor"), cursor);
    EXPECT_TRUE(cursor.CreateEvent);
    EXPECT_EQ(kCursorRole_UseInv, cursor.StandardRole);

    DataUtil::InventoryItemData item;
    AGF::InventoryItem().ReadAllData(root->FirstChildElement("InventoryItem"), item);
    EXPECT_STREQ("Key", item.Description.GetCStr());
    EXPECT_EQ(kAlignBottomRight, item.HotspotAlignment);
    EXPECT_STREQ("iKey_AnyClick", item.OnAnyClick.GetCStr());

    DataUtil::ViewData view;
    AGF::View().ReadAllData(root->FirstChildElement("View"), view);
    ASSERT_EQ(1u, view.Loops.size());
    ASSERT_EQ(1u, view.Loops[0].Frames.size());
    const DataUtil::ViewFrameData &frame = view.Loops[0].Frames[0];
    EXPECT_EQ(0, frame.Image); // never negative
    EXPECT_EQ(Common::kFlip_Both, frame.Flip);
    EXPECT_EQ(2, frame.XOffset);
    EXPECT_EQ(-3, frame.YOffset);
    EXPECT_STREQ("step", frame.EventName.GetCStr());

    std::vector<DataUtil::SpriteData> sprites;
    AGF::ReadSpriteList(sprites, root);
    ASSERT_EQ(2u, sprites.size());
    EXPECT_EQ(8, sprites[0].ImportColorDepth);
    EXPECT_EQ(0, sprites[1].ImportColorDepth);

    DataUtil::FontData font;
    AGF::Font().ReadAllData(root->FirstChildElement("Font"), font);
    EXPECT_STREQ("Fonts/Arial.ttf", font.FontFileName.GetCStr());
    std::vector<Common::String> font_files;
    AGF::ReadFontFileList(font_files, root);
    ASSERT_EQ(1u, font_files.size());
    EXPECT_STREQ("Fonts/Arial.ttf", font_files[0].GetCStr());
}
