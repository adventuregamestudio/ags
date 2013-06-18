//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// GameState, a class of dynamic game data
//
//=============================================================================
#ifndef __AGS_EE_GAME__GAMESTATE_H
#define __AGS_EE_GAME__GAMESTATE_H

#include "ac/runtime_defines.h"
#include "core/types.h"
#include "media/audio/queuedaudioitem.h"
#include "util/array.h"
#include "util/string.h"

namespace AGS
{

namespace Common { class Stream; }

namespace Engine
{

using Common::Array;
using Common::ObjectArray;
using Common::Stream;
using Common::String;

#define GAME_STATE_RESERVED_INTS 7

class GameState
{
public:
    GameState();
    ~GameState();

    void ReadFromFile_v321(Stream *in);
    void WriteToFile_v321(Stream *out);
    void ReadFromSavedGame(Stream *in);
    void WriteToSavedGame(Stream *out);

private:
    void ReadQueuedAudioItems_Aligned(Stream *in);
    void WriteQueuedAudioItems_Aligned(Stream *out);

    // TODO: all members are currently public; hide them later
public:
    //-----------------------------------------------------
    // !!IMPORTANT!!
    // The following members should strictly correspond to legacy
    // GameState struct, for they are referenced by offset from the
    // script and plugins.
    int32_t     PlayerScore;                // player's current score
    int32_t     UsedCursorMode;             // set by ProcessClick to last cursor mode used
    int32_t     DisabledUserInterface;      // >0 while in cutscene/etc
    int32_t     GlobalScriptTimer;          // obsolete
    int32_t     DebugMode;                  // whether we're in debug mode
    int32_t     GlobalVars[MAXGLOBALVARS];  // obsolete
    int32_t     MessageTime;                // time left for auto-remove messages
    int32_t     UsedInvItemIndex;           // inventory item last used
    int32_t     TopInvItemIndex;            // first inventory item in inventory window
    int32_t     InventoryDisplayedCount;    // total number of items displayed in inventory window
    int32_t     obsolete_inv_numorder;      // ???
    int32_t     InventoryColCount;          // number of items displayed in a row in inventory window
    int32_t     TextDisplaySpeed;           // how quickly text is removed
    int32_t     SierraInventoryBkgColour;   // background used to paint default inv window
    int32_t     SpeechAnimSpeed;            // animation speed of talking anims
    int32_t     InvItemWidth;               // set by SetInvDimensions
    int32_t     InvItemHeight;
    int32_t     SpeechTextOutlineColour;    // colour of outline fonts (default black)
    int32_t     SierraSpeechSwapPortraitSide; // sierra-style speech swap sides
    int32_t     SpeechTextWindowGuiIndex;   // textwindow used for sierra-style speech
    int32_t     FollowingCharacterChangeRoomDelay; // delay before moving following characters into new room
    int32_t     TotalScore;                 // maximum possible score
    int32_t     SkipDisplayMethod;          // how the user can skip normal Display windows
    int32_t     NoMultiLoopRepeat;          // do not repeat multi-loops from start (for backwards compatibility)
    int32_t     RoomScriptFinished;         // on_call finished in room
    int32_t     ClickedInvItemIndex;        // inv item they clicked on
    int32_t     NoTextBkgForVoiceSpeech;    // no textwindow bgrnd when voice speech is used
    int32_t     DialogOptionsMaxWidth;      // max width of dialog options text window
    int32_t     NoHicolorFadeIn;            // fade out but instant in for hi-color
    int32_t     BkgSpeechRelativeToGameSpeed; // is background speech relative to game speed
    int32_t     BkgSpeechStayOnDisplay;     // whether to auto remove bg speech when DisplaySpeech is used
    int32_t     UnfactorVoiceTagFromDisplayTime; // remove "&10" (voice tag) when calculating time for text to stay
    int32_t     Mp3LoopBeforeEnd;           // (**UNUSED**) loop this time before end of track (ms)
    int32_t     MusicMuteForVoicePlay;      // how much to drop music volume by when speech is played
    int32_t     IsInCutscene;               // we are between a StartCutscene and EndCutscene
    int32_t     FastForwardCutscene;        // player has elected to skip cutscene
    int32_t     CurrentRoomWidth;           // width of current room (320-res co-ordinates)
    int32_t     CurrentRoomHeight;          // height of current room (320-res co-ordinates)
    // ** up to here is referenced in the AGSGameOptions plugin interface
    //-----------------------------------------------------
    int32_t     GameSpeedModifier;
    int32_t     ScoreSoundIndex;
    int32_t     TakeoverData;               // value passed to RunAGSGame in previous game
    int32_t     ReplayHotkey;
    int32_t     DialogOptionsX;
    int32_t     DialogOptionsY;
    int32_t     NarratorCharacterIndex;
    int32_t     AmbientSoundsPersist;       // do not stop ambient sounds on room change
    int32_t     LipsyncSpeed;
    int32_t     CloseMouthSpeechTime;       // ????
    int32_t     DisableAntiAliasing;
    int32_t     TextSpeedModifier;
    int32_t     DisplayTextAlignment;
    int32_t     SpeechBubbleWidth;
    int32_t     DialogOptionsMinWidth;
    int32_t     DisableDialogParser;
    int32_t     RoomBkgAnimSpeed;           // the setting for this room
    int32_t     TopBarBkgColour;
    int32_t     TopBarTextColour;
    int32_t     TopBarBorderColour;
    int32_t     TopBarBorderWidth;
    int32_t     TopBarY;
    int32_t     ScreenshotWidth;
    int32_t     ScreenshotHeight;
    int32_t     TopBarFont;
    int32_t     SpeechTextAlignment;
    int32_t     AutoUseWalktoPoints;
    int32_t     InventoryGreysOutWhenDisabled;
    int32_t     SpeechSkipKey;
    int32_t     GameAbortKey;
    int32_t     FadeToRed;
    int32_t     FadeToGreen;
    int32_t     FadeToBlue;
    int32_t     ShowSingleDialogOption;
    int32_t     KeepScreenDuringInstantTransition;
    int32_t     DialogOptionReadColour;     // color of dialog options that were read at least once
    int32_t     StopDialogAtEnd;            // ????
    int32_t     SpeechPortraitPlacement;    // speech portrait placement mode (automatic/custom)
    int32_t     SpeechPortraitX;            // a speech portrait x offset from corresponding screen side
    int32_t     SpeechPortraitY;            // a speech portrait y offset 
    int32_t     Reserved[GAME_STATE_RESERVED_INTS];
    // ** up to here is referenced in the script "game." object
    //-----------------------------------------------------
    // end of exported part
    //-----------------------------------------------------

    int32_t     IsRecording;                // user is recording their moves
    int32_t     IsPlayback;                 // playing back recording
    int16_t     GameStep;                   // step number for matching recordings
    int32_t     RandomSeed;                 // random seed
    int32_t     PlayerOnRegionIndex;        // player's current region
    int32_t     ScreenIsFadedOut;           // the screen is currently black
    int32_t     TestInteractionMode;        // do not run interaction, just check if it exists
    int32_t     RoomBkgFrameIndex;          // for animating backgrounds
    int32_t     RoomBkgAnimDelay;
    int32_t     MusicVolumeWas;             // before the volume drop
    int16_t     WaitCounter;
    int16_t     MouseBoundLeft;
    int16_t     MouseBoundRight;
    int16_t     MouseBoundTop;
    int16_t     MouseBoundBottom;
    int32_t     TransitionStyle;
    int32_t     RoomBkgFrameLocked;
    Array<int32_t> GlobalScriptVariables; // [MAXGSVALUES]
    int32_t     CurrentMusicIndex;
    int32_t     MusicLoopMode;
    int32_t     MusicMasterVolume;
    int32_t     DigitalMasterVolume;
    Array<bool> WalkAreasEnabled; // [MAX_WALK_AREAS+1]
    int16_t     ScreenFlipped;
    int16_t     ViewportLocked;
    int32_t     CharacterEnterRoomAtX;
    int32_t     CharacterEnterRoomAtY;
    int32_t     CharacterEnterRoomAtEdge;
    int32_t     SpeechVoiceMode;
    int32_t     SkipSpeechMode;
    Array<int32_t> ScriptTimers; //[MAX_TIMERS];
    int32_t     SoundVolume;
    int32_t     SpeechVolume;
    int32_t     NormalFont;
    int32_t     SpeechFont;
    int8_t      SkipWaitMode;
    int32_t     LastSpeechPortraitCharacter;
    int32_t     UseSeparateMusicLib;
    int32_t     InConversation;
    int32_t     ScreenTint;
    int32_t     ParsedWordCount;
    Array<int16_t> ParsedWords; //[MAX_PARSED_WORDS];
    String      UnknownWord;
    int32_t     RawDrawColour;
    Array<int32_t> RoomBkgWasModified; //[MAX_BSCENE];
    Array<int16_t> SavedGameFileNumbers; //[MAXSAVEGAMES]; // ????
    int32_t     RoomChangeCount;
    int32_t     MouseCursorHidden; // should be int, because incremented and decremented
    int32_t     SilentMidiIndex;
    int32_t     SilentMidiChannel;
    int32_t     CurrentMusicLoopMode;  // remember what the loop flag was when this music started
    uint32_t    ShakeScreenDelay;  // unsigned long to match loopcounter
    int32_t     ShakeScreenAmount;
    int32_t     ShakeScreenLength;
    int32_t     RoomTintRed;
    int32_t     RoomTintGreen;
    int32_t     RoomTintBlue;
    int32_t     RoomTintLevel;
    int32_t     RoomTintLight;
    int32_t     PlayMusicAfterCustsceneSkip; // music index to play after finished fast-forwarding cutscene
    int32_t     SkipUntilCharacterStops;     // character index, or -1 if not skipping
    int32_t     GetLocationNameLastTime;
    int32_t     GetLocationNameSaveCursor;
    int32_t     RestoreCursorModeTo;
    int32_t     RestoreCursorImageTo;
    int16_t     MusicQueueLength;
    Array<int16_t> MusicQueue; //[MAX_QUEUED_MUSIC];
    int16_t     NewMusicQueueLength;
    int16_t     CrossfadingOutChannel;
    int16_t     CrossfadeStep;
    int16_t     CrossfadeOutVolumePerStep;
    int16_t     CrossfadeInitialVolumeOut;
    int16_t     CrossfadingInChannel;
    int16_t     CrossfadeInVolumePerStep;
    int16_t     CrossfadeFinalVolumeIn;
    Array<QueuedAudioItem> NewMusicQueue; //[MAX_QUEUED_MUSIC];
    String      TakeoverFrom;
    String      PlayMp3FileName;
    ObjectArray<String> GlobalStrings; //[MAXGLOBALSTRINGS];
    String      LastParserEntry;
    String      GameName;
    int32_t     GroundLevelAreasDisabled;
    int32_t     NextRoomTransition;
    int32_t     GammaAdjustment;
    int16_t     TemporarilyHidCharacter;  // Hide Player Charactr ticked
    int16_t     InventoryBackwardsCompatible;
    Array<int32_t> GuiDrawOrder;
    ObjectArray<String> DoOnceTokens;
    int32_t     DoOnceTokenCount;
    int32_t     DisplayTextMinTimeMs;
    int32_t     DisplayTextIgnoreUserInputDelayMs;
    uint32_t    IgnoreUserInputUntilTime;
    Array<int32_t> DefaultAudioTypeVolumes; //[MAX_AUDIO_TYPES];
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__GAMESTATE_H
