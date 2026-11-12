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
#include <memory>
#include <string>
#include <utility>
#include <vector>
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

//-----------------------------------------------------------------------------
// Tests of the readers for each block of a Game.agf (issue #3147).
//
// Each test passes a piece of XML, written the way the Editor writes it, to the
// reader of one kind of entity, and checks the data that comes out. The tests do
// not open a file, so they do not depend on the project format version.
//
// Notes on the XML used here:
// - Colours are written in decimal; the 0xAARRGGBB form that the Editor writes
//   is read in CharacterAGS4Properties.
// - The AGS 4 Editor writes <ScriptName>, <DisplayName> and <Flip> where the
//   AGS 3 Editor wrote <Name>, <Description> or <RealName> and <Flipped>, and
//   keeps the script module of interactions in the entity itself and not in
//   <Interactions>. The readers know both, but not all of them look for both
//   forms; the XML below has the two forms, so that each test says what the
//   data is, and not which way of writing it a reader happens to know.
//-----------------------------------------------------------------------------

namespace
{

class AGFReaderBlocks : public ::testing::Test
{
protected:
    // Parses the XML text; the elements are valid until the next call
    AGF::DocElem Parse(const char *xml)
    {
        _doc.reset(new tinyxml2::XMLDocument());
        EXPECT_EQ(tinyxml2::XML_SUCCESS, _doc->Parse(xml)) << xml;
        return _doc->RootElement();
    }

private:
    std::unique_ptr<tinyxml2::XMLDocument> _doc;
};

// Reads the entity references of a list the way the tools do
std::vector<DataUtil::EntityRef> ReadRefs(AGF::EntityListParser &list_parser, AGF::EntityParser &parser,
    AGF::DocElem root)
{
    std::vector<DataUtil::EntityRef> refs;
    AGF::ReadAllEntityRefs(refs, list_parser, parser, root);
    return refs;
}

// A game object may keep its custom properties and interactions in the same elements
const char *kPropertiesAndInteractions = R"xml(
    <Properties>
      <CustomProperty><Name>mood</Name><Value>happy</Value></CustomProperty>
      <CustomProperty><Name>level</Name><Value>7</Value></CustomProperty>
    </Properties>
    <ScriptModule>GlobalScript.asc</ScriptModule>
    <Interactions Version="4.0.0.24">
      <ScriptModule>GlobalScript.asc</ScriptModule>
      <Event Index="1" Name="Look_at" UID="Curs0002">on_look</Event>
      <Event Index="3" Name="Talk_to" UID="Curs0004">on_talk</Event>
    </Interactions>
)xml";

template <typename TData>
void ExpectPropertiesAndInteractions(const TData &data)
{
    ASSERT_EQ(2u, data.Properties.size());
    EXPECT_STREQ("mood", data.Properties[0].Name.GetCStr());
    EXPECT_STREQ("happy", data.Properties[0].Value.GetCStr());
    EXPECT_STREQ("level", data.Properties[1].Name.GetCStr());
    EXPECT_STREQ("7", data.Properties[1].Value.GetCStr());
    EXPECT_STREQ("GlobalScript.asc", data.ScriptModule.GetCStr());
    // Events are stored by their index, the unused indexes are empty
    ASSERT_EQ(4u, data.InteractionEvents.size());
    EXPECT_STREQ("", data.InteractionEvents[0].GetCStr());
    EXPECT_STREQ("on_look", data.InteractionEvents[1].GetCStr());
    EXPECT_STREQ("", data.InteractionEvents[2].GetCStr());
    EXPECT_STREQ("on_talk", data.InteractionEvents[3].GetCStr());
}

} // namespace

//
// Game settings and runtime setup
//

TEST_F(AGFReaderBlocks, GameSettings)
{
    AGF::DocElem game = Parse(R"xml(<Game><Settings>
        <AlwaysDisplayTextAsSpeech>True</AlwaysDisplayTextAsSpeech>
        <AndroidAppVersionCode>3</AndroidAppVersionCode>
        <AndroidAppVersionName>2.5.1</AndroidAppVersionName>
        <AndroidApplicationId>org.example.game</AndroidApplicationId>
        <AndroidBuildFormat>Aab</AndroidBuildFormat>
        <AntiAliasFonts>True</AntiAliasFonts>
        <AntiGlideMode>True</AntiGlideMode>
        <AttachDataToExe>True</AttachDataToExe>
        <AudioIndexer>9</AudioIndexer>
        <AutoMoveInWalkMode>True</AutoMoveInWalkMode>
        <BackwardsText>True</BackwardsText>
        <BuildTargets>Windows,Linux</BuildTargets>
        <ClipGUIControls>True</ClipGUIControls>
        <ColorDepth>HighColor</ColorDepth>
        <CompressSpritesType>LZW</CompressSpritesType>
        <CustomDataDir>Extra,More</CustomDataDir>
        <CustomResolution>640x400</CustomResolution>
        <DebugMode>True</DebugMode>
        <DefaultRoomMaskResolution>2</DefaultRoomMaskResolution>
        <Description xml:space="preserve">A test game</Description>
        <DeveloperName>Dev Name</DeveloperName>
        <DeveloperURL>http://example.org</DeveloperURL>
        <DialogOptionsBackwards>True</DialogOptionsBackwards>
        <DialogOptionsBullet>11</DialogOptionsBullet>
        <DialogOptionsGUI>3</DialogOptionsGUI>
        <DialogOptionsGap>7</DialogOptionsGap>
        <DialogScriptNarrateFunction>MyNarrate</DialogScriptNarrateFunction>
        <DialogScriptSayFunction>MySay</DialogScriptSayFunction>
        <DisplayMultipleInventory>True</DisplayMultipleInventory>
        <DisplaySingleDialogOption>True</DisplaySingleDialogOption>
        <GUIDAsString>1234-5678</GUIDAsString>
        <GUIHandleOnlyLeftMouseButton>True</GUIHandleOnlyLeftMouseButton>
        <GameFPS>25</GameFPS>
        <GameFileName>testgame</GameFileName>
        <GameName>Test Game</GameName>
        <GameTextEncoding>UTF-8</GameTextEncoding>
        <GameTextLanguage>Italian</GameTextLanguage>
        <Genre>Adventure</Genre>
        <GlobalSpeechAnimationDelay>6</GlobalSpeechAnimationDelay>
        <HandleInvClicksInScript>True</HandleInvClicksInScript>
        <InventoryCursors>True</InventoryCursors>
        <InventoryHotspotMarkerCrosshairColor>255</InventoryHotspotMarkerCrosshairColor>
        <InventoryHotspotMarkerDotColor>16</InventoryHotspotMarkerDotColor>
        <InventoryHotspotMarkerSprite>15</InventoryHotspotMarkerSprite>
        <InventoryHotspotMarkerStyle>Sprite</InventoryHotspotMarkerStyle>
        <NumberDialogOptions>Normal</NumberDialogOptions>
        <OptimizeSpriteStorage>True</OptimizeSpriteStorage>
        <PixelPerfect>True</PixelPerfect>
        <ReleaseDate>2026-01-02</ReleaseDate>
        <RenderAtScreenResolution>True</RenderAtScreenResolution>
        <RoomTransition>Dissolve</RoomTransition>
        <RunGameLoopsWhileDialogOptionsDisplayed>True</RunGameLoopsWhileDialogOptionsDisplayed>
        <SaveGameFileExtension>tst</SaveGameFileExtension>
        <SaveGameFolderName>TestGame</SaveGameFolderName>
        <SaveScreenshots>True</SaveScreenshots>
        <ScaleCharacterSpriteOffsets>True</ScaleCharacterSpriteOffsets>
        <ScriptAPIVersion>v363</ScriptAPIVersion>
        <ScriptCompatLevel>v321</ScriptCompatLevel>
        <SkipSpeech>MouseOnly</SkipSpeech>
        <SpeechPortraitSide>Alternate</SpeechPortraitSide>
        <SpeechStyle>SierraWithBackground</SpeechStyle>
        <SplitResources>100</SplitResources>
        <TextBoxKeyClaimStyle>TextOnly</TextBoxKeyClaimStyle>
        <TextWindowGUI>2</TextWindowGUI>
        <ThoughtGUI>4</ThoughtGUI>
        <TurnBeforeFacing>True</TurnBeforeFacing>
        <TurnBeforeWalking>True</TurnBeforeWalking>
        <TurnOrderPriority>Random</TurnOrderPriority>
        <UniqueID>123456</UniqueID>
        <UseGlobalSpeechAnimationDelay>True</UseGlobalSpeechAnimationDelay>
        <Version>1.2.3</Version>
        <WalkInLookMode>True</WalkInLookMode>
        <WhenInterfaceDisabled>GoBlack</WhenInterfaceDisabled>
    </Settings></Game>)xml");
    ASSERT_TRUE(game != nullptr);
    DataUtil::GameSettings s;
    AGF::ReadGameSettings(s, game);

    EXPECT_TRUE(s.AlwaysDisplayTextAsSpeech);
    EXPECT_EQ(3, s.AndroidAppVersionCode);
    EXPECT_STREQ("2.5.1", s.AndroidAppVersionName.GetCStr());
    EXPECT_STREQ("org.example.game", s.AndroidApplicationId.GetCStr());
    EXPECT_EQ(DataUtil::kAndroidBuild_Aab, s.AndroidBuildFormat);
    EXPECT_TRUE(s.AntiAliasFonts);
    EXPECT_TRUE(s.AntiGlideMode);
    EXPECT_TRUE(s.AttachDataToExe);
    EXPECT_EQ(9, s.AudioIndexer);
    EXPECT_TRUE(s.AutoMoveInWalkMode);
    EXPECT_TRUE(s.BackwardsText);
    EXPECT_STREQ("Windows,Linux", s.BuildTargets.GetCStr());
    EXPECT_TRUE(s.ClipGUIControls);
    EXPECT_EQ(DataUtil::kGameColorDepth_HighColor, s.ColorDepth);
    EXPECT_EQ(Common::kSprCompress_LZW, s.CompressSpritesType);
    EXPECT_STREQ("Extra,More", s.CustomDataDir.GetCStr());
    EXPECT_STREQ("640x400", s.CustomResolution.GetCStr());
    EXPECT_TRUE(s.DebugMode);
    EXPECT_EQ(2, s.DefaultRoomMaskResolution);
    EXPECT_STREQ("A test game", s.Description.GetCStr());
    EXPECT_STREQ("Dev Name", s.DeveloperName.GetCStr());
    EXPECT_STREQ("http://example.org", s.DeveloperURL.GetCStr());
    EXPECT_TRUE(s.DialogOptionsBackwards);
    EXPECT_EQ(11, s.DialogOptionsBullet);
    EXPECT_EQ(3, s.DialogOptionsGUI);
    EXPECT_EQ(7, s.DialogOptionsGap);
    EXPECT_STREQ("MyNarrate", s.DialogScriptNarrateFunction.GetCStr());
    EXPECT_STREQ("MySay", s.DialogScriptSayFunction.GetCStr());
    EXPECT_TRUE(s.DisplayMultipleInventory);
    EXPECT_TRUE(s.DisplaySingleDialogOption);
    EXPECT_STREQ("1234-5678", s.GUIDAsString.GetCStr());
    EXPECT_TRUE(s.GUIHandleOnlyLeftMouseButton);
    EXPECT_EQ(25, s.GameFPS);
    EXPECT_STREQ("testgame", s.GameFileName.GetCStr());
    EXPECT_STREQ("Test Game", s.GameName.GetCStr());
    EXPECT_STREQ("UTF-8", s.GameTextEncoding.GetCStr());
    EXPECT_STREQ("Italian", s.GameTextLanguage.GetCStr());
    EXPECT_STREQ("Adventure", s.Genre.GetCStr());
    EXPECT_EQ(6, s.GlobalSpeechAnimationDelay);
    EXPECT_TRUE(s.HandleInvClicksInScript);
    EXPECT_TRUE(s.InventoryCursors);
    EXPECT_EQ(255u, s.InventoryHotspotMarkerCrosshairColor);
    EXPECT_EQ(16u, s.InventoryHotspotMarkerDotColor);
    EXPECT_EQ(15, s.InventoryHotspotMarkerSprite);
    EXPECT_EQ(DataUtil::kInventoryHotspot_Sprite, s.InventoryHotspotMarkerStyle);
    EXPECT_EQ(kDlgOptNumbering, s.NumberDialogOptions);
    EXPECT_TRUE(s.OptimizeSpriteStorage);
    EXPECT_TRUE(s.PixelPerfect);
    EXPECT_STREQ("2026-01-02", s.ReleaseDate.GetCStr());
    EXPECT_EQ(kRenderAtScreenRes_Enabled, s.RenderAtScreenResolution);
    EXPECT_EQ(kScrTran_Dissolve, s.RoomTransition);
    EXPECT_TRUE(s.RunGameLoopsWhileDialogOptionsDisplayed);
    EXPECT_STREQ("tst", s.SaveGameFileExtension.GetCStr());
    EXPECT_STREQ("TestGame", s.SaveGameFolderName.GetCStr());
    EXPECT_TRUE(s.SaveScreenshots);
    EXPECT_TRUE(s.ScaleCharacterSpriteOffsets);
    EXPECT_STREQ("v363", s.ScriptAPIVersion.GetCStr());
    EXPECT_STREQ("v321", s.ScriptCompatLevel.GetCStr());
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOrTimer, s.SkipSpeech);
    EXPECT_EQ(DataUtil::kSpeechPortrait_Alternate, s.SpeechPortraitSide);
    EXPECT_EQ(DataUtil::kSpeechStyle_SierraWithBackground, s.SpeechStyle);
    EXPECT_STREQ("100", s.SplitResources.GetCStr());
    EXPECT_EQ(DataUtil::kTextBoxKeyClaim_TextOnly, s.TextBoxKeyClaimStyle);
    EXPECT_EQ(2, s.TextWindowGUI);
    EXPECT_EQ(4, s.ThoughtGUI);
    EXPECT_TRUE(s.TurnBeforeFacing);
    EXPECT_TRUE(s.TurnBeforeWalking);
    EXPECT_EQ(DataUtil::kTurnOrder_Random, s.TurnOrderPriority);
    EXPECT_EQ(123456, s.UniqueID);
    EXPECT_TRUE(s.UseGlobalSpeechAnimationDelay);
    EXPECT_STREQ("1.2.3", s.Version.GetCStr());
    EXPECT_TRUE(s.WalkInLookMode);
    EXPECT_EQ(Common::kGuiDis_Blackout, s.WhenInterfaceDisabled);
}

TEST_F(AGFReaderBlocks, GameSettingsDefaults)
{
    // Absent elements give the defaults of the Editor: this is what a project of an older Editor gets
    AGF::DocElem game = Parse("<Game><Settings /></Game>");
    ASSERT_TRUE(game != nullptr);
    DataUtil::GameSettings s;
    AGF::ReadGameSettings(s, game);

    // (DebugMode and NumberDialogOptions are left out: their defaults are not the same in all readers)
    EXPECT_FALSE(s.PixelPerfect);
    EXPECT_STREQ("", s.GameName.GetCStr());
    EXPECT_EQ(40, s.GameFPS);
    EXPECT_EQ(DataUtil::kAndroidBuild_ApkEmbedded, s.AndroidBuildFormat);
    EXPECT_EQ(DataUtil::kGameColorDepth_TrueColor, s.ColorDepth);
    EXPECT_EQ(Common::kSprCompress_None, s.CompressSpritesType);
    EXPECT_EQ(DataUtil::kInventoryHotspot_None, s.InventoryHotspotMarkerStyle);
    EXPECT_EQ(kRenderAtScreenRes_UserDefined, s.RenderAtScreenResolution);
    EXPECT_EQ(kScrTran_Fade, s.RoomTransition);
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOrKeyboardOrTimer, s.SkipSpeech);
    EXPECT_EQ(DataUtil::kSpeechPortrait_Left, s.SpeechPortraitSide);
    EXPECT_EQ(DataUtil::kSpeechStyle_Lucasarts, s.SpeechStyle);
    EXPECT_EQ(DataUtil::kTextBoxKeyClaim_All, s.TextBoxKeyClaimStyle);
    EXPECT_EQ(DataUtil::kTurnOrder_Clockwise, s.TurnOrderPriority);
    EXPECT_EQ(Common::kGuiDis_Greyout, s.WhenInterfaceDisabled);
}

TEST_F(AGFReaderBlocks, GameSettingsUnknownEnumNamesGiveTheDefault)
{
    AGF::DocElem game = Parse(R"xml(<Game><Settings>
        <ColorDepth>Lots</ColorDepth><RoomTransition>Spin</RoomTransition>
        <SpeechStyle>Mime</SpeechStyle><WhenInterfaceDisabled>Explode</WhenInterfaceDisabled>
        <GameFPS>fast</GameFPS>
    </Settings></Game>)xml");
    ASSERT_TRUE(game != nullptr);
    DataUtil::GameSettings s;
    AGF::ReadGameSettings(s, game);
    EXPECT_EQ(DataUtil::kGameColorDepth_TrueColor, s.ColorDepth);
    EXPECT_EQ(kScrTran_Fade, s.RoomTransition);
    EXPECT_EQ(DataUtil::kSpeechStyle_Lucasarts, s.SpeechStyle);
    EXPECT_EQ(Common::kGuiDis_Greyout, s.WhenInterfaceDisabled);
    EXPECT_EQ(40, s.GameFPS);
}

TEST_F(AGFReaderBlocks, CustomDataDirectories)
{
    AGF::DocElem game = Parse("<Game><Settings><CustomDataDir>Extra,,More</CustomDataDir></Settings></Game>");
    ASSERT_TRUE(game != nullptr);
    std::vector<AGS::Common::String> dirs;
    AGF::ReadCustomDataDirectories(dirs, game);
    ASSERT_EQ(2u, dirs.size());
    EXPECT_STREQ("Extra", dirs[0].GetCStr());
    EXPECT_STREQ("More", dirs[1].GetCStr());
}

TEST_F(AGFReaderBlocks, RuntimeSetup)
{
    AGF::DocElem game = Parse(R"xml(<Game><RuntimeSetup>
        <AAScaledSprites>True</AAScaledSprites>
        <AutoLockMouse>True</AutoLockMouse>
        <CompressSaves>True</CompressSaves>
        <CustomAppDataPath>appdata</CustomAppDataPath>
        <CustomSavePath>saves</CustomSavePath>
        <DigitalSound>wasapi</DigitalSound>
        <FullscreenDesktop>True</FullscreenDesktop>
        <FullscreenGameScaling>proportional</FullscreenGameScaling>
        <GameScaling>integer</GameScaling>
        <GameScalingMultiplier>3</GameScalingMultiplier>
        <GraphicsDriver>D3D9</GraphicsDriver>
        <GraphicsFilter>Linear</GraphicsFilter>
        <MouseSpeed>1.5</MouseSpeed>
        <RenderAtScreenResolution>True</RenderAtScreenResolution>
        <Rotation>unlocked</Rotation>
        <ShowFPS>True</ShowFPS>
        <SoundCacheSize>16</SoundCacheSize>
        <SpriteCacheSize>128</SpriteCacheSize>
        <TextureCacheSize>64</TextureCacheSize>
        <TitleText>The Title</TitleText>
        <TouchToMouseEmulation>one_finger</TouchToMouseEmulation>
        <TouchToMouseMotionMode>relative</TouchToMouseMotionMode>
        <Translation>Italiano</Translation>
        <UseCustomAppDataPath>True</UseCustomAppDataPath>
        <UseCustomSavePath>True</UseCustomSavePath>
        <UseVoicePack>True</UseVoicePack>
        <VSync>True</VSync>
        <Windowed>True</Windowed>
    </RuntimeSetup></Game>)xml");
    ASSERT_TRUE(game != nullptr);
    DataUtil::RuntimeSetup rs;
    AGF::ReadRuntimeSetup(rs, game);

    EXPECT_TRUE(rs.AAScaledSprites);
    EXPECT_TRUE(rs.AutoLockMouse);
    EXPECT_TRUE(rs.CompressSaves);
    EXPECT_STREQ("appdata", rs.CustomAppDataPath.GetCStr());
    EXPECT_STREQ("saves", rs.CustomSavePath.GetCStr());
    EXPECT_STREQ("wasapi", rs.AudioDriver.GetCStr()); // <DigitalSound>
    EXPECT_TRUE(rs.FullscreenDesktop);
    EXPECT_STREQ("proportional", rs.FullscreenGameScaling.GetCStr());
    EXPECT_STREQ("integer", rs.WindowGameScaling.GetCStr()); // <GameScaling>
    EXPECT_EQ(3, rs.GameScalingMultiplier);
    EXPECT_STREQ("D3D9", rs.GraphicsDriver.GetCStr());
    EXPECT_STREQ("Linear", rs.GraphicsFilter.GetCStr());
    EXPECT_FLOAT_EQ(1.5f, rs.MouseSpeed);
    EXPECT_TRUE(rs.RenderAtScreenResolution);
    EXPECT_STREQ("unlocked", rs.Rotation.GetCStr());
    EXPECT_TRUE(rs.ShowFPS);
    EXPECT_EQ(16, rs.SoundCacheSize);
    EXPECT_EQ(128, rs.SpriteCacheSize);
    EXPECT_EQ(64, rs.TextureCacheSize);
    EXPECT_STREQ("The Title", rs.TitleText.GetCStr());
    EXPECT_STREQ("one_finger", rs.TouchToMouseEmulation.GetCStr());
    EXPECT_STREQ("relative", rs.TouchToMouseMotionMode.GetCStr());
    EXPECT_STREQ("Italiano", rs.Translation.GetCStr());
    EXPECT_TRUE(rs.UseCustomAppDataPath);
    EXPECT_TRUE(rs.UseCustomSavePath);
    EXPECT_TRUE(rs.UseVoicePack);
    EXPECT_TRUE(rs.VSync);
    EXPECT_TRUE(rs.Windowed);
}

//
// Characters, inventory items, cursors
//

TEST_F(AGFReaderBlocks, Character)
{
    const std::string xml = std::string(R"xml(<Character>
        <AdjustSpeedWithScaling>True</AdjustSpeedWithScaling>
        <AdjustVolumeWithScaling>True</AdjustVolumeWithScaling>
        <AnimationDelay>6</AnimationDelay>
        <Baseline>12</Baseline>
        <BlinkingView>5</BlinkingView>
        <BlockingRectangle>1,2,30,40</BlockingRectangle>
        <Clickable>False</Clickable>
        <DiagonalLoops>False</DiagonalLoops>
        <DisplayName>Mr. Test</DisplayName>
        <RealName>Mr. Test</RealName>
        <ID>3</ID>
        <IdleAnimationDelay>9</IdleAnimationDelay>
        <IdleDelay>21</IdleDelay>
        <IdleView>7</IdleView>
        <MovementLinkedToAnimation>False</MovementLinkedToAnimation>
        <MovementSpeed>4</MovementSpeed>
        <MovementSpeedX>5</MovementSpeedX>
        <MovementSpeedY>6</MovementSpeedY>
        <NormalView>2</NormalView>
        <ScriptName>cTest</ScriptName>
        <Solid>False</Solid>
        <SpeechAnimationDelay>8</SpeechAnimationDelay>
        <SpeechColor>13</SpeechColor>
        <SpeechView>10</SpeechView>
        <StartX>100</StartX>
        <StartY>150</StartY>
        <StartingRoom>4</StartingRoom>
        <ThinkingView>11</ThinkingView>
        <Transparency>25</Transparency>
        <TurnBeforeWalking>False</TurnBeforeWalking>
        <TurnWhenFacing>False</TurnWhenFacing>
        <UniformMovementSpeed>False</UniformMovementSpeed>
        <UseRoomAreaLighting>False</UseRoomAreaLighting>
        <UseRoomAreaScaling>False</UseRoomAreaScaling>)xml") +
        kPropertiesAndInteractions + "</Character>";
    AGF::DocElem elem = Parse(xml.c_str());
    ASSERT_TRUE(elem != nullptr);

    AGF::Character parser;
    DataUtil::CharacterData c;
    AGF::ReadEntityRef(c, parser, elem);
    parser.ReadAllData(elem, c);

    EXPECT_STREQ("Character", c.TypeName.GetCStr());
    EXPECT_EQ(3, c.ID);
    EXPECT_STREQ("cTest", c.ScriptName.GetCStr());
    EXPECT_TRUE(c.AdjustSpeedWithScaling);
    EXPECT_TRUE(c.AdjustVolumeWithScaling);
    EXPECT_EQ(6, c.AnimationDelay);
    EXPECT_EQ(12, c.Baseline);
    EXPECT_EQ(5, c.BlinkingView);
    EXPECT_EQ(1, c.BlockingX);
    EXPECT_EQ(2, c.BlockingY);
    EXPECT_EQ(30, c.BlockingWidth);
    EXPECT_EQ(40, c.BlockingHeight);
    EXPECT_FALSE(c.Clickable);
    EXPECT_FALSE(c.DiagonalLoops);
    EXPECT_STREQ("Mr. Test", c.RealName.GetCStr());
    EXPECT_EQ(9, c.IdleAnimationDelay);
    EXPECT_EQ(21, c.IdleDelay);
    EXPECT_EQ(7, c.IdleView);
    EXPECT_FALSE(c.MovementLinkedToAnimation);
    EXPECT_EQ(4, c.MovementSpeed);
    EXPECT_EQ(5, c.MovementSpeedX);
    EXPECT_EQ(6, c.MovementSpeedY);
    EXPECT_EQ(2, c.NormalView);
    EXPECT_FALSE(c.Solid);
    EXPECT_EQ(8, c.SpeechAnimationDelay);
    EXPECT_EQ(13, c.SpeechColor);
    EXPECT_EQ(10, c.SpeechView);
    EXPECT_EQ(100, c.StartX);
    EXPECT_EQ(150, c.StartY);
    EXPECT_EQ(4, c.StartingRoom);
    EXPECT_EQ(11, c.ThinkingView);
    EXPECT_EQ(25, c.Transparency);
    EXPECT_FALSE(c.TurnBeforeWalking);
    EXPECT_FALSE(c.TurnWhenFacing);
    EXPECT_FALSE(c.UniformMovementSpeed);
    EXPECT_FALSE(c.UseRoomAreaLighting);
    EXPECT_FALSE(c.UseRoomAreaScaling);
    ExpectPropertiesAndInteractions(c);
}

TEST_F(AGFReaderBlocks, CharacterDefaults)
{
    AGF::DocElem elem = Parse("<Character><ID>0</ID><ScriptName>cEgo</ScriptName></Character>");
    ASSERT_TRUE(elem != nullptr);
    DataUtil::CharacterData c;
    AGF::Character().ReadAllData(elem, c);

    // A new character is clickable, solid, turns before walking, and so on
    EXPECT_TRUE(c.Clickable);
    EXPECT_TRUE(c.DiagonalLoops);
    EXPECT_TRUE(c.Solid);
    EXPECT_TRUE(c.TurnBeforeWalking);
    EXPECT_TRUE(c.TurnWhenFacing);
    EXPECT_TRUE(c.UniformMovementSpeed);
    EXPECT_TRUE(c.UseRoomAreaLighting);
    EXPECT_TRUE(c.UseRoomAreaScaling);
    EXPECT_FALSE(c.AdjustSpeedWithScaling);
    EXPECT_EQ(-1, c.StartingRoom);
    EXPECT_EQ(0, c.BlockingWidth);
    EXPECT_EQ(0u, c.Properties.size());
    EXPECT_EQ(0u, c.InteractionEvents.size());
}

TEST_F(AGFReaderBlocks, InventoryItem)
{
    const std::string xml = std::string(R"xml(<InventoryItem>
        <CursorImage>21</CursorImage>
        <DisplayName>Old key</DisplayName>
        <Description>Old key</Description>
        <HotspotAlignment>MiddleCenter</HotspotAlignment>
        <HotspotX>4</HotspotX>
        <HotspotY>5</HotspotY>
        <ID>2</ID>
        <Image>20</Image>
        <PlayerStartsWithItem>True</PlayerStartsWithItem>
        <ScriptName>iKey</ScriptName>
        <Name>iKey</Name>)xml") +
        kPropertiesAndInteractions + "</InventoryItem>";
    AGF::DocElem elem = Parse(xml.c_str());
    ASSERT_TRUE(elem != nullptr);

    AGF::InventoryItem parser;
    DataUtil::InventoryItemData item;
    AGF::ReadEntityRef(item, parser, elem);
    parser.ReadAllData(elem, item);

    EXPECT_STREQ("InventoryItem", item.TypeName.GetCStr());
    EXPECT_EQ(2, item.ID);
    EXPECT_STREQ("iKey", item.ScriptName.GetCStr());
    EXPECT_STREQ("Old key", item.Description.GetCStr());
    EXPECT_EQ(20, item.Image);
    EXPECT_EQ(21, item.CursorImage);
    EXPECT_EQ(4, item.HotspotX);
    EXPECT_EQ(5, item.HotspotY);
    EXPECT_TRUE(item.PlayerStartsWith);
}

TEST_F(AGFReaderBlocks, InventoryItemDefaults)
{
    AGF::DocElem elem = Parse("<InventoryItem><ID>1</ID></InventoryItem>");
    ASSERT_TRUE(elem != nullptr);
    DataUtil::InventoryItemData item;
    AGF::InventoryItem().ReadAllData(elem, item);
    EXPECT_STREQ("", item.Description.GetCStr());
    EXPECT_EQ(0, item.Image);
    EXPECT_FALSE(item.PlayerStartsWith);
}

TEST_F(AGFReaderBlocks, Cursor)
{
    AGF::DocElem elem = Parse(R"xml(<MouseCursor>
        <Animate>True</Animate>
        <AnimateOnlyOnHotspots>True</AnimateOnlyOnHotspots>
        <AnimateOnlyWhenMoving>True</AnimateOnlyWhenMoving>
        <AnimationDelay>9</AnimationDelay>
        <HotspotX>3</HotspotX>
        <HotspotY>4</HotspotY>
        <ID>2</ID>
        <Image>30</Image>
        <Name xml:space="preserve">Walk to</Name>
        <StandardMode>True</StandardMode>
        <View>8</View>
    </MouseCursor>)xml");
    ASSERT_TRUE(elem != nullptr);

    AGF::Cursor parser;
    DataUtil::CursorData cursor;
    AGF::ReadEntityRef(cursor, parser, elem);
    parser.ReadAllData(elem, cursor);

    EXPECT_EQ(2, cursor.ID);
    // The script name of a cursor is made of its name, with the prefix
    EXPECT_STREQ("eModeWalkto", cursor.ScriptName.GetCStr());
    EXPECT_STREQ("Walk to", cursor.Name.GetCStr());
    EXPECT_EQ(30, cursor.Image);
    EXPECT_EQ(3, cursor.HotspotX);
    EXPECT_EQ(4, cursor.HotspotY);
    EXPECT_TRUE(cursor.Animate);
    EXPECT_EQ(8, cursor.View);
    EXPECT_TRUE(cursor.AnimateOnlyOnHotspots);
    EXPECT_TRUE(cursor.AnimateOnlyWhenMoving);
    EXPECT_TRUE(cursor.StandardMode);
    EXPECT_EQ(9, cursor.AnimationDelay);
}

TEST_F(AGFReaderBlocks, CursorDefaults)
{
    AGF::DocElem elem = Parse("<MouseCursor><ID>0</ID><Name>Look</Name></MouseCursor>");
    ASSERT_TRUE(elem != nullptr);
    DataUtil::CursorData cursor;
    AGF::Cursor().ReadAllData(elem, cursor);
    EXPECT_EQ(-1, cursor.View);
    EXPECT_EQ(5, cursor.AnimationDelay);
    EXPECT_FALSE(cursor.Animate);
    EXPECT_FALSE(cursor.StandardMode);
}

//
// Views, fonts, audio
//

TEST_F(AGFReaderBlocks, View)
{
    AGF::DocElem elem = Parse(R"xml(<View>
        <ID>4</ID>
        <ScriptName>VWALK</ScriptName>
        <Name>VWALK</Name>
        <Loops>
          <Loop>
            <ID>0</ID>
            <RunNextLoop>True</RunNextLoop>
            <Frames>
              <ViewFrame>
                <Delay>3</Delay>
                <EventName />
                <Flip>None</Flip>
                <ID>0</ID>
                <Image>100</Image>
                <Sound>5</Sound>
              </ViewFrame>
              <ViewFrame>
                <Delay>4</Delay>
                <EventName />
                <Flip>Horizontal</Flip>
                <Flipped>True</Flipped>
                <ID>1</ID>
                <Image>101</Image>
                <Sound>-1</Sound>
              </ViewFrame>
            </Frames>
          </Loop>
          <Loop>
            <ID>1</ID>
            <RunNextLoop>False</RunNextLoop>
            <Frames />
          </Loop>
        </Loops>
    </View>)xml");
    ASSERT_TRUE(elem != nullptr);

    AGF::View parser;
    DataUtil::ViewData view;
    AGF::ReadEntityRef(view, parser, elem);
    parser.ReadAllData(elem, view);

    EXPECT_EQ(4, view.ID);
    EXPECT_STREQ("VWALK", view.ScriptName.GetCStr());
    ASSERT_EQ(2u, view.Loops.size());
    EXPECT_TRUE(view.Loops[0].RunNextLoop);
    EXPECT_FALSE(view.Loops[1].RunNextLoop);
    EXPECT_EQ(0u, view.Loops[1].Frames.size());
    ASSERT_EQ(2u, view.Loops[0].Frames.size());
    EXPECT_EQ(3, view.Loops[0].Frames[0].Delay);
    EXPECT_EQ(Common::kFlip_None, view.Loops[0].Frames[0].Flip);
    EXPECT_EQ(100, view.Loops[0].Frames[0].Image);
    EXPECT_EQ(5, view.Loops[0].Frames[0].Sound);
    EXPECT_EQ(4, view.Loops[0].Frames[1].Delay);
    EXPECT_EQ(Common::kFlip_Horizontal, view.Loops[0].Frames[1].Flip);
    EXPECT_EQ(101, view.Loops[0].Frames[1].Image);
    EXPECT_EQ(-1, view.Loops[0].Frames[1].Sound);
}

TEST_F(AGFReaderBlocks, ViewWithoutLoops)
{
    AGF::DocElem elem = Parse("<View><ID>1</ID><ScriptName>VEMPTY</ScriptName><Loops /></View>");
    ASSERT_TRUE(elem != nullptr);
    DataUtil::ViewData view;
    AGF::View().ReadAllData(elem, view);
    EXPECT_EQ(0u, view.Loops.size());
}

TEST_F(AGFReaderBlocks, Font)
{
    AGF::DocElem elem = Parse(R"xml(<Font>
        <AutoOutlineStyle>Rounded</AutoOutlineStyle>
        <AutoOutlineThickness>2</AutoOutlineThickness>
        <CharacterSpacing>1</CharacterSpacing>
        <CustomHeightValue>18</CustomHeightValue>
        <FontFileName>speech.ttf</FontFileName>
        <HeightDefinedBy>CustomValue</HeightDefinedBy>
        <ID>3</ID>
        <LineSpacing>4</LineSpacing>
        <Name xml:space="preserve">Speech font</Name>
        <OutlineFont>2</OutlineFont>
        <OutlineStyle>UseOutlineFont</OutlineStyle>
        <PointSize>14</PointSize>
        <SizeMultiplier>2</SizeMultiplier>
        <VerticalOffset>-1</VerticalOffset>
    </Font>)xml");
    ASSERT_TRUE(elem != nullptr);

    AGF::Font parser;
    DataUtil::FontData font;
    AGF::ReadEntityRef(font, parser, elem);
    parser.ReadAllData(elem, font);

    EXPECT_EQ(3, font.ID);
    EXPECT_STREQ("eFontSpeechfont", font.ScriptName.GetCStr());
    EXPECT_STREQ("Speech font", font.Name.GetCStr());
    EXPECT_EQ(DataUtil::kFontAutoOutline_Rounded, font.AutoOutlineStyle);
    EXPECT_EQ(2, font.AutoOutlineThickness);
    EXPECT_EQ(1, font.CharacterSpacing);
    EXPECT_EQ(18, font.CustomHeightValue);
    EXPECT_EQ(DataUtil::kFontHeight_CustomValue, font.HeightDefinedBy);
    EXPECT_EQ(4, font.LineSpacing);
    EXPECT_EQ(2, font.OutlineFont);
    EXPECT_EQ(DataUtil::kFontOutline_UseOutlineFont, font.OutlineStyle);
    EXPECT_EQ(14, font.PointSize);
    EXPECT_EQ(2, font.SizeMultiplier);
    EXPECT_EQ(-1, font.VerticalOffset);
}

TEST_F(AGFReaderBlocks, FontDefaults)
{
    AGF::DocElem elem = Parse("<Font><ID>0</ID><Name>Font 0</Name></Font>");
    ASSERT_TRUE(elem != nullptr);
    DataUtil::FontData font;
    AGF::Font().ReadAllData(elem, font);
    EXPECT_EQ(DataUtil::kFontAutoOutline_Squared, font.AutoOutlineStyle);
    EXPECT_EQ(DataUtil::kFontHeight_NominalHeight, font.HeightDefinedBy);
    EXPECT_EQ(DataUtil::kFontOutline_None, font.OutlineStyle);
    EXPECT_EQ(1, font.SizeMultiplier);
}

TEST_F(AGFReaderBlocks, AudioClip)
{
    AGF::DocElem elem = Parse(R"xml(<AudioClip>
        <BundlingType>InSeparateVOX</BundlingType>
        <DefaultPriority>VeryHigh</DefaultPriority>
        <DefaultRepeat>True</DefaultRepeat>
        <DefaultVolume>65</DefaultVolume>
        <FileType>MP3</FileType>
        <ID>6</ID>
        <Index>10</Index>
        <Properties />
        <ScriptName>aTheme</ScriptName>
        <SourceFileName>Music/theme.mp3</SourceFileName>
        <Type>2</Type>
    </AudioClip>)xml");
    ASSERT_TRUE(elem != nullptr);

    AGF::AudioClip parser;
    DataUtil::AudioClipData clip;
    AGF::ReadEntityRef(clip, parser, elem);
    parser.ReadAllData(elem, clip);

    EXPECT_EQ(6, clip.ID);
    EXPECT_STREQ("aTheme", clip.ScriptName.GetCStr());
    EXPECT_EQ(10, clip.Index);
    EXPECT_STREQ("Music/theme.mp3", clip.SourceFileName.GetCStr());
    // The file is stored under the name made of the index, and keeps the extension
    EXPECT_STREQ("au00000A.mp3", clip.CacheFileName.GetCStr());
    EXPECT_EQ(DataUtil::kAudioBundling_InSeparateVOX, clip.BundlingType);
    EXPECT_EQ(2, clip.Type);
    EXPECT_EQ(DataUtil::kAudioFile_MP3, clip.FileType);
    EXPECT_TRUE(clip.Repeat);
    EXPECT_EQ(100, clip.Priority); // VeryHigh
    EXPECT_EQ(65, clip.Volume);
}

TEST_F(AGFReaderBlocks, AudioClipTakesDefaultsFromFolder)
{
    // A clip that does not set a property inherits it from the folder it is in.
    // The Editor writes the settings of a folder in a child element of the same name; the same
    // values are also given as the direct children of the folder, which some readers look at.
    AGF::DocElem root = Parse(R"xml(<AudioClipFolder Name="Music">
        <SubFolders />
        <AudioClips>
          <AudioClip>
            <DefaultPriority>Inherit</DefaultPriority>
            <DefaultRepeat>Inherit</DefaultRepeat>
            <DefaultVolume>-1</DefaultVolume>
            <ID>0</ID>
            <ScriptName>aInherits</ScriptName>
          </AudioClip>
          <AudioClip>
            <DefaultPriority>Low</DefaultPriority>
            <DefaultRepeat>False</DefaultRepeat>
            <DefaultVolume>20</DefaultVolume>
            <ID>1</ID>
            <ScriptName>aOwn</ScriptName>
          </AudioClip>
        </AudioClips>
        <AudioClipFolder>
          <DefaultPriority>High</DefaultPriority>
          <DefaultRepeat>True</DefaultRepeat>
          <DefaultVolume>70</DefaultVolume>
        </AudioClipFolder>
        <DefaultPriority>High</DefaultPriority>
        <DefaultRepeat>True</DefaultRepeat>
        <DefaultVolume>70</DefaultVolume>
    </AudioClipFolder>)xml");
    ASSERT_TRUE(root != nullptr);
    AGF::DocElem clips = root->FirstChildElement("AudioClips");
    ASSERT_TRUE(clips != nullptr);

    AGF::AudioClip parser;
    DataUtil::AudioClipData inherits;
    parser.ReadAllData(clips->FirstChildElement("AudioClip"), inherits);
    EXPECT_TRUE(inherits.Repeat);
    EXPECT_EQ(75, inherits.Priority); // High
    EXPECT_EQ(70, inherits.Volume);

    DataUtil::AudioClipData own;
    parser.ReadAllData(clips->FirstChildElement("AudioClip")->NextSiblingElement("AudioClip"), own);
    EXPECT_FALSE(own.Repeat);
    EXPECT_EQ(25, own.Priority); // Low
    EXPECT_EQ(20, own.Volume);
}

TEST_F(AGFReaderBlocks, AudioClipWithoutAnyDefaults)
{
    AGF::DocElem elem = Parse("<AudioClip><ID>0</ID><Index>1</Index><ScriptName>aX</ScriptName></AudioClip>");
    ASSERT_TRUE(elem != nullptr);
    DataUtil::AudioClipData clip;
    AGF::AudioClip().ReadAllData(elem, clip);
    EXPECT_FALSE(clip.Repeat);
    EXPECT_EQ(50, clip.Priority);
    EXPECT_EQ(100, clip.Volume);
}

TEST_F(AGFReaderBlocks, AudioType)
{
    AGF::DocElem elem = Parse(R"xml(<AudioClipType>
        <BackwardsCompatibilityType>False</BackwardsCompatibilityType>
        <CrossfadeClips>SlowishFade</CrossfadeClips>
        <MaxChannels>3</MaxChannels>
        <Name>Ambient</Name>
        <TypeID>4</TypeID>
        <VolumeReductionWhileSpeechPlaying>35</VolumeReductionWhileSpeechPlaying>
    </AudioClipType>)xml");
    ASSERT_TRUE(elem != nullptr);

    AGF::AudioType parser;
    DataUtil::AudioTypeData type;
    AGF::ReadEntityRef(type, parser, elem);
    parser.ReadAllData(elem, type);

    EXPECT_EQ(4, type.ID);
    EXPECT_STREQ("eAudioTypeAmbient", type.ScriptName.GetCStr());
    EXPECT_STREQ("Ambient", type.Name.GetCStr());
    EXPECT_EQ(3, type.MaxChannels);
    EXPECT_EQ(35, type.VolumeReductionWhileSpeechPlaying);
    EXPECT_EQ(DataUtil::kCrossfade_Slowish, type.Crossfade);
}

//
// GUIs and their controls
//

TEST_F(AGFReaderBlocks, GUI)
{
    AGF::DocElem elem = Parse(R"xml(<GUIMain>
        <NormalGUI>
          <BackgroundColor>5</BackgroundColor>
          <BackgroundImage>12</BackgroundImage>
          <BorderColor>6</BorderColor>
          <Clickable>False</Clickable>
          <Height>180</Height>
          <ID>2</ID>
          <Left>10</Left>
          <Name>gInventory</Name>
          <OnClick>gInventory_OnClick</OnClick>
          <PopupStyle>MouseYPos</PopupStyle>
          <PopupYPos>30</PopupYPos>
          <ScriptModule>GlobalScript.asc</ScriptModule>
          <ScriptName>gInventory</ScriptName>
          <Top>20</Top>
          <Transparency>40</Transparency>
          <Visible>True</Visible>
          <Width>300</Width>
          <ZOrder>7</ZOrder>
        </NormalGUI>
        <Controls />
    </GUIMain>)xml");
    ASSERT_TRUE(elem != nullptr);

    AGF::GUIMain parser;
    DataUtil::GUIData gui;
    AGF::ReadEntityRef(gui, parser, elem);
    parser.ReadAllData(elem, gui);

    EXPECT_STREQ("GUI", gui.TypeName.GetCStr());
    EXPECT_EQ(2, gui.ID);
    EXPECT_STREQ("gInventory", gui.ScriptName.GetCStr());
    EXPECT_FALSE(gui.IsTextWindow);
    EXPECT_EQ(5, gui.BackgroundColor);
    EXPECT_EQ(12, gui.BackgroundImage);
    EXPECT_EQ(6, gui.BorderColor);
    EXPECT_FALSE(gui.Clickable);
    EXPECT_EQ(180, gui.Height);
    EXPECT_EQ(10, gui.Left);
    EXPECT_STREQ("gInventory_OnClick", gui.OnClick.GetCStr());
    EXPECT_EQ(DataUtil::kGUIPopupStyle_MouseYPos, gui.PopupStyle);
    EXPECT_EQ(30, gui.PopupYPos);
    EXPECT_STREQ("GlobalScript.asc", gui.ScriptModule.GetCStr());
    EXPECT_EQ(20, gui.Top);
    EXPECT_EQ(40, gui.Transparency);
    EXPECT_EQ(1, gui.Visible);
    EXPECT_EQ(300, gui.Width);
    EXPECT_EQ(7, gui.ZOrder);
}

TEST_F(AGFReaderBlocks, TextWindowGUI)
{
    AGF::DocElem elem = Parse(R"xml(<GUIMain>
        <TextWindowGUI>
          <BackgroundColor>5</BackgroundColor>
          <BackgroundImage>12</BackgroundImage>
          <ID>3</ID>
          <Name>gTextWindow</Name>
          <Padding>8</Padding>
          <ScriptName>gTextWindow</ScriptName>
          <TextColor>14</TextColor>
        </TextWindowGUI>
        <Controls />
    </GUIMain>)xml");
    ASSERT_TRUE(elem != nullptr);

    AGF::GUIMain parser;
    DataUtil::GUIData gui;
    AGF::ReadEntityRef(gui, parser, elem);
    parser.ReadAllData(elem, gui);

    EXPECT_EQ(3, gui.ID);
    EXPECT_STREQ("gTextWindow", gui.ScriptName.GetCStr());
    EXPECT_TRUE(gui.IsTextWindow);
    EXPECT_EQ(5, gui.BackgroundColor);
    EXPECT_EQ(12, gui.BackgroundImage);
    EXPECT_EQ(8, gui.Padding);
    EXPECT_EQ(14, gui.TextColor);
}

namespace
{

// A control element with the properties that all the controls have, and the given specific ones
std::string Control(const char *tag, const char *name, const char *specific)
{
    return std::string("<") + tag + R"xml(>
        <BackgroundColor>7</BackgroundColor>
        <BorderColor>8</BorderColor>
        <BorderWidth>2</BorderWidth>
        <Clickable>False</Clickable>
        <Enabled>False</Enabled>
        <Height>40</Height>
        <ID>6</ID>
        <Left>10</Left>
        <PaddingX>4</PaddingX>
        <PaddingY>5</PaddingY>
        <ShowBorder>True</ShowBorder>
        <SolidBackground>True</SolidBackground>
        <Top>20</Top>
        <Translated>False</Translated>
        <Visible>False</Visible>
        <Width>30</Width>
        <ZOrder>3</ZOrder>
        <ScriptName>)xml" + name + "</ScriptName><Name>" + name + "</Name>" + specific + "</" + tag + ">";
}

void ExpectCommonControlData(const DataUtil::GUIControlData &data, const char *name)
{
    EXPECT_EQ(6, data.ID);
    EXPECT_STREQ(name, data.ScriptName.GetCStr());
    EXPECT_EQ(7, data.BackgroundColor);
    EXPECT_EQ(8, data.BorderColor);
    EXPECT_EQ(2, data.BorderWidth);
    EXPECT_FALSE(data.Clickable);
    EXPECT_FALSE(data.Enabled);
    EXPECT_EQ(40, data.Height);
    EXPECT_EQ(10, data.Left);
    EXPECT_EQ(4, data.PaddingX);
    EXPECT_EQ(5, data.PaddingY);
    EXPECT_TRUE(data.ShowBorder);
    EXPECT_TRUE(data.SolidBackground);
    EXPECT_EQ(20, data.Top);
    EXPECT_FALSE(data.Translated);
    EXPECT_FALSE(data.Visible);
    EXPECT_EQ(30, data.Width);
    EXPECT_EQ(3, data.ZOrder);
}

} // namespace

TEST_F(AGFReaderBlocks, GUIButton)
{
    const std::string xml = Control("GUIButton", "btnOk", R"xml(
        <BorderShadeColor>9</BorderShadeColor>
        <ClickAction>RunScript</ClickAction>
        <ClipImage>True</ClipImage>
        <ColorStyle>DynamicFlat</ColorStyle>
        <Font>2</Font>
        <Image>50</Image>
        <MouseOverBackgroundColor>10</MouseOverBackgroundColor>
        <MouseOverBorderColor>11</MouseOverBorderColor>
        <MouseOverTextColor>12</MouseOverTextColor>
        <MouseoverImage>51</MouseoverImage>
        <NewModeNumber>4</NewModeNumber>
        <OnClick>btnOk_OnClick</OnClick>
        <PushedBackgroundColor>13</PushedBackgroundColor>
        <PushedBorderColor>14</PushedBorderColor>
        <PushedImage>52</PushedImage>
        <PushedTextColor>15</PushedTextColor>
        <Text>OK</Text>
        <TextAlignment>MiddleRight</TextAlignment>
        <TextColor>16</TextColor>
        <TextOutlineColor>17</TextOutlineColor>
        <WrapText>True</WrapText>)xml");
    AGF::DocElem elem = Parse(xml.c_str());
    ASSERT_TRUE(elem != nullptr);

    AGF::GUIControl parser;
    EXPECT_STREQ("Button", parser.ReadType(elem).GetCStr());
    DataUtil::GUIButtonData btn;
    AGF::ReadEntityRef(btn, parser, elem);
    parser.ReadAllData(elem, btn);
    parser.ReadButtonData(elem, btn);

    ExpectCommonControlData(btn, "btnOk");
    EXPECT_STREQ("RunScript", btn.ClickAction.GetCStr());
    EXPECT_TRUE(btn.ClipImage);
    EXPECT_TRUE(btn.WrapText);
    EXPECT_EQ(2, btn.Font);
    EXPECT_EQ(50, btn.Image);
    EXPECT_EQ(51, btn.MouseoverImage);
    EXPECT_EQ(52, btn.PushedImage);
    EXPECT_EQ(4, btn.NewModeNumber);
    EXPECT_STREQ("btnOk_OnClick", btn.OnClick.GetCStr());
    EXPECT_STREQ("OK", btn.Text.GetCStr());
    EXPECT_EQ(kAlignMiddleRight, btn.TextAlignment);
    EXPECT_EQ(16, btn.TextColor);
    EXPECT_EQ(DataUtil::kButtonColor_DynamicFlat, btn.ColorStyle);
    EXPECT_EQ(9, btn.BorderShadeColor);
    EXPECT_EQ(10, btn.MouseOverBackgroundColor);
    EXPECT_EQ(13, btn.PushedBackgroundColor);
    EXPECT_EQ(11, btn.MouseOverBorderColor);
    EXPECT_EQ(14, btn.PushedBorderColor);
    EXPECT_EQ(12, btn.MouseOverTextColor);
    EXPECT_EQ(15, btn.PushedTextColor);
    EXPECT_EQ(17, btn.TextOutlineColor);
}

TEST_F(AGFReaderBlocks, GUITextWindowEdgeIsAButton)
{
    AGF::DocElem elem = Parse("<GUITextWindowEdge><ID>1</ID><ScriptName>Edge1</ScriptName><Image>9</Image>"
                              "</GUITextWindowEdge>");
    ASSERT_TRUE(elem != nullptr);
    EXPECT_STREQ("Button", AGF::GUIControl().ReadType(elem).GetCStr());
}

TEST_F(AGFReaderBlocks, GUILabel)
{
    const std::string xml = Control("GUILabel", "lblTitle", R"xml(
        <Font>3</Font>
        <Text>Hello [world</Text>
        <TextAlignment>BottomLeft</TextAlignment>
        <TextColor>18</TextColor>
        <TextOutlineColor>19</TextOutlineColor>)xml");
    AGF::DocElem elem = Parse(xml.c_str());
    ASSERT_TRUE(elem != nullptr);

    AGF::GUIControl parser;
    EXPECT_STREQ("Label", parser.ReadType(elem).GetCStr());
    DataUtil::GUILabelData label;
    AGF::ReadEntityRef(label, parser, elem);
    parser.ReadAllData(elem, label);
    parser.ReadLabelData(elem, label);

    ExpectCommonControlData(label, "lblTitle");
    EXPECT_EQ(3, label.Font);
    EXPECT_STREQ("Hello [world", label.Text.GetCStr());
    EXPECT_EQ(kAlignBottomLeft, label.TextAlignment);
    EXPECT_EQ(18, label.TextColor);
    EXPECT_EQ(19, label.TextOutlineColor);
}

TEST_F(AGFReaderBlocks, GUISlider)
{
    const std::string xml = Control("GUISlider", "sldVolume", R"xml(
        <BackgroundImage>60</BackgroundImage>
        <BorderShadeColor>9</BorderShadeColor>
        <HandleColor>20</HandleColor>
        <HandleImage>61</HandleImage>
        <HandleOffset>3</HandleOffset>
        <MaxValue>200</MaxValue>
        <MinValue>-10</MinValue>
        <OnChange>sldVolume_OnChange</OnChange>
        <Value>50</Value>)xml");
    AGF::DocElem elem = Parse(xml.c_str());
    ASSERT_TRUE(elem != nullptr);

    AGF::GUIControl parser;
    EXPECT_STREQ("Slider", parser.ReadType(elem).GetCStr());
    DataUtil::GUISliderData slider;
    AGF::ReadEntityRef(slider, parser, elem);
    parser.ReadAllData(elem, slider);
    parser.ReadSliderData(elem, slider);

    ExpectCommonControlData(slider, "sldVolume");
    EXPECT_EQ(60, slider.BackgroundImage);
    EXPECT_EQ(9, slider.BorderShadeColor);
    EXPECT_EQ(20, slider.HandleColor);
    EXPECT_EQ(61, slider.HandleImage);
    EXPECT_EQ(3, slider.HandleOffset);
    EXPECT_EQ(200, slider.MaxValue);
    EXPECT_EQ(-10, slider.MinValue);
    EXPECT_STREQ("sldVolume_OnChange", slider.OnChange.GetCStr());
    EXPECT_EQ(50, slider.Value);
}

TEST_F(AGFReaderBlocks, GUIInventoryWindow)
{
    const std::string xml = Control("GUIInventory", "invMain", R"xml(
        <CharacterID>2</CharacterID>
        <ItemHeight>22</ItemHeight>
        <ItemWidth>33</ItemWidth>)xml");
    AGF::DocElem elem = Parse(xml.c_str());
    ASSERT_TRUE(elem != nullptr);

    AGF::GUIControl parser;
    EXPECT_STREQ("InvWindow", parser.ReadType(elem).GetCStr());
    DataUtil::GUIInventoryData inv;
    AGF::ReadEntityRef(inv, parser, elem);
    parser.ReadAllData(elem, inv);
    parser.ReadInventoryData(elem, inv);

    ExpectCommonControlData(inv, "invMain");
    EXPECT_EQ(2, inv.CharacterID);
    EXPECT_EQ(22, inv.ItemHeight);
    EXPECT_EQ(33, inv.ItemWidth);
}

TEST_F(AGFReaderBlocks, GUITextBox)
{
    const std::string xml = Control("GUITextBox", "txtName", R"xml(
        <Font>1</Font>
        <OnActivate>txtName_Activate</OnActivate>
        <Text>Player</Text>
        <TextAlignment>TopRight</TextAlignment>
        <TextColor>21</TextColor>
        <TextOutlineColor>22</TextOutlineColor>)xml");
    AGF::DocElem elem = Parse(xml.c_str());
    ASSERT_TRUE(elem != nullptr);

    AGF::GUIControl parser;
    EXPECT_STREQ("TextBox", parser.ReadType(elem).GetCStr());
    DataUtil::GUITextBoxData txt;
    AGF::ReadEntityRef(txt, parser, elem);
    parser.ReadAllData(elem, txt);
    parser.ReadTextBoxData(elem, txt);

    ExpectCommonControlData(txt, "txtName");
    EXPECT_EQ(1, txt.Font);
    EXPECT_STREQ("txtName_Activate", txt.OnActivate.GetCStr());
    EXPECT_STREQ("Player", txt.Text.GetCStr());
    EXPECT_EQ(kAlignTopRight, txt.TextAlignment);
    EXPECT_EQ(21, txt.TextColor);
    EXPECT_EQ(22, txt.TextOutlineColor);
}

TEST_F(AGFReaderBlocks, GUIListBox)
{
    const std::string xml = Control("GUIListBox", "lstSaves", R"xml(
        <Font>1</Font>
        <OnSelectionChanged>lstSaves_Changed</OnSelectionChanged>
        <SelectedBackgroundColor>23</SelectedBackgroundColor>
        <SelectedTextColor>24</SelectedTextColor>
        <ShowScrollArrows>True</ShowScrollArrows>
        <TextAlignment>TopCenter</TextAlignment>
        <TextColor>25</TextColor>
        <TextOutlineColor>26</TextOutlineColor>)xml");
    AGF::DocElem elem = Parse(xml.c_str());
    ASSERT_TRUE(elem != nullptr);

    AGF::GUIControl parser;
    EXPECT_STREQ("ListBox", parser.ReadType(elem).GetCStr());
    DataUtil::GUIListBoxData list;
    AGF::ReadEntityRef(list, parser, elem);
    parser.ReadAllData(elem, list);
    parser.ReadListBoxData(elem, list);

    ExpectCommonControlData(list, "lstSaves");
    EXPECT_EQ(1, list.Font);
    EXPECT_STREQ("lstSaves_Changed", list.OnSelectionChanged.GetCStr());
    EXPECT_EQ(23, list.SelectedBackgroundColor);
    EXPECT_EQ(24, list.SelectedTextColor);
    EXPECT_TRUE(list.ShowScrollArrows);
    EXPECT_EQ(kHAlignCenter, list.TextAlignment);
    EXPECT_EQ(25, list.TextColor);
    EXPECT_EQ(26, list.TextOutlineColor);
}

TEST_F(AGFReaderBlocks, GUIControlDefaults)
{
    AGF::DocElem elem = Parse("<GUILabel><ID>0</ID><ScriptName>Label1</ScriptName></GUILabel>");
    ASSERT_TRUE(elem != nullptr);
    AGF::GUIControl parser;
    DataUtil::GUILabelData label;
    parser.ReadAllData(elem, label);
    parser.ReadLabelData(elem, label);
    // Controls are translated by default; labels and text boxes align to the left
    EXPECT_TRUE(label.Translated);
    EXPECT_EQ(kAlignTopLeft, label.TextAlignment);
}

TEST_F(AGFReaderBlocks, GUIControlTextAlignmentNames)
{
    // The readers accept both the current and the older names of alignments
    struct AlignmentCase
    {
        const char *Name;
        FrameAlignment Expected;
    };
    const AlignmentCase cases[] = {
        {"TopLeft", kAlignTopLeft},
        {"TopCenter", kAlignTopCenter},
        {"TopRight", kAlignTopRight},
        {"MiddleLeft", kAlignMiddleLeft},
        {"MiddleCenter", kAlignMiddleCenter},
        {"MiddleRight", kAlignMiddleRight},
        {"BottomLeft", kAlignBottomLeft},
        {"BottomCenter", kAlignBottomCenter},
        {"BottomRight", kAlignBottomRight},
        {"Left", kAlignTopLeft},
        {"Center", kAlignTopCenter},
        {"Right", kAlignTopRight},
        {"Centre", kAlignMiddleCenter},
    };
    AGF::GUIControl parser;
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        const std::string xml = std::string("<GUILabel><TextAlignment>") + cases[i].Name +
            "</TextAlignment></GUILabel>";
        AGF::DocElem elem = Parse(xml.c_str());
        ASSERT_TRUE(elem != nullptr);
        DataUtil::GUILabelData label;
        parser.ReadLabelData(elem, label);
        EXPECT_EQ(cases[i].Expected, label.TextAlignment) << cases[i].Name;
    }
}

//
// Dialogs
//

TEST_F(AGFReaderBlocks, Dialog)
{
    AGF::DocElem elem = Parse(R"xml(<Dialog>
        <ID>2</ID>
        <ScriptName>dTalk</ScriptName>
        <Name>dTalk</Name>
        <ShowTextParser>True</ShowTextParser>
        <Script>@S
return;</Script>
        <DialogOptions>
          <DialogOption><ID>1</ID><Say>True</Say><Show>True</Show><Text>Hello</Text></DialogOption>
          <DialogOption><ID>2</ID><Say>False</Say><Show>False</Show><Text>Bye</Text></DialogOption>
          <DialogOption><ID>3</ID><Say>True</Say><Show>False</Show><Text>?</Text></DialogOption>
        </DialogOptions>
    </Dialog>)xml");
    ASSERT_TRUE(elem != nullptr);

    AGF::Dialog parser;
    DataUtil::EntityRef ref;
    AGF::ReadEntityRef(ref, parser, elem);
    EXPECT_STREQ("Dialog", ref.TypeName.GetCStr());
    EXPECT_EQ(2, ref.ID);
    EXPECT_STREQ("dTalk", ref.ScriptName.GetCStr());
    EXPECT_EQ(3, parser.ReadOptionCount(elem));
    EXPECT_STREQ("@S\nreturn;", parser.ReadScript(elem).GetCStr());
}

TEST_F(AGFReaderBlocks, DialogWithoutOptions)
{
    AGF::DocElem elem = Parse("<Dialog><ID>0</ID><ScriptName>dEmpty</ScriptName></Dialog>");
    ASSERT_TRUE(elem != nullptr);
    AGF::Dialog parser;
    EXPECT_EQ(0, parser.ReadOptionCount(elem));
    EXPECT_STREQ("", parser.ReadScript(elem).GetCStr());
}

//
// Game-wide lists: variables, custom properties, palette, sprites and so on
//

TEST_F(AGFReaderBlocks, GlobalVariables)
{
    AGF::DocElem game = Parse(R"xml(<Game><GlobalVariables><Variables>
        <GlobalVariable>
          <ArraySize>0</ArraySize>
          <ArrayType>None</ArrayType>
          <DefaultValue>True</DefaultValue>
          <Name>doorOpen</Name>
          <Type>bool</Type>
        </GlobalVariable>
        <GlobalVariable>
          <ArraySize>0</ArraySize>
          <ArrayType>None</ArrayType>
          <DefaultValue />
          <Name>playerName</Name>
          <Type>String</Type>
        </GlobalVariable>
    </Variables></GlobalVariables></Game>)xml");
    ASSERT_TRUE(game != nullptr);
    std::vector<DataUtil::Variable> vars;
    AGF::ReadGlobalVariables(vars, game);

    ASSERT_EQ(2u, vars.size());
    EXPECT_STREQ("bool", vars[0].Type.GetCStr());
    EXPECT_STREQ("doorOpen", vars[0].Name.GetCStr());
    EXPECT_STREQ("True", vars[0].Value.GetCStr());
    EXPECT_STREQ("String", vars[1].Type.GetCStr());
    EXPECT_STREQ("playerName", vars[1].Name.GetCStr());
    EXPECT_STREQ("", vars[1].Value.GetCStr());
}

TEST_F(AGFReaderBlocks, NoGlobalVariables)
{
    AGF::DocElem game = Parse("<Game><GlobalVariables><Variables /></GlobalVariables></Game>");
    ASSERT_TRUE(game != nullptr);
    std::vector<DataUtil::Variable> vars;
    AGF::ReadGlobalVariables(vars, game);
    EXPECT_EQ(0u, vars.size());
    AGF::DocElem none = Parse("<Game />");
    ASSERT_TRUE(none != nullptr);
    AGF::ReadGlobalVariables(vars, none);
    EXPECT_EQ(0u, vars.size());
}

TEST_F(AGFReaderBlocks, CustomPropertySchema)
{
    AGF::DocElem game = Parse(R"xml(<Game><PropertyDefinitions>
        <CustomPropertySchemaItem>
          <AppliesToCharacters>False</AppliesToCharacters>
          <AppliesToHotspots>False</AppliesToHotspots>
          <AppliesToInvItems>False</AppliesToInvItems>
          <AppliesToObjects>False</AppliesToObjects>
          <AppliesToRooms>False</AppliesToRooms>
          <DefaultValue>3</DefaultValue>
          <Description xml:space="preserve">How heavy it is</Description>
          <Name>Weight</Name>
          <Translated>True</Translated>
          <Type>Number</Type>
        </CustomPropertySchemaItem>
        <CustomPropertySchemaItem>
          <DefaultValue>False</DefaultValue>
          <Description />
          <Name>Fragile</Name>
          <Type>Boolean</Type>
        </CustomPropertySchemaItem>
        <CustomPropertySchemaItem>
          <DefaultValue />
          <Description />
          <Name>Comment</Name>
          <Type>Text</Type>
        </CustomPropertySchemaItem>
    </PropertyDefinitions></Game>)xml");
    ASSERT_TRUE(game != nullptr);
    std::vector<DataUtil::CustomPropertySchemaItem> schema;
    AGF::ReadCustomPropertySchema(schema, game);

    ASSERT_EQ(3u, schema.size());
    EXPECT_STREQ("Weight", schema[0].Name.GetCStr());
    EXPECT_STREQ("How heavy it is", schema[0].Description.GetCStr());
    EXPECT_STREQ("3", schema[0].DefaultValue.GetCStr());
    EXPECT_EQ(Common::kPropertyInteger, schema[0].Type);
    EXPECT_FALSE(schema[0].AppliesToCharacters);
    EXPECT_FALSE(schema[0].AppliesToHotspots);
    EXPECT_FALSE(schema[0].AppliesToInvItems);
    EXPECT_FALSE(schema[0].AppliesToObjects);
    EXPECT_FALSE(schema[0].AppliesToRooms);
    EXPECT_TRUE(schema[0].Translated);
    EXPECT_STREQ("Fragile", schema[1].Name.GetCStr());
    EXPECT_EQ(Common::kPropertyBoolean, schema[1].Type);
    // A property applies to every kind of object unless the schema says otherwise
    EXPECT_TRUE(schema[1].AppliesToCharacters);
    EXPECT_TRUE(schema[1].AppliesToHotspots);
    EXPECT_TRUE(schema[1].AppliesToInvItems);
    EXPECT_TRUE(schema[1].AppliesToObjects);
    EXPECT_TRUE(schema[1].AppliesToRooms);
    EXPECT_FALSE(schema[1].Translated);
    EXPECT_STREQ("Comment", schema[2].Name.GetCStr());
    EXPECT_EQ(Common::kPropertyString, schema[2].Type);
}

TEST_F(AGFReaderBlocks, GamePalette)
{
    AGF::DocElem game = Parse(R"xml(<Game><Palette>
        <PaletteEntry Index="0" Type="Gamewide" Red="0" Green="0" Blue="0" />
        <PaletteEntry Index="2" Type="Background" Red="168" Green="84" Blue="42" />
        <PaletteEntry Index="300" Type="Gamewide" Red="1" Green="1" Blue="1" />
    </Palette></Game>)xml");
    ASSERT_TRUE(game != nullptr);
    std::vector<DataUtil::PaletteEntryData> pal;
    pal.resize(4);
    AGF::ReadGamePalette(pal, game);

    // The entry out of the 256 colours is ignored
    ASSERT_EQ(4u, pal.size());
    EXPECT_EQ(DataUtil::kPaletteColourType_Gamewide, pal[0].ColourType);
    EXPECT_EQ(DataUtil::kPaletteColourType_Background, pal[2].ColourType);
    EXPECT_EQ(168, pal[2].Red);
    EXPECT_EQ(84, pal[2].Green);
    EXPECT_EQ(42, pal[2].Blue);
}

TEST_F(AGFReaderBlocks, Sprites)
{
    AGF::DocElem game = Parse(R"xml(<Game><Sprites>
      <SpriteFolder Name="Main">
        <SubFolders>
          <SpriteFolder Name="Backgrounds">
            <SubFolders />
            <Sprites>
              <Sprite Slot="7" Width="320" Height="200" ColorDepth="24" AlphaChannel="False">
                <Source>
                  <FileName>Sprites/Backgrounds/town.png</FileName>
                  <OffsetX>0</OffsetX>
                  <OffsetY>0</OffsetY>
                  <ImportHeight>200</ImportHeight>
                  <ImportWidth>320</ImportWidth>
                  <ImportAsTile>False</ImportAsTile>
                  <Frame>0</Frame>
                  <RemapToGamePalette>False</RemapToGamePalette>
                  <RemapToRoomPalette>False</RemapToRoomPalette>
                  <ImportMethod>LeaveAsIs</ImportMethod>
                  <TransparentColorIndex>0</TransparentColorIndex>
                </Source>
              </Sprite>
            </Sprites>
          </SpriteFolder>
        </SubFolders>
        <Sprites>
          <Sprite Slot="2" Width="16" Height="24" ColorDepth="32" AlphaChannel="True">
            <Source>
              <FileName>Sprites/hero.png</FileName>
              <OffsetX>8</OffsetX>
              <OffsetY>16</OffsetY>
              <ImportHeight>24</ImportHeight>
              <ImportWidth>16</ImportWidth>
              <ImportAsTile>True</ImportAsTile>
              <Frame>3</Frame>
              <RemapToGamePalette>True</RemapToGamePalette>
              <RemapToRoomPalette>True</RemapToRoomPalette>
              <ImportMethod>TopLeft</ImportMethod>
              <TransparentColorIndex>5</TransparentColorIndex>
            </Source>
          </Sprite>
          <Sprite Slot="3" Width="8" Height="8" ColorDepth="8" AlphaChannel="False" />
        </Sprites>
      </SpriteFolder>
    </Sprites></Game>)xml");
    ASSERT_TRUE(game != nullptr);
    std::vector<DataUtil::SpriteData> sprites;
    AGF::ReadSpriteList(sprites, game);

    // Find the sprites by slot, the tests do not depend on how the folders are walked
    ASSERT_EQ(3u, sprites.size());
    const DataUtil::SpriteData *town = nullptr;
    const DataUtil::SpriteData *hero = nullptr;
    const DataUtil::SpriteData *bare = nullptr;
    for (size_t i = 0; i < sprites.size(); ++i)
    {
        if (sprites[i].Slot == 7)
            town = &sprites[i];
        if (sprites[i].Slot == 2)
            hero = &sprites[i];
        if (sprites[i].Slot == 3)
            bare = &sprites[i];
    }
    ASSERT_TRUE(town != nullptr && hero != nullptr && bare != nullptr);

    EXPECT_EQ(320, town->Width);
    EXPECT_EQ(200, town->Height);
    EXPECT_EQ(24, town->ColorDepth);
    EXPECT_FALSE(town->AlphaChannel);
    EXPECT_STREQ("Sprites/Backgrounds/town.png", town->SourceFile.GetCStr());
    EXPECT_EQ(DataUtil::kSpriteImport_LeaveAsIs, town->TransparentColour);

    EXPECT_EQ(16, hero->Width);
    EXPECT_EQ(24, hero->Height);
    EXPECT_EQ(32, hero->ColorDepth);
    EXPECT_TRUE(hero->AlphaChannel);
    EXPECT_STREQ("Sprites/hero.png", hero->SourceFile.GetCStr());
    EXPECT_EQ(8, hero->ImportOffsetX);
    EXPECT_EQ(16, hero->ImportOffsetY);
    EXPECT_EQ(16, hero->ImportWidth);
    EXPECT_EQ(24, hero->ImportHeight);
    EXPECT_TRUE(hero->ImportAsTile);
    EXPECT_EQ(3, hero->ImportFrame);
    EXPECT_TRUE(hero->RemapToGamePalette);
    EXPECT_TRUE(hero->RemapToRoomPalette);
    EXPECT_EQ(DataUtil::kSpriteImport_TopLeft, hero->TransparentColour);
    EXPECT_EQ(5, hero->TransparentColourIndex);

    // A sprite without a source has the defaults
    EXPECT_STREQ("", bare->SourceFile.GetCStr());
    EXPECT_EQ(DataUtil::kSpriteImport_LeaveAsIs, bare->TransparentColour);
}

TEST_F(AGFReaderBlocks, Plugins)
{
    AGF::DocElem game = Parse(R"xml(<Game><Plugins>
        <Plugin><FileName>agsflashlight.dll</FileName><Data>AAEC</Data></Plugin>
        <Plugin><FileName>ags_snowrain.dll</FileName><Data /></Plugin>
    </Plugins></Game>)xml");
    ASSERT_TRUE(game != nullptr);
    std::vector<AGS::Common::String> plugins;
    AGF::ReadPluginList(plugins, game);
    ASSERT_EQ(2u, plugins.size());
    EXPECT_STREQ("agsflashlight.dll", plugins[0].GetCStr());
    EXPECT_STREQ("ags_snowrain.dll", plugins[1].GetCStr());
}

TEST_F(AGFReaderBlocks, Rooms)
{
    AGF::DocElem game = Parse(R"xml(<Game><Rooms Version="4.0.0.33">
      <UnloadedRoomFolder Name="Main">
        <SubFolders>
          <UnloadedRoomFolder Name="Cellar">
            <SubFolders />
            <UnloadedRooms>
              <UnloadedRoom><Description xml:space="preserve">the cellar</Description><Number>1</Number></UnloadedRoom>
            </UnloadedRooms>
          </UnloadedRoomFolder>
        </SubFolders>
        <UnloadedRooms>
          <UnloadedRoom><Description xml:space="preserve">the tower</Description><Number>2</Number></UnloadedRoom>
          <UnloadedRoom><Number>3</Number></UnloadedRoom>
        </UnloadedRooms>
      </UnloadedRoomFolder>
    </Rooms></Game>)xml");
    ASSERT_TRUE(game != nullptr);
    std::vector<std::pair<int, AGS::Common::String> > rooms;
    AGF::ReadRoomList(rooms, game);
    ASSERT_EQ(3u, rooms.size());
    EXPECT_EQ(1, rooms[0].first);
    EXPECT_STREQ("the cellar", rooms[0].second.GetCStr());
    EXPECT_EQ(2, rooms[1].first);
    EXPECT_STREQ("the tower", rooms[1].second.GetCStr());
    EXPECT_EQ(3, rooms[2].first);
    EXPECT_STREQ("", rooms[2].second.GetCStr());
}

TEST_F(AGFReaderBlocks, Translations)
{
    AGF::DocElem game = Parse(R"xml(<Game><Translations>
        <Translation><Name>Italiano</Name></Translation>
        <Translation><Name>Deutsch</Name></Translation>
    </Translations></Game>)xml");
    ASSERT_TRUE(game != nullptr);
    std::vector<AGS::Common::String> names;
    AGF::ReadTranslationList(names, game);
    ASSERT_EQ(2u, names.size());
    EXPECT_STREQ("Italiano", names[0].GetCStr());
    EXPECT_STREQ("Deutsch", names[1].GetCStr());
}

TEST_F(AGFReaderBlocks, Scripts)
{
    AGF::DocElem game = Parse(R"xml(<Game><Scripts>
      <ScriptFolder Name="Main">
        <SubFolders>
          <ScriptFolder Name="modules">
            <SubFolders />
            <ScriptAndHeaders>
              <ScriptAndHeader>
                <ScriptAndHeader_Header>
                  <Script><FileName>tap.ash</FileName><Name /><IsHeader>True</IsHeader></Script>
                </ScriptAndHeader_Header>
                <ScriptAndHeader_Script>
                  <Script><FileName>tap.asc</FileName><Name>tap</Name><IsHeader>False</IsHeader></Script>
                </ScriptAndHeader_Script>
              </ScriptAndHeader>
            </ScriptAndHeaders>
          </ScriptFolder>
        </SubFolders>
        <ScriptAndHeaders>
          <ScriptAndHeader>
            <ScriptAndHeader_Header>
              <Script><FileName>GlobalScript.ash</FileName><Name /><IsHeader>True</IsHeader></Script>
            </ScriptAndHeader_Header>
            <ScriptAndHeader_Script>
              <Script><FileName>GlobalScript.asc</FileName><Name /><IsHeader>False</IsHeader></Script>
            </ScriptAndHeader_Script>
          </ScriptAndHeader>
          <ScriptAndHeader>
            <ScriptAndHeader_Script>
              <Script><FileName>NoHeader.asc</FileName><Name /><IsHeader>False</IsHeader></Script>
            </ScriptAndHeader_Script>
          </ScriptAndHeader>
        </ScriptAndHeaders>
      </ScriptFolder>
    </Scripts></Game>)xml");
    ASSERT_TRUE(game != nullptr);

    // Modules of subfolders come first, then the modules of the folder itself
    std::vector<AGS::Common::String> scripts;
    AGF::ReadScriptList(scripts, game);
    ASSERT_EQ(3u, scripts.size());
    EXPECT_STREQ("tap.asc", scripts[0].GetCStr());
    EXPECT_STREQ("GlobalScript.asc", scripts[1].GetCStr());
    EXPECT_STREQ("NoHeader.asc", scripts[2].GetCStr());

    // The module with no header does not give one
    std::vector<AGS::Common::String> headers;
    AGF::ReadScriptHeaderList(headers, game);
    ASSERT_EQ(2u, headers.size());
    EXPECT_STREQ("tap.ash", headers[0].GetCStr());
    EXPECT_STREQ("GlobalScript.ash", headers[1].GetCStr());
}

//
// Lists of entities: folders, nesting, and the order of items
//

TEST_F(AGFReaderBlocks, FolderListsAreReadSubfoldersFirst)
{
    AGF::DocElem game = Parse(R"xml(<Game><Characters>
      <CharacterFolder Name="Main">
        <SubFolders>
          <CharacterFolder Name="Npcs">
            <SubFolders>
              <CharacterFolder Name="Deep">
                <SubFolders />
                <Characters>
                  <Character><ID>3</ID><ScriptName>cDeep</ScriptName></Character>
                </Characters>
              </CharacterFolder>
            </SubFolders>
            <Characters>
              <Character><ID>2</ID><ScriptName>cNpc</ScriptName></Character>
            </Characters>
          </CharacterFolder>
        </SubFolders>
        <Characters>
          <Character><ID>0</ID><ScriptName>cEgo</ScriptName></Character>
          <Character><ID>1</ID><ScriptName>cFriend</ScriptName></Character>
        </Characters>
      </CharacterFolder>
    </Characters></Game>)xml");
    ASSERT_TRUE(game != nullptr);

    AGF::Characters list;
    AGF::Character parser;
    std::vector<DataUtil::EntityRef> refs = ReadRefs(list, parser, game);
    ASSERT_EQ(4u, refs.size());
    EXPECT_STREQ("cDeep", refs[0].ScriptName.GetCStr());
    EXPECT_EQ(3, refs[0].ID);
    EXPECT_STREQ("cNpc", refs[1].ScriptName.GetCStr());
    EXPECT_STREQ("cEgo", refs[2].ScriptName.GetCStr());
    EXPECT_STREQ("cFriend", refs[3].ScriptName.GetCStr());
    EXPECT_STREQ("Character", refs[0].TypeName.GetCStr());
}

TEST_F(AGFReaderBlocks, EntityListsReadTheirOwnElements)
{
    AGF::DocElem game = Parse(R"xml(<Game>
      <AudioClips><AudioClipFolder Name="Main"><SubFolders />
        <AudioClips><AudioClip><ID>1</ID><ScriptName>aBell</ScriptName></AudioClip></AudioClips>
      </AudioClipFolder></AudioClips>
      <AudioClipTypes>
        <AudioClipType><TypeID>1</TypeID><Name>Sound</Name></AudioClipType>
        <AudioClipType><TypeID>2</TypeID><Name>Music</Name></AudioClipType>
      </AudioClipTypes>
      <Cursors>
        <MouseCursor><ID>0</ID><Name>Walk</Name></MouseCursor>
        <MouseCursor><ID>1</ID><Name>Look</Name></MouseCursor>
      </Cursors>
      <Dialogs><DialogFolder Name="Main"><SubFolders />
        <Dialogs><Dialog><ID>0</ID><ScriptName>dChat</ScriptName><Name>dChat</Name></Dialog></Dialogs>
      </DialogFolder></Dialogs>
      <Fonts>
        <Font><ID>0</ID><Name>Normal</Name></Font>
        <Font><ID>1</ID><Name>Speech</Name></Font>
      </Fonts>
      <InventoryItems><InventoryItemFolder Name="Main"><SubFolders />
        <InventoryItems>
          <InventoryItem><ID>1</ID><ScriptName>iKey</ScriptName><Name>iKey</Name></InventoryItem>
          <InventoryItem><ID>2</ID><ScriptName>iCoin</ScriptName><Name>iCoin</Name></InventoryItem>
        </InventoryItems>
      </InventoryItemFolder></InventoryItems>
      <Views><ViewFolder Name="Default"><SubFolders />
        <Views><View><ID>1</ID><ScriptName>VWALK</ScriptName><Name>VWALK</Name></View></Views>
      </ViewFolder></Views>
    </Game>)xml");
    ASSERT_TRUE(game != nullptr);

    {
        AGF::AudioClips list;
        AGF::AudioClip parser;
        std::vector<DataUtil::EntityRef> refs = ReadRefs(list, parser, game);
        ASSERT_EQ(1u, refs.size());
        EXPECT_EQ(1, refs[0].ID);
        EXPECT_STREQ("aBell", refs[0].ScriptName.GetCStr());
        EXPECT_STREQ("AudioClip", refs[0].TypeName.GetCStr());
    }
    {
        AGF::AudioTypes list;
        AGF::AudioType parser;
        std::vector<DataUtil::EntityRef> refs = ReadRefs(list, parser, game);
        ASSERT_EQ(2u, refs.size());
        EXPECT_EQ(1, refs[0].ID);
        EXPECT_STREQ("eAudioTypeSound", refs[0].ScriptName.GetCStr());
        EXPECT_EQ(2, refs[1].ID);
        EXPECT_STREQ("eAudioTypeMusic", refs[1].ScriptName.GetCStr());
    }
    {
        AGF::Cursors list;
        AGF::Cursor parser;
        std::vector<DataUtil::EntityRef> refs = ReadRefs(list, parser, game);
        ASSERT_EQ(2u, refs.size());
        EXPECT_STREQ("eModeWalk", refs[0].ScriptName.GetCStr());
        EXPECT_STREQ("eModeLook", refs[1].ScriptName.GetCStr());
    }
    {
        AGF::Dialogs list;
        AGF::Dialog parser;
        std::vector<DataUtil::EntityRef> refs = ReadRefs(list, parser, game);
        ASSERT_EQ(1u, refs.size());
        EXPECT_STREQ("dChat", refs[0].ScriptName.GetCStr());
        EXPECT_STREQ("Dialog", refs[0].TypeName.GetCStr());
    }
    {
        AGF::Fonts list;
        AGF::Font parser;
        std::vector<DataUtil::EntityRef> refs = ReadRefs(list, parser, game);
        ASSERT_EQ(2u, refs.size());
        EXPECT_STREQ("eFontNormal", refs[0].ScriptName.GetCStr());
        EXPECT_STREQ("eFontSpeech", refs[1].ScriptName.GetCStr());

        std::vector<int> ids;
        AGF::ReadFontList(ids, game);
        ASSERT_EQ(2u, ids.size());
        EXPECT_EQ(0, ids[0]);
        EXPECT_EQ(1, ids[1]);
    }
    {
        AGF::Inventory list;
        AGF::InventoryItem parser;
        std::vector<DataUtil::EntityRef> refs = ReadRefs(list, parser, game);
        ASSERT_EQ(2u, refs.size());
        EXPECT_EQ(1, refs[0].ID);
        EXPECT_STREQ("iKey", refs[0].ScriptName.GetCStr());
        EXPECT_STREQ("iCoin", refs[1].ScriptName.GetCStr());
        EXPECT_STREQ("InventoryItem", refs[0].TypeName.GetCStr());
    }
    {
        AGF::Views list;
        AGF::View parser;
        std::vector<DataUtil::EntityRef> refs = ReadRefs(list, parser, game);
        ASSERT_EQ(1u, refs.size());
        EXPECT_EQ(1, refs[0].ID);
        EXPECT_STREQ("VWALK", refs[0].ScriptName.GetCStr());
    }
}

TEST_F(AGFReaderBlocks, GUIListGivesGUIsAndControls)
{
    AGF::DocElem game = Parse(R"xml(<Game><GUIs>
      <GUIFolder Name="Main"><SubFolders /><GUIs>
        <GUIMain>
          <NormalGUI><ID>0</ID><Name>gMain</Name><ScriptName>gMain</ScriptName></NormalGUI>
          <Controls>
            <GUIButton><ID>0</ID><ScriptName>btnPlay</ScriptName><Name>btnPlay</Name></GUIButton>
            <GUILabel><ID>1</ID><ScriptName>lblInfo</ScriptName><Name>lblInfo</Name></GUILabel>
            <GUIInventory><ID>2</ID><ScriptName>invAll</ScriptName><Name>invAll</Name></GUIInventory>
            <GUIListBox><ID>3</ID><ScriptName>lstGames</ScriptName><Name>lstGames</Name></GUIListBox>
            <GUISlider><ID>4</ID><ScriptName>sldVol</ScriptName><Name>sldVol</Name></GUISlider>
            <GUITextBox><ID>5</ID><ScriptName>txtName</ScriptName><Name>txtName</Name></GUITextBox>
          </Controls>
        </GUIMain>
        <GUIMain>
          <TextWindowGUI><ID>1</ID><Name>gText</Name><ScriptName>gText</ScriptName></TextWindowGUI>
          <Controls>
            <GUITextWindowEdge><ID>0</ID><ScriptName>Edge0</ScriptName><Name>Edge0</Name></GUITextWindowEdge>
          </Controls>
        </GUIMain>
      </GUIs></GUIFolder>
    </GUIs></Game>)xml");
    ASSERT_TRUE(game != nullptr);

    AGF::GUIs list;
    AGF::GUIMain parser;
    std::vector<AGF::DocElem> elems;
    list.GetAll(game, elems);
    ASSERT_EQ(2u, elems.size());

    std::vector<DataUtil::EntityRef> guis = ReadRefs(list, parser, game);
    ASSERT_EQ(2u, guis.size());
    EXPECT_EQ(0, guis[0].ID);
    EXPECT_STREQ("gMain", guis[0].ScriptName.GetCStr());
    EXPECT_EQ(1, guis[1].ID);
    EXPECT_STREQ("gText", guis[1].ScriptName.GetCStr());

    AGF::GUIControls controls;
    AGF::GUIControl control;
    std::vector<DataUtil::EntityRef> refs = ReadRefs(controls, control, elems[0]);
    ASSERT_EQ(6u, refs.size());
    const char *names[] = {"btnPlay", "lblInfo", "invAll", "lstGames", "sldVol", "txtName"};
    const char *types[] = {"Button", "Label", "InvWindow", "ListBox", "Slider", "TextBox"};
    for (size_t i = 0; i < refs.size(); ++i)
    {
        EXPECT_EQ(static_cast<int>(i), refs[i].ID);
        EXPECT_STREQ(names[i], refs[i].ScriptName.GetCStr());
        EXPECT_STREQ(types[i], refs[i].TypeName.GetCStr());
    }

    refs = ReadRefs(controls, control, elems[1]);
    ASSERT_EQ(1u, refs.size());
    EXPECT_STREQ("Edge0", refs[0].ScriptName.GetCStr());
    EXPECT_STREQ("Button", refs[0].TypeName.GetCStr());
}

TEST_F(AGFReaderBlocks, ScriptNamesAreMadeOfValidCharacters)
{
    AGF::DocElem elem = Parse("<MouseCursor><ID>0</ID><Name>Use-this, now!</Name></MouseCursor>");
    ASSERT_TRUE(elem != nullptr);
    AGF::Cursor parser;
    DataUtil::EntityRef ref;
    AGF::ReadEntityRef(ref, parser, elem);
    EXPECT_STREQ("eModeUsethisnow", ref.ScriptName.GetCStr());
}

TEST_F(AGFReaderBlocks, MissingListsGiveNoEntities)
{
    AGF::DocElem game = Parse("<Game><Settings /></Game>");
    ASSERT_TRUE(game != nullptr);
    AGF::Characters characters;
    AGF::Character character;
    EXPECT_EQ(0u, ReadRefs(characters, character, game).size());
    AGF::Views views;
    AGF::View view;
    EXPECT_EQ(0u, ReadRefs(views, view, game).size());
    std::vector<std::pair<int, AGS::Common::String> > rooms;
    AGF::ReadRoomList(rooms, game);
    EXPECT_EQ(0u, rooms.size());
    std::vector<DataUtil::SpriteData> sprites;
    AGF::ReadSpriteList(sprites, game);
    EXPECT_EQ(0u, sprites.size());
}

TEST_F(AGFReaderBlocks, AudioClipList)
{
    AGF::DocElem game = Parse(R"xml(<Game><AudioClips>
      <AudioClipFolder Name="Main"><SubFolders />
        <AudioClips>
          <AudioClip><ID>0</ID><Index>1</Index><ScriptName>aOne</ScriptName>
            <SourceFileName>Sounds/one.ogg</SourceFileName><FileType>OGG</FileType></AudioClip>
          <AudioClip><ID>1</ID><Index>2</Index><ScriptName>aTwo</ScriptName>
            <SourceFileName>Sounds/two.wav</SourceFileName><FileType>WAV</FileType></AudioClip>
        </AudioClips>
        <AudioClipFolder><DefaultPriority>Normal</DefaultPriority><DefaultRepeat>False</DefaultRepeat>
          <DefaultVolume>90</DefaultVolume></AudioClipFolder>
        <DefaultPriority>Normal</DefaultPriority><DefaultRepeat>False</DefaultRepeat><DefaultVolume>90</DefaultVolume>
      </AudioClipFolder>
    </AudioClips></Game>)xml");
    ASSERT_TRUE(game != nullptr);
    std::vector<DataUtil::AudioClipData> clips;
    AGF::ReadAudioClips(clips, game);
    ASSERT_EQ(2u, clips.size());
    EXPECT_EQ(1, clips[0].Index);
    EXPECT_STREQ("Sounds/one.ogg", clips[0].SourceFileName.GetCStr());
    EXPECT_STREQ("au000001.ogg", clips[0].CacheFileName.GetCStr());
    EXPECT_EQ(DataUtil::kAudioFile_OGG, clips[0].FileType);
    EXPECT_EQ(DataUtil::kAudioFile_WAV, clips[1].FileType);
    EXPECT_STREQ("au000002.wav", clips[1].CacheFileName.GetCStr());
    EXPECT_FALSE(clips[0].Repeat);
    EXPECT_EQ(50, clips[0].Priority);
    EXPECT_EQ(90, clips[0].Volume);
}
