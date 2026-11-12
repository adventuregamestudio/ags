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
#include <new>
#include "gtest/gtest.h"
#include "data/agfreader.h"

using namespace AGS;

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
    EXPECT_TRUE(s->AntiGlideMode);
    EXPECT_FALSE(s->AttachDataToExe);
    EXPECT_EQ(1, s->AudioIndexer); // AudioClip.FixedIndexBase
    EXPECT_TRUE(s->AutoMoveInWalkMode);
    EXPECT_FALSE(s->BackwardsText);
    EXPECT_TRUE(s->ClipGUIControls);
    EXPECT_TRUE(s->DebugMode);
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
    EXPECT_EQ(-1, s->PlaySoundOnScore);
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
    EXPECT_TRUE(s->UseLowResCoordinatesInScript);
    EXPECT_FALSE(s->UseOldCustomDialogOptionsAPI);
    EXPECT_FALSE(s->UseOldKeyboardHandling);
    EXPECT_FALSE(s->UseOldVoiceClipNaming);
    EXPECT_FALSE(s->WalkInLookMode);
    s->~GameSettings();
}
