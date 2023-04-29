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

#include <stdio.h>
#include "ac/common.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/lipsync.h"
#include "ac/path_helper.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "game/roomstruct.h"
#include "main/engine.h"
#include "media/audio/audio_core.h"
#include "media/audio/audio_system.h"
#include "ac/timer.h"
#include "util/string_compat.h"

using namespace AGS::Common;

extern GameState play;
extern GameSetupStruct game;
extern RoomStruct thisroom;
extern std::vector<SpeechLipSyncLine> splipsync;
extern int numLipLines, curLipLine, curLipLinePhoneme;


int IsChannelPlaying(int chan) {
    if (play.fast_forward)
        return 0;

    if ((chan < 0) || (chan >= game.numGameChannels))
        quit("!IsChannelPlaying: invalid sound channel");

    if (AudioChans::ChannelIsPlaying(chan))
        return 1;

    return 0;
}

void SetSpeechVolume(int newvol) {
    if ((newvol<0) | (newvol>255))
        quit("!SetSpeechVolume: invalid volume - must be from 0-255");

    auto* ch = AudioChans::GetChannel(SCHAN_SPEECH);
    if (ch)
        ch->set_volume255(newvol);
    play.speech_volume = newvol;
}

void SetVoiceMode(int newmod)
{
    if ((newmod < kSpeech_First) | (newmod > kSpeech_Last))
        quitprintf("!SetVoiceMode: invalid mode number %d", newmod);
    play.speech_mode = (SpeechMode)newmod;
}

int GetVoiceMode()
{
    return (int)play.speech_mode;
}

int IsVoxAvailable() {
    return play.voice_avail ? 1 : 0;
}

int IsMusicVoxAvailable () {
    return play.separate_music_lib ? 1 : 0;
}

extern ScriptAudioChannel scrAudioChannel[MAX_GAME_CHANNELS];

ScriptAudioChannel *PlayVoiceClip(CharacterInfo *ch, int sndid, bool as_speech)
{
    if (!play_voice_nonblocking(ch->index_id, sndid, as_speech))
        return NULL;
    return &scrAudioChannel[SCHAN_SPEECH];
}

// Construct an asset name for the voice-over clip for the given character and cue id
String get_cue_filename(int charid, int sndid)
{
    String asset_path = get_voice_assetpath();
    String script_name;
    if (charid >= 0)
    {
        // append the first 4 characters of the script name to the filename
        if (game.chars[charid].scrname[0] == 'c')
            script_name.SetString(&game.chars[charid].scrname[1], 4);
        else
            script_name.SetString(game.chars[charid].scrname, 4);
    }
    else
    {
        script_name = "NARR";
    }
    return String::FromFormat("%s%s%d", asset_path.GetCStr(), script_name.GetCStr(), sndid);
}

// Play voice-over clip on the common channel;
// voice_name should be bare clip name without extension
static bool play_voice_clip_on_channel(const String &voice_name)
{
    stop_and_destroy_channel(SCHAN_SPEECH);

    String asset_name = voice_name;
    asset_name.Append(".wav");
    SOUNDCLIP *speechmp3 = my_load_wave(get_voice_over_assetpath(asset_name), false);

    if (speechmp3 == nullptr) {
        asset_name.ReplaceMid(asset_name.GetLength() - 3, 3, "ogg");
        speechmp3 = my_load_ogg(get_voice_over_assetpath(asset_name), false);
    }

    if (speechmp3 == nullptr) {
        asset_name.ReplaceMid(asset_name.GetLength() - 3, 3, "mp3");
        speechmp3 = my_load_mp3(get_voice_over_assetpath(asset_name), false);
    }

    if (speechmp3 == nullptr) {
        asset_name.ReplaceMid(asset_name.GetLength() - 3, 3, "flac");
        speechmp3 = my_load_flac(get_voice_over_assetpath(asset_name), false);
    }

    if (speechmp3 != nullptr) {
        speechmp3->set_volume255(play.speech_volume);
        if (!speechmp3->play()) {
            // not assigned to a channel, so clean up manually.
            delete speechmp3;
            speechmp3 = nullptr;
        }
    }

    if (speechmp3 == nullptr) {
        debug_script_warn("Speech load failure: '%s'", voice_name.GetCStr());
        return false;
    }

    AudioChans::SetChannel(SCHAN_SPEECH, std::unique_ptr<SOUNDCLIP>(speechmp3));
    return true;
}

// Play voice-over clip and adjust audio volumes;
// voice_name should be bare clip name without extension
static bool play_voice_clip_impl(const String &voice_name, bool as_speech, bool is_blocking)
{
    if (!play_voice_clip_on_channel(voice_name))
        return false;
    if (!as_speech)
        return true;

    play.speech_has_voice = true;
    play.speech_voice_blocking = is_blocking;

    cancel_scheduled_music_update();
    apply_volume_drop_modifier(true);
    return true;
}

// Stop voice-over clip and schedule audio volume reset
static void stop_voice_clip_impl()
{
    // update the music in a bit (fixes two speeches follow each other
    // and music going up-then-down)
    schedule_music_update_at(AGS_Clock::now() + std::chrono::milliseconds(500));
    stop_and_destroy_channel(SCHAN_SPEECH);
}

bool play_voice_speech(int charid, int sndid)
{
    // don't play speech if we're skipping a cutscene
    if (!play.ShouldPlayVoiceSpeech())
        return false;

    String voice_file = get_cue_filename(charid, sndid);
    if (!play_voice_clip_impl(voice_file, true, true))
        return false;

    int ii;  // Compare the base file name to the .pam file name
    curLipLine = -1;  // See if we have voice lip sync for this line
    curLipLinePhoneme = -1;
    for (ii = 0; ii < numLipLines; ii++) {
        if (voice_file.CompareNoCase(splipsync[ii].filename) == 0) {
            curLipLine = ii;
            break;
        }
    }
    // if the lip-sync is being used for voice sync, disable
    // the text-related lipsync
    if (numLipLines > 0)
        game.options[OPT_LIPSYNCTEXT] = 0;

    // change Sierra w/bgrnd  to Sierra without background when voice
    // is available (for Tierra)
    if ((game.options[OPT_SPEECHTYPE] == 2) && (play.no_textbg_when_voice > 0)) {
        game.options[OPT_SPEECHTYPE] = 1;
        play.no_textbg_when_voice = 2;
    }
    return true;
}

bool play_voice_nonblocking(int charid, int sndid, bool as_speech)
{
    // don't play voice if we're skipping a cutscene
    if (!play.ShouldPlayVoiceSpeech())
        return false;
    // don't play voice if there's a blocking speech with voice-over already
    if (play.IsBlockingVoiceSpeech())
        return false;

    String voice_file = get_cue_filename(charid, sndid);
    return play_voice_clip_impl(voice_file, as_speech, false);
}

void stop_voice_speech()
{
    if (!play.speech_has_voice)
        return;

    stop_voice_clip_impl();

    // Reset lipsync
    curLipLine = -1;
    // Set back to Sierra w/bgrnd
    if (play.no_textbg_when_voice == 2)
    {
        play.no_textbg_when_voice = 1;
        game.options[OPT_SPEECHTYPE] = 2;
    }
    play.speech_has_voice = false;
    play.speech_voice_blocking = false;
}

void stop_voice_nonblocking()
{
    if (!play.speech_has_voice)
        return;
    stop_voice_clip_impl();
    // Only reset speech flags if we are truly playing a non-blocking voice;
    // otherwise we might be inside blocking speech function and should let
    // it keep these flags to be able to finalize properly.
    // This is an imperfection of current speech implementation.
    if (!play.speech_voice_blocking)
    {
        play.speech_has_voice = false;
        play.speech_voice_blocking = false;
    }
}
