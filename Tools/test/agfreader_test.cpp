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
#include <cstring>
#include <fstream>
#include <new>
#include <tinyxml2.h>
#include "gtest/gtest.h"
#include "data/agfreader.h"
#include "data/scriptgen.h"

using namespace AGS;

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
        f << "<AGSEditorDocument Version=\"3.0.3.2\" VersionIndex=\"3060316\" EditorVersion=\"3.6.4.0\">"
             "<Game><Settings/><GUIs>"
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

// A default-constructed GameSettings must have the scalar defaults of AGS.Types.Settings (Editor)
TEST(AGFReader, GameSettingsDefaultConstructed)
{
    // not value-initialized: members without an initializer would be indeterminate here
    alignas(DataUtil::GameSettings) unsigned char storage[sizeof(DataUtil::GameSettings)];
    memset(storage, 0xA5, sizeof(storage));
    DataUtil::GameSettings *s = new (storage) DataUtil::GameSettings;
    EXPECT_FALSE(s->AllowRelativeAssetResolutions);
    EXPECT_FALSE(s->AlwaysDisplayTextAsSpeech);
    EXPECT_EQ(1, s->AndroidAppVersionCode);
    EXPECT_FALSE(s->AntiAliasFonts);
    EXPECT_FALSE(s->AntiGlideMode);
    EXPECT_FALSE(s->AttachDataToExe);
    EXPECT_EQ(1, s->AudioIndexer); // AudioClip.FixedIndexBase
    EXPECT_TRUE(s->AutoMoveInWalkMode);
    EXPECT_FALSE(s->BackwardsText);
    EXPECT_TRUE(s->ClipGUIControls);
    EXPECT_FALSE(s->DebugMode);
    EXPECT_EQ(1, s->DefaultRoomMaskResolution);
    EXPECT_FALSE(s->DialogOptionsBackwards);
    EXPECT_EQ(0, s->DialogOptionsBullet);
    EXPECT_EQ(0, s->DialogOptionsGUI);
    EXPECT_EQ(0, s->DialogOptionsGap);
    EXPECT_FALSE(s->DisplayMultipleInventory);
    EXPECT_FALSE(s->DisplaySingleDialogOption);
    EXPECT_TRUE(s->EnforceNewAudio);
    EXPECT_TRUE(s->EnforceNewStrings);
    EXPECT_TRUE(s->EnforceObjectBasedScript);
    EXPECT_FALSE(s->GUIHandleOnlyLeftMouseButton);
    EXPECT_EQ(5, s->GlobalSpeechAnimationDelay);
    EXPECT_TRUE(s->HandleInvClicksInScript);
    EXPECT_TRUE(s->InventoryCursors);
    EXPECT_EQ(0u, s->InventoryHotspotMarkerCrosshairColor);
    EXPECT_EQ(0u, s->InventoryHotspotMarkerDotColor);
    EXPECT_EQ(0, s->InventoryHotspotMarkerSprite);
    EXPECT_TRUE(s->LeftToRightPrecedence);
    EXPECT_FALSE(s->LetterboxMode);
    EXPECT_EQ(0, s->MaximumScore);
    EXPECT_TRUE(s->MouseWheelEnabled);
    EXPECT_TRUE(s->OptimizeSpriteStorage);
    EXPECT_TRUE(s->PixelPerfect);
    EXPECT_EQ(0, s->PlaySoundOnScore); // AudioClip.FixedIndexNoValue
    EXPECT_TRUE(s->RunGameLoopsWhileDialogOptionsDisplayed);
    EXPECT_FALSE(s->SaveScreenshots);
    EXPECT_TRUE(s->ScaleCharacterSpriteOffsets);
    EXPECT_FALSE(s->ScaleMovementSpeedWithMaskResolution);
    EXPECT_EQ(0, s->TextWindowGUI);
    EXPECT_EQ(0, s->ThoughtGUI);
    EXPECT_FALSE(s->TurnBeforeFacing);
    EXPECT_FALSE(s->TurnBeforeWalking);
    EXPECT_EQ(0, s->UniqueID);
    EXPECT_FALSE(s->UseGlobalSpeechAnimationDelay);
    EXPECT_FALSE(s->UseLowResCoordinatesInScript);
    EXPECT_FALSE(s->UseOldCustomDialogOptionsAPI);
    EXPECT_FALSE(s->UseOldKeyboardHandling);
    EXPECT_FALSE(s->UseOldVoiceClipNaming);
    EXPECT_FALSE(s->WalkInLookMode);
    s->~GameSettings();
}

TEST(AGFReader, GameSettingsAbsentElementsGiveEditorDefaults)
{
    tinyxml2::XMLDocument doc;
    ASSERT_EQ(tinyxml2::XML_SUCCESS, doc.Parse("<Game><Settings /></Game>"));
    DataUtil::GameSettings s;
    AGF::ReadGameSettings(s, doc.RootElement());
    EXPECT_TRUE(s.PixelPerfect);
    EXPECT_FALSE(s.DebugMode);
    EXPECT_FALSE(s.AntiGlideMode);
    EXPECT_TRUE(s.LeftToRightPrecedence);
    EXPECT_EQ(60, s.GameFPS);
    EXPECT_EQ(5, s.GlobalSpeechAnimationDelay);
    EXPECT_EQ(1, s.AudioIndexer);
    EXPECT_FALSE(s.UseLowResCoordinatesInScript);
    // Settings.FromXml sets these two to false before reading, so the Editor does not turn them on
    // in a project from before the settings existed
    EXPECT_FALSE(s.EnforceNewAudio);
    EXPECT_FALSE(s.RunGameLoopsWhileDialogOptionsDisplayed);

    // present elements override the defaults
    tinyxml2::XMLDocument doc2;
    ASSERT_EQ(tinyxml2::XML_SUCCESS, doc2.Parse(
        "<Game><Settings><PixelPerfect>False</PixelPerfect><GameFPS>30</GameFPS></Settings></Game>"));
    DataUtil::GameSettings s2;
    AGF::ReadGameSettings(s2, doc2.RootElement());
    EXPECT_FALSE(s2.PixelPerfect);
    EXPECT_EQ(30, s2.GameFPS);
    EXPECT_FALSE(s2.DebugMode);
}
