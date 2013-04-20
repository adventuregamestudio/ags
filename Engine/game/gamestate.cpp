
#include "game/gamestate.h"
#include "util/alignedstream.h"

namespace AGS
{
namespace Engine
{

using Common::AlignedStream;

GameState::GameState()
{
    GlobalScriptVariables.New(MAXGSVALUES);
    WalkAreasEnabled.New(MAX_WALK_AREAS+1);
    ScriptTimers.New(MAX_TIMERS);
    ParsedWords.New(MAX_PARSED_WORDS);
    RoomBkgWasModified.New(MAX_BSCENE);
    SavedGameFileNumbers.New(MAXSAVEGAMES);
    MusicQueue.New(MAX_QUEUED_MUSIC);
    NewMusicQueue.New(MAX_QUEUED_MUSIC);
    GlobalStrings.New(MAXGLOBALSTRINGS);
    DefaultAudioTypeVolumes.New(MAX_AUDIO_TYPES);
}

GameState::~GameState()
{
}

void GameState::ReadFromFile_v321(Stream *in)
{
    PlayerScore = in->ReadInt32();
    UsedCursorMode = in->ReadInt32();
    DisabledUserInterface = in->ReadInt32();
    GlobalScriptTimer = in->ReadInt32();
    DebugMode = in->ReadInt32();
    in->ReadArrayOfInt32(GlobalVars, MAXGLOBALVARS);
    MessageTime = in->ReadInt32();
    UsedInvItemIndex = in->ReadInt32();
    TopInvItemIndex = in->ReadInt32();
    InventoryDisplayedCount = in->ReadInt32();
    obsolete_inv_numorder = in->ReadInt32();
    InventoryColCount = in->ReadInt32();
    TextDisplaySpeed = in->ReadInt32();
    SierraInventoryBkgColour = in->ReadInt32();
    SpeechAnimSpeed = in->ReadInt32();
    InvItemWidth = in->ReadInt32();
    InvItemHeight = in->ReadInt32();
    SpeechTextOutlineColour = in->ReadInt32();
    SierraSpeechSwapPortraitSide = in->ReadInt32();
    SpeechTextWindowGuiIndex = in->ReadInt32();
    FollowingCharacterChangeRoomDelay = in->ReadInt32();
    TotalScore = in->ReadInt32();
    SkipDisplayMethod = in->ReadInt32();
    NoMultiLoopRepeat = in->ReadInt32();
    RoomScriptFinished = in->ReadInt32();
    ClickedInvItemIndex = in->ReadInt32();
    NoTextBkgForVoiceSpeech = in->ReadInt32();
    DialogOptionsMaxWidth = in->ReadInt32();
    NoHicolorFadeIn = in->ReadInt32();
    BkgSpeechRelativeToGameSpeed = in->ReadInt32();
    BkgSpeechStayOnDisplay = in->ReadInt32();
    UnfactorVoiceTagFromDisplayTime = in->ReadInt32();
    Mp3LoopBeforeEnd = in->ReadInt32();
    MusicMuteForVoicePlay = in->ReadInt32();
    IsInCutscene = in->ReadInt32();
    FastForwardCutscene = in->ReadInt32();
    CurrentRoomWidth = in->ReadInt32();
    CurrentRoomHeight = in->ReadInt32();
    GameSpeedModifier = in->ReadInt32();
    ScoreSoundIndex = in->ReadInt32();
    TakeoverData = in->ReadInt32();
    ReplayHotkey = in->ReadInt32();
    DialogOptionsX = in->ReadInt32();
    DialogOptionsY = in->ReadInt32();
    NarratorCharacterIndex = in->ReadInt32();
    AmbientSoundsPersist = in->ReadInt32();
    LipsyncSpeed = in->ReadInt32();
    CloseMouthSpeechTime = in->ReadInt32();
    DisableAntiAliasing = in->ReadInt32();
    TextSpeedModifier = in->ReadInt32();
    DisplayTextAlignment = in->ReadInt32();
    SpeechBubbleWidth = in->ReadInt32();
    DialogOptionsMinWidth = in->ReadInt32();
    DisableDialogParser = in->ReadInt32();
    RoomBkgAnimSpeed = in->ReadInt32();  // the setting for this room
    TopBarBkgColour= in->ReadInt32();
    TopBarTextColour = in->ReadInt32();
    TopBarBorderColour = in->ReadInt32();
    TopBarBorderWidth = in->ReadInt32();
    TopBarY = in->ReadInt32();
    ScreenshotWidth = in->ReadInt32();
    ScreenshotHeight = in->ReadInt32();
    TopBarFont = in->ReadInt32();
    SpeechTextAlignment = in->ReadInt32();
    AutoUseWalktoPoints = in->ReadInt32();
    InventoryGreysOutWhenDisabled = in->ReadInt32();
    SpeechSkipKey = in->ReadInt32();
    GameAbortKey = in->ReadInt32();
    FadeToRed = in->ReadInt32();
    FadeToGreen = in->ReadInt32();
    FadeToBlue = in->ReadInt32();
    ShowSingleDialogOption = in->ReadInt32();
    KeepScreenDuringInstantTransition = in->ReadInt32();
    DialogOptionReadColour = in->ReadInt32();
    StopDialogAtEnd = in->ReadInt32();
    in->ReadArrayOfInt32(Reserved, 10);
    // ** up to here is referenced in the script "game." object
    IsRecording = in->ReadInt32();   // user is recording their moves
    IsPlayback = in->ReadInt32();    // playing back recording
    GameStep = in->ReadInt16();    // step number for matching recordings
    RandomSeed = in->ReadInt32();    // random seed
    PlayerOnRegionIndex = in->ReadInt32();    // player's current region
    ScreenIsFadedOut = in->ReadInt32(); // the screen is currently black
    TestInteractionMode = in->ReadInt32();
    RoomBkgFrameIndex = in->ReadInt32();
    RoomBkgAnimDelay = in->ReadInt32();  // for animating backgrounds
    MusicVolumeWas = in->ReadInt32();  // before the volume drop
    WaitCounter = in->ReadInt16();
    MouseBoundLeft = in->ReadInt16();
    MouseBoundRight = in->ReadInt16();
    MouseBoundTop = in->ReadInt16();
    MouseBoundBottom = in->ReadInt16();
    TransitionStyle = in->ReadInt32();
    RoomBkgFrameLocked = in->ReadInt32();
    GlobalScriptVariables.ReadRawOver(in, MAXGSVALUES);
    CurrentMusicIndex = in->ReadInt32();
    MusicLoopMode = in->ReadInt32();
    MusicMasterVolume = in->ReadInt32();
    DigitalMasterVolume = in->ReadInt32();
    WalkAreasEnabled.ReadRawOver(in, MAX_WALK_AREAS+1);
    ScreenFlipped = in->ReadInt16();
    ViewportLocked = in->ReadInt16();
    CharacterEnterRoomAtX = in->ReadInt32();
    CharacterEnterRoomAtY = in->ReadInt32();
    CharacterEnterRoomAtEdge = in->ReadInt32();
    SpeechVoiceMode = in->ReadInt32();
    SkipSpeechMode = in->ReadInt32();
    ScriptTimers.ReadRawOver(in, MAX_TIMERS);
    SoundVolume = in->ReadInt32();
    SpeechVolume = in->ReadInt32();
    NormalFont = in->ReadInt32();
    SpeechFont = in->ReadInt32();
    SkipWaitMode = in->ReadInt8();
    LastSpeechPortraitCharacter = in->ReadInt32();
    UseSeparateMusicLib = in->ReadInt32();
    InConversation = in->ReadInt32();
    ScreenTint = in->ReadInt32();
    ParsedWordCount = in->ReadInt32();
    ParsedWords.ReadRawOver(in, MAX_PARSED_WORDS);
    UnknownWord.ReadCount(in, 100);
    RawDrawColour = in->ReadInt32();
    RoomBkgWasModified.ReadRawOver(in, MAX_BSCENE);
    SavedGameFileNumbers.ReadRawOver(in, MAXSAVEGAMES);
    RoomChangeCount = in->ReadInt32();
    MouseCursorHidden = in->ReadInt32();
    SilentMidiIndex = in->ReadInt32();
    SilentMidiChannel = in->ReadInt32();
    CurrentMusicLoopMode = in->ReadInt32();
    ShakeScreenDelay = in->ReadInt32();
    ShakeScreenAmount = in->ReadInt32();
    ShakeScreenLength = in->ReadInt32();
    RoomTintRed = in->ReadInt32();
    RoomTintGreen = in->ReadInt32();
    RoomTintBlue = in->ReadInt32();
    RoomTintLevel = in->ReadInt32();
    RoomTintLight = in->ReadInt32();
    PlayMusicAfterCustsceneSkip = in->ReadInt32();
    SkipUntilCharacterStops = in->ReadInt32();
    GetLocationNameLastTime = in->ReadInt32();
    GetLocationNameSaveCursor = in->ReadInt32();
    RestoreCursorModeTo = in->ReadInt32();
    RestoreCursorImageTo = in->ReadInt32();
    MusicQueueLength = in->ReadInt16();
    MusicQueue.ReadRawOver(in, MAX_QUEUED_MUSIC);
    NewMusicQueueLength = in->ReadInt16();
    CrossfadingOutChannel = in->ReadInt16();
    CrossfadeStep = in->ReadInt16();
    CrossfadeOutVolumePerStep = in->ReadInt16();
    CrossfadeInitialVolumeOut = in->ReadInt16();
    CrossfadingInChannel = in->ReadInt16();
    CrossfadeInVolumePerStep = in->ReadInt16();
    CrossfadeFinalVolumeIn = in->ReadInt16();

    ReadQueuedAudioItems_Aligned(in);

    TakeoverFrom.ReadCount(in, 50);
    PlayMp3FileName.ReadCount(in, PLAYMP3FILE_MAX_FILENAME_LEN);
    for (int i = 0; i < MAXGLOBALSTRINGS; ++i)
    {
        GlobalStrings[i].ReadCount(in, MAX_MAXSTRLEN);
    }
    LastParserEntry.ReadCount(in, MAX_MAXSTRLEN);
    GameName.ReadCount(in, 100);
    GroundLevelAreasDisabled = in->ReadInt32();
    NextRoomTransition = in->ReadInt32();
    GammaAdjustment = in->ReadInt32();
    TemporarilyHidCharacter = in->ReadInt16();
    InventoryBackwardsCompatible = in->ReadInt16();
    in->ReadInt32(); // gui_draw_order
    in->ReadInt32(); // do_once_tokens;
    DoOnceTokenCount = in->ReadInt32();
    DisplayTextMinTimeMs = in->ReadInt32();
    DisplayTextIgnoreUserInputDelayMs = in->ReadInt32();
    IgnoreUserInputUntilTime = in->ReadInt32();
    DefaultAudioTypeVolumes.ReadRawOver(in, MAX_AUDIO_TYPES);
}

void GameState::WriteToFile_v321(Stream *out)
{
    out->WriteInt32(PlayerScore);
    out->WriteInt32(UsedCursorMode);
    out->WriteInt32(DisabledUserInterface);
    out->WriteInt32(GlobalScriptTimer);
    out->WriteInt32(DebugMode);
    out->WriteArrayOfInt32(GlobalVars, MAXGLOBALVARS);
    out->WriteInt32(MessageTime);
    out->WriteInt32(UsedInvItemIndex);
    out->WriteInt32(TopInvItemIndex);
    out->WriteInt32(InventoryDisplayedCount);
    out->WriteInt32(obsolete_inv_numorder);
    out->WriteInt32(InventoryColCount);
    out->WriteInt32(TextDisplaySpeed);
    out->WriteInt32(SierraInventoryBkgColour);
    out->WriteInt32(SpeechAnimSpeed);
    out->WriteInt32(InvItemWidth);
    out->WriteInt32(InvItemHeight);
    out->WriteInt32(SpeechTextOutlineColour);
    out->WriteInt32(SierraSpeechSwapPortraitSide);
    out->WriteInt32(SpeechTextWindowGuiIndex);
    out->WriteInt32(FollowingCharacterChangeRoomDelay);
    out->WriteInt32(TotalScore);
    out->WriteInt32(SkipDisplayMethod);
    out->WriteInt32(NoMultiLoopRepeat);
    out->WriteInt32(RoomScriptFinished);
    out->WriteInt32(ClickedInvItemIndex);
    out->WriteInt32(NoTextBkgForVoiceSpeech);
    out->WriteInt32(DialogOptionsMaxWidth);
    out->WriteInt32(NoHicolorFadeIn);
    out->WriteInt32(BkgSpeechRelativeToGameSpeed);
    out->WriteInt32(BkgSpeechStayOnDisplay);
    out->WriteInt32(UnfactorVoiceTagFromDisplayTime);
    out->WriteInt32(Mp3LoopBeforeEnd);
    out->WriteInt32(MusicMuteForVoicePlay);
    out->WriteInt32(IsInCutscene);
    out->WriteInt32(FastForwardCutscene);
    out->WriteInt32(CurrentRoomWidth);
    out->WriteInt32(CurrentRoomHeight);
    out->WriteInt32(GameSpeedModifier);
    out->WriteInt32(ScoreSoundIndex);
    out->WriteInt32(TakeoverData);
    out->WriteInt32(ReplayHotkey);
    out->WriteInt32(DialogOptionsX);
    out->WriteInt32(DialogOptionsY);
    out->WriteInt32(NarratorCharacterIndex);
    out->WriteInt32(AmbientSoundsPersist);
    out->WriteInt32(LipsyncSpeed);
    out->WriteInt32(CloseMouthSpeechTime);
    out->WriteInt32(DisableAntiAliasing);
    out->WriteInt32(TextSpeedModifier);
    out->WriteInt32(DisplayTextAlignment);
    out->WriteInt32(SpeechBubbleWidth);
    out->WriteInt32(DialogOptionsMinWidth);
    out->WriteInt32(DisableDialogParser);
    out->WriteInt32(RoomBkgAnimSpeed);  // the setting for this room
    out->WriteInt32(TopBarBkgColour);
    out->WriteInt32(TopBarTextColour);
    out->WriteInt32(TopBarBorderColour);
    out->WriteInt32(TopBarBorderWidth);
    out->WriteInt32(TopBarY);
    out->WriteInt32(ScreenshotWidth);
    out->WriteInt32(ScreenshotHeight);
    out->WriteInt32(TopBarFont);
    out->WriteInt32(SpeechTextAlignment);
    out->WriteInt32(AutoUseWalktoPoints);
    out->WriteInt32(InventoryGreysOutWhenDisabled);
    out->WriteInt32(SpeechSkipKey);
    out->WriteInt32(GameAbortKey);
    out->WriteInt32(FadeToRed);
    out->WriteInt32(FadeToGreen);
    out->WriteInt32(FadeToBlue);
    out->WriteInt32(ShowSingleDialogOption);
    out->WriteInt32(KeepScreenDuringInstantTransition);
    out->WriteInt32(DialogOptionReadColour);
    out->WriteInt32(StopDialogAtEnd);
    out->WriteArrayOfInt32(Reserved, 10);
    // ** up to here is referenced in the script "game." object
    out->WriteInt32( IsRecording);   // user is recording their moves
    out->WriteInt32( IsPlayback);    // playing back recording
    out->WriteInt16(GameStep);    // step number for matching recordings
    out->WriteInt32(RandomSeed);    // random seed
    out->WriteInt32( PlayerOnRegionIndex);    // player's current region
    out->WriteInt32( ScreenIsFadedOut); // the screen is currently black
    out->WriteInt32( TestInteractionMode);
    out->WriteInt32( RoomBkgFrameIndex);
    out->WriteInt32( RoomBkgAnimDelay);  // for animating backgrounds
    out->WriteInt32( MusicVolumeWas);  // before the volume drop
    out->WriteInt16(WaitCounter);
    out->WriteInt16(MouseBoundLeft);
    out->WriteInt16(MouseBoundRight);
    out->WriteInt16(MouseBoundTop);
    out->WriteInt16(MouseBoundBottom);
    out->WriteInt32( TransitionStyle);
    out->WriteInt32( RoomBkgFrameLocked);
    GlobalScriptVariables.WriteRaw(out);
    out->WriteInt32( CurrentMusicIndex);
    out->WriteInt32( MusicLoopMode);
    out->WriteInt32( MusicMasterVolume);
    out->WriteInt32( DigitalMasterVolume);
    WalkAreasEnabled.WriteRaw(out);
    out->WriteInt16( ScreenFlipped);
    out->WriteInt16( ViewportLocked);
    out->WriteInt32( CharacterEnterRoomAtX);
    out->WriteInt32( CharacterEnterRoomAtY);
    out->WriteInt32( CharacterEnterRoomAtEdge);
    out->WriteInt32( SpeechVoiceMode);
    out->WriteInt32( SkipSpeechMode);
    ScriptTimers.WriteRaw(out);
    out->WriteInt32( SoundVolume);
    out->WriteInt32( SpeechVolume);
    out->WriteInt32( NormalFont);
    out->WriteInt32( SpeechFont);
    out->WriteInt8( SkipWaitMode);
    out->WriteInt32( LastSpeechPortraitCharacter);
    out->WriteInt32( UseSeparateMusicLib);
    out->WriteInt32( InConversation);
    out->WriteInt32( ScreenTint);
    out->WriteInt32( ParsedWordCount);
    ParsedWords.WriteRaw(out);
    UnknownWord.WriteCount(out, 100);
    out->WriteInt32( RawDrawColour);
    RoomBkgWasModified.WriteRaw(out);
    SavedGameFileNumbers.WriteRaw(out);
    out->WriteInt32( RoomChangeCount);
    out->WriteInt32( MouseCursorHidden);
    out->WriteInt32( SilentMidiIndex);
    out->WriteInt32( SilentMidiChannel);
    out->WriteInt32( CurrentMusicLoopMode);
    out->WriteInt32( ShakeScreenDelay);
    out->WriteInt32( ShakeScreenAmount);
    out->WriteInt32( ShakeScreenLength);
    out->WriteInt32( RoomTintRed);
    out->WriteInt32( RoomTintGreen);
    out->WriteInt32( RoomTintBlue);
    out->WriteInt32( RoomTintLevel);
    out->WriteInt32( RoomTintLight);
    out->WriteInt32( PlayMusicAfterCustsceneSkip);
    out->WriteInt32( SkipUntilCharacterStops);
    out->WriteInt32( GetLocationNameLastTime);
    out->WriteInt32( GetLocationNameSaveCursor);
    out->WriteInt32( RestoreCursorModeTo);
    out->WriteInt32( RestoreCursorImageTo);
    out->WriteInt16( MusicQueueLength);
    MusicQueue.WriteRaw(out);
    out->WriteInt16( NewMusicQueueLength);
    out->WriteInt16( CrossfadingOutChannel);
    out->WriteInt16( CrossfadeStep);
    out->WriteInt16( CrossfadeOutVolumePerStep);
    out->WriteInt16( CrossfadeInitialVolumeOut);
    out->WriteInt16( CrossfadingInChannel);
    out->WriteInt16( CrossfadeInVolumePerStep);
    out->WriteInt16( CrossfadeFinalVolumeIn);

    WriteQueuedAudioItems_Aligned(out);

    TakeoverFrom.WriteCount(out, 50);
    PlayMp3FileName.WriteCount(out, PLAYMP3FILE_MAX_FILENAME_LEN);
    for (int i = 0; i < MAXGLOBALSTRINGS; ++i)
    {
        GlobalStrings[i].WriteCount(out, MAX_MAXSTRLEN);
    }
    LastParserEntry.WriteCount(out, MAX_MAXSTRLEN);
    GameName.WriteCount(out, 100);
    out->WriteInt32( GroundLevelAreasDisabled);
    out->WriteInt32( NextRoomTransition);
    out->WriteInt32( GammaAdjustment);
    out->WriteInt16(TemporarilyHidCharacter);
    out->WriteInt16(InventoryBackwardsCompatible);
    out->WriteInt32(0); // gui_draw_order
    out->WriteInt32(0); // do_once_tokens
    out->WriteInt32( DoOnceTokenCount);
    out->WriteInt32( DisplayTextMinTimeMs);
    out->WriteInt32( DisplayTextIgnoreUserInputDelayMs);
    out->WriteInt32( IgnoreUserInputUntilTime);
    DefaultAudioTypeVolumes.WriteRaw(out);
}

void GameState::ReadQueuedAudioItems_Aligned(Common::Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int i = 0; i < MAX_QUEUED_MUSIC; ++i)
    {
        NewMusicQueue[i].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void GameState::WriteQueuedAudioItems_Aligned(Common::Stream *out)
{
    AlignedStream align_s(out, Common::kAligned_Write);
    for (int i = 0; i < MAX_QUEUED_MUSIC; ++i)
    {
        NewMusicQueue[i].WriteToFile(&align_s);
        align_s.Reset();
    }
}

} // namespace Engine
} // namespace AGS
