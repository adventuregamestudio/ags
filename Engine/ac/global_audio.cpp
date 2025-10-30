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
#include "util/string_compat.h"
#include "util/time_util.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern RoomStruct thisroom;
extern std::vector<SpeechLipSyncLine> splipsync;
extern int numLipLines, curLipLine, curLipLinePhoneme;

void StopAmbientSound (int channel) {
    if (ambient[channel].channel == AUDIO_CHANNEL_UNDEFINED)
        return;

    stop_and_destroy_channel(channel);
    ambient[channel].channel = AUDIO_CHANNEL_UNDEFINED;
}

void PlayAmbientSound (int channel, int sndnum, int vol, int x, int y) {
    // the channel parameter is to allow multiple ambient sounds in future
    if ((channel < 1) || (channel == LEGACY_AUDIO_CHAN_SPEECH) || (channel >= game.numGameChannels))
        quit("!PlayAmbientSound: invalid channel number");
    if ((vol < 1) || (vol > 255))
        quit("!PlayAmbientSound: volume must be 1 to 255");

    ScriptAudioClip *aclip = GetAudioClipForOldStyleNumber(game, false, sndnum);
    if (aclip && !is_audiotype_allowed_to_play((AudioFileType)aclip->fileType))
        return;

    // only play the sound if it's not already playing
    if ((ambient[channel].channel < 0) || (!AudioChans::ChannelIsPlaying(ambient[channel].channel)) ||
        (ambient[channel].num != sndnum)) {

            StopAmbientSound(channel);
            // in case a normal non-ambient sound was playing, stop it too
            stop_and_destroy_channel(channel);

            std::unique_ptr<SoundClip> asound = aclip ? load_sound_and_play(aclip, true) : nullptr;
            if (asound == nullptr) {
                debug_script_warn ("Cannot load ambient sound %d", sndnum);
                debug_script_log("FAILED to load ambient sound %d", sndnum);
                return;
            }

            debug_script_log("Playing ambient sound %d on channel %d", sndnum, channel);
            ambient[channel].channel = channel;
            asound->priority = 15;  // ambient sound higher priority than normal sfx
            AudioChans::SetChannel(channel, std::move(asound));
    }
    // calculate the maximum distance away the player can be, using X
    // only (since X centred is still more-or-less total Y)
    ambient[channel].maxdist = ((x > thisroom.Width / 2) ? x : (thisroom.Width - x)) - AMBIENCE_FULL_DIST;
    ambient[channel].num = sndnum;
    ambient[channel].x = x;
    ambient[channel].y = y;
    ambient[channel].vol = vol;
    update_ambient_sound_vol();
}

int IsChannelPlaying(int chan) {
    if (play.fast_forward)
        return 0;

    if ((chan < 0) || (chan >= game.numGameChannels))
        quit("!IsChannelPlaying: invalid sound channel");

    if (AudioChans::ChannelIsPlaying(chan))
        return 1;

    return 0;
}

int IsSoundPlaying() {
    if (play.fast_forward)
        return 0;

    // find if there's a sound playing
    for (int i = LEGACY_AUDIO_CHAN_NORMAL; i < game.numGameChannels; i++) {
        if (AudioChans::GetChannelIfPlaying(i))
            return 1;
    }

    return 0;
}

// returns -1 on failure, channel number on success
int PlaySoundEx(int val1, int channel) {

    if (debug_flags & DBG_NOSFX)
        return -1;

    ScriptAudioClip *aclip = GetAudioClipForOldStyleNumber(game, false, val1);
    if (aclip && !is_audiotype_allowed_to_play((AudioFileType)aclip->fileType))
        return -1; // if sound is off, ignore it

    if ((channel < LEGACY_AUDIO_CHAN_NORMAL) || (channel >= game.numGameChannels))
        quitprintf("!PlaySoundEx: invalid channel specified, must be %d-%d", LEGACY_AUDIO_CHAN_NORMAL, game.numGameChannels - 1);

    // if an ambient sound is playing on this channel, abort it
    StopAmbientSound(channel);

    if (val1 < 0) {
        stop_and_destroy_channel (channel);
        return -1;
    }
    // if skipping a cutscene, don't try and play the sound
    if (play.fast_forward)
        return -1;

    // free the old sound
    stop_and_destroy_channel (channel);
    debug_script_log("Playing sound %d on channel %d", val1, channel);

    std::unique_ptr<SoundClip> soundfx = aclip ? load_sound_and_play(aclip, false) : nullptr;
    if (soundfx == nullptr) {
        debug_script_warn("Sound sample load failure: cannot load sound %d", val1);
        debug_script_log("FAILED to load sound %d", val1);
        return -1;
    }

    soundfx->priority = 10;
    soundfx->set_volume255(play.sound_volume);
    AudioChans::SetChannel(channel, std::move(soundfx));
    return channel;
}

void StopAllSounds(int evenAmbient) {
    // backwards-compatible hack -- stop Type 3 (default Sound Type)
    Game_StopAudio(3);

    if (evenAmbient)
        Game_StopAudio(1);
}

void PlayMusicResetQueue(int newmus) {
    play.music_queue_size = 0;
    newmusic(newmus);
}

void SeekMIDIPosition (int position) {
    if (play.silent_midi == 0 && current_music_type != MUS_MIDI)
        return;

    auto *ch = AudioChans::GetChannel(play.silent_midi == 0 ? LEGACY_AUDIO_CHAN_MUSIC : play.silent_midi_channel);
    if (ch)
    {
        ch->seek(position);
        debug_script_log("Seek MIDI position to %d", position);
    }
}

int GetMIDIPosition () {
    if (play.fast_forward)
        return 99999;
    if (play.silent_midi == 0 && current_music_type != MUS_MIDI)
        return -1; // returns -1 on failure according to old manuals
    
    auto* ch = AudioChans::GetChannelIfPlaying(play.silent_midi == 0 ? LEGACY_AUDIO_CHAN_MUSIC : play.silent_midi_channel);
    if (ch) {
        return ch->get_pos();
    }

    return -1;
}

int IsMusicPlaying() {
    // in case they have a "while (IsMusicPlaying())" loop
    if ((play.fast_forward) && (play.skip_until_char_stops < 0))
        return 0;

    // This only returns positive if there was a music started by old audio API
    if (current_music_type == 0)
        return 0;

    auto *ch = AudioChans::GetChannel(LEGACY_AUDIO_CHAN_MUSIC);
    if (ch == nullptr)
    { // This was probably a hacky fix in case it was not reset by game update; TODO: find out if needed
        current_music_type = 0;
        return 0;
    }

    bool result = (ch->is_ready()) || (crossFading > 0 && (AudioChans::GetChannelIfPlaying(crossFading) != nullptr));
    return result ? 1 : 0;
}

int PlayMusicQueued(int musnum) {

    // Just get the queue size
    if (musnum < 0)
        return play.music_queue_size;

    if ((IsMusicPlaying() == 0) && (play.music_queue_size == 0)) {
        newmusic(musnum);
        return 0;
    }

    if (play.music_queue_size >= MAX_QUEUED_MUSIC) {
        debug_script_log("Too many queued music, cannot add %d", musnum);
        return 0;
    }

    if ((play.music_queue_size > 0) && 
        (play.music_queue[play.music_queue_size - 1] >= QUEUED_MUSIC_REPEAT)) {
            debug_script_warn("PlayMusicQueued: cannot queue music after a repeating tune has been queued");
            return 0;
    }

    if (play.music_repeat) {
        debug_script_log("Queuing music %d to loop", musnum);
        musnum += QUEUED_MUSIC_REPEAT;
    }
    else {
        debug_script_log("Queuing music %d", musnum);
    }

    play.music_queue[play.music_queue_size] = musnum;
    play.music_queue_size++;

    if (play.music_queue_size == 1) {

        clear_music_cache();

        cachedQueuedMusic = load_music_from_disk(musnum, (play.music_repeat > 0));
    }

    return play.music_queue_size;
}

void scr_StopMusic() {
    play.music_queue_size = 0;
    stopmusic();
}

void SeekMODPattern(int patnum) {
    if (current_music_type != MUS_MOD)
        return;

    auto* ch = AudioChans::GetChannelIfPlaying(LEGACY_AUDIO_CHAN_MUSIC);
    if (ch) {
        ch->seek (patnum);
        debug_script_log("Seek MOD/XM to pattern %d", patnum);
    }
}

void SeekMP3PosMillis (int posn) {
    if (current_music_type != MUS_MP3 && current_music_type != MUS_OGG)
        return;

    auto *mus_ch = AudioChans::GetChannel(LEGACY_AUDIO_CHAN_MUSIC);
    auto *cf_ch = (crossFading > 0) ? AudioChans::GetChannel(crossFading) : nullptr;
    if (cf_ch)
        cf_ch->seek(posn);
    else if (mus_ch)
        mus_ch->seek(posn);
}

int GetMP3PosMillis () {
    // in case they have "while (GetMP3PosMillis() < 5000) "
    if (play.fast_forward)
        return 999999;
    if (current_music_type != MUS_MP3 && current_music_type != MUS_OGG)
        return 0;  // returns 0 on failure according to old manuals

    auto* ch = AudioChans::GetChannelIfPlaying(LEGACY_AUDIO_CHAN_MUSIC);
    if (ch) {
        int result = ch->get_pos_ms();
        if (result >= 0)
            return result;

        return ch->get_pos ();
    }

    return 0;
}

void SetMusicVolume(int newvol) {
    if ((newvol < kRoomVolumeMin) || (newvol > kRoomVolumeMax))
        quitprintf("!SetMusicVolume: invalid volume number. Must be from %d to %d.", kRoomVolumeMin, kRoomVolumeMax);
    thisroom.Options.MusicVolume=(RoomVolumeMod)newvol;
    update_music_volume();
}

void SetMusicMasterVolume(int newvol) {
    const int min_volume = loaded_game_file_version < kGameVersion_330 ? 0 :
        -LegacyMusicMasterVolumeAdjustment - (kRoomVolumeMax * LegacyRoomVolumeFactor);
    if ((newvol < min_volume) | (newvol>100))
        quitprintf("!SetMusicMasterVolume: invalid volume - must be from %d to %d", min_volume, 100);
    play.music_master_volume=newvol+LegacyMusicMasterVolumeAdjustment;
    update_music_volume();
}

void SetSoundVolume(int newvol) {
    if ((newvol<0) | (newvol>255))
        quit("!SetSoundVolume: invalid volume - must be from 0-255");
    play.sound_volume = newvol;
    Game_SetAudioTypeVolume(LEGACY_AUDIOTYPE_AMBIENT_SOUND, (newvol * 100) / 255, VOL_BOTH);
    Game_SetAudioTypeVolume(LEGACY_AUDIOTYPE_SOUND, (newvol * 100) / 255, VOL_BOTH);
    update_ambient_sound_vol ();
}

void SetChannelVolume(int chan, int newvol) {
    if ((newvol<0) || (newvol>255))
        quit("!SetChannelVolume: invalid volume - must be from 0-255");
    if ((chan < 0) || (chan >= game.numGameChannels))
        quit("!SetChannelVolume: invalid channel id");

    auto* ch = AudioChans::GetChannelIfPlaying(chan);

    if (ch) {
        if (chan == ambient[chan].channel) {
            ambient[chan].vol = newvol;
            update_ambient_sound_vol();
        }
        else
            ch->set_volume255(newvol);
    }
}

void SetDigitalMasterVolume (int newvol) {
    if ((newvol<0) | (newvol>100))
        quit("!SetDigitalMasterVolume: invalid volume - must be from 0-100");
    play.digital_master_volume = newvol;
    auto newvol_f = static_cast<float>(newvol) / 100.0;
    audio_core_set_master_volume(newvol_f);
}

int GetCurrentMusic() {
    return play.cur_music_number;
}

void SetMusicRepeat(int loopflag) {
    play.music_repeat=loopflag;
}

void PlayMP3File (const char *filename)
{
    debug_script_log("PlayMP3File %s", filename);

    AssetPath asset_name(filename, "audio");
    const bool do_loop = (play.music_repeat > 0);
    std::unique_ptr<SoundClip> clip(load_sound_clip(asset_name, "", do_loop));
    if (!clip)
    {
        debug_script_warn("PlayMP3File: music file '%s' not found or cannot be read", filename);
        return;
    }

    const int use_chan = prepare_for_new_music();
    current_music_type = clip->soundType;
    play.cur_music_number = 1000;
    play.playmp3file_name = filename;
    clip->set_volume255(150); // CHECKME: why 150?...
    clip->play();
    AudioChans::SetChannel(use_chan, std::move(clip));
    post_new_music_check();
    update_music_volume();
}

void PlaySilentMIDI (int mnum) {
    if (current_music_type == MUS_MIDI)
        quit("!PlaySilentMIDI: proper midi music is in progress");

    const int silent_midi_chan = LEGACY_AUDIO_CHAN_SPEECH;
    stop_and_destroy_channel(silent_midi_chan);

    std::unique_ptr<SoundClip> clip = load_sound_clip_from_old_style_number(true, mnum, false);
    if (clip == nullptr)
    {
        debug_script_warn("PlaySilentMIDI: music file 'aMusic%d' not found or cannot be read", mnum);
        return;
    }
    
    clip->set_volume100(0);
    clip->play();
    AudioChans::SetChannel(silent_midi_chan, std::move(clip));
    play.silent_midi = mnum;
    play.silent_midi_channel = silent_midi_chan;
}

void SetSpeechVolume(int newvol) {
    if ((newvol<0) | (newvol>255))
        quit("!SetSpeechVolume: invalid volume - must be from 0-255");

    Game_SetAudioTypeVolume(AUDIO_CLIP_TYPE_SPEECH, Math::Range255To100(newvol), VOL_BOTH);
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

// Construct an asset name for the voice-over clip for the given character and cue id
static String get_cue_filename(int charid, int sndid, bool old_style)
{
    // Clip name generation rule:
    // Cut a small case 'c' prefix from the character's script name,
    // this is a ugly hack, but is still done, because in AGS
    // characters were traditionally named as 'cEgo'.
    // New-style: use full script name (past the prefix),
    //            clip number (X) is separated by a dot:
    //            "CHARNAME.X"
    // Old-style: use only first 4 characters (past the prefix).
    //            clip number (X) is not separated:
    //            "CHARX"
    const char *charname = (charid >= 0) ? game.chars2[charid].scrname_new.GetCStr()
        : "narrator";
    size_t from = (charname[0] == 'c') ? 1 : 0u;
    size_t len = old_style ? 4 : SIZE_MAX;
    String charname_fix(charname + from, len);
    
    const char *fmt_str = old_style ? "%s%d" : "%s.%d";
    String asset_filename = String::FromFormat(fmt_str, charname_fix.GetCStr(), sndid);
    return Path::ConcatPaths(get_voice_assetpath(), asset_filename);
}

// Play voice-over clip on the common channel;
// voice_name should be bare clip name without extension
static ScriptAudioChannel *play_voice_clip_on_any_channel(const String &voice_name, int priority = SCR_NO_VALUE, int repeat = SCR_NO_VALUE)
{
    // TODO: perhaps a better algorithm, allow any extension / sound format?
    // e.g. make a hashmap matching a voice name to a asset name
    std::array<const char*, 3> exts = {{ "mp3", "ogg", "wav" }};
    AssetPath apath = get_voice_over_assetpath(voice_name);
    bool found = false;
    for (auto *ext : exts)
    {
        apath.Name.Format("%s.%s", voice_name.GetCStr(), ext);
        found = AssetMgr->DoesAssetExist(apath);
        if (found)
            break;
    }

    if (!found) {
        debug_script_warn("Speech file not found: '%s'", voice_name.GetCStr());
        return nullptr;
    }

    ScriptAudioClip clip(AUDIO_CLIP_TYPE_SPEECH, "", apath.Name, kAudioBundle_SpeechVox);
    return play_audio_clip(&clip, priority, repeat, 0, false);
}

// Play voice-over clip and adjust audio volumes;
// voice_name should be bare clip name without extension
static ScriptAudioChannel *play_voice_clip_impl(const String &voice_name, bool as_speech, bool is_blocking,
    int priority = SCR_NO_VALUE, int repeat = SCR_NO_VALUE)
{
    ScriptAudioChannel *achan = play_voice_clip_on_any_channel(voice_name, priority, repeat);
    if (!achan)
        return nullptr;

    if (as_speech)
    {
        // Mark the playback as "speech", so that it caused audio volume drop,
        // and any other effects meant to happen when the speech voice is playing.
        // There can be only single blocking speech at the same time, save its channel id
        if (is_blocking)
            play.speech_blocking_voice_chan = achan->id;
        play.voice_chan_as_speech[achan->id] = true;
        update_voice_state();
    }
    return achan;
}

// Reset the game state in case blocking voice speech has stopped
static void on_blocking_voice_stop()
{
    // Reset lipsync
    curLipLine = -1;
    // Set back to Sierra w/bgrnd
    if (play.no_textbg_when_voice == 2)
    {
        play.no_textbg_when_voice = 1;
        game.options[OPT_SPEECHTYPE] = kSpeechStyle_SierraBackground;
    }
    play.speech_blocking_voice_chan = AUDIO_CHANNEL_UNDEFINED;
}

// Check if there's no active voice-over clips, and schedule audio volume reset
void update_voice_state()
{
    int speech_voice_count = 0;
    // Here we assume that the speech audio type is under fixed index 0,
    // and therefore all its reserved channels are first in the channel list
    for (int i = 0; i < game.audioClipTypes[AUDIO_CLIP_TYPE_SPEECH].reservedChannels; ++i)
    {
        if (AudioChans::ChannelIsPlaying(i))
        {
            if (play.voice_chan_as_speech[i])
                speech_voice_count++;
        }
        else
        {
            // If the blocking speech's playback has stopped for any reason,
            // then update the respective game state
            if (play.speech_blocking_voice_chan == i)
                on_blocking_voice_stop();

            play.voice_chan_as_speech[i] = false;
        }
    }

    // Handle general voice-over playbacks state
    if (play.speech_voice_count == 0 && speech_voice_count > 0)
    {
        // First voice-over started: apply a volume drop
        cancel_scheduled_music_update();
        play.music_vol_was = play.music_master_volume;
        // Negative value means set exactly; positive means drop that amount
        if (play.speech_music_drop < 0)
            play.music_master_volume = -play.speech_music_drop;
        else
            play.music_master_volume -= play.speech_music_drop;
        apply_volume_drop_modifier(true);
        update_music_volume();
        update_ambient_sound_vol();
    }
    else if (play.speech_voice_count > 0 && speech_voice_count == 0)
    {
        // Last voice-over have stopped:
        // reset settings back and schedule audio volume update
        play.music_master_volume = play.music_vol_was;
        // update the music in a bit (fixes two speeches follow each other
        // and music going up-then-down)
        // FIXME: don't use a hardcoded number, make at least an internal constant or variable
        schedule_music_update_at(Clock::now() + std::chrono::milliseconds(500));
    }

    play.speech_voice_count = speech_voice_count;
}

bool play_voice_speech(int charid, int sndid)
{
    // don't play speech if we're skipping a cutscene
    if (!play.ShouldPlayVoiceSpeech())
        return false;

    String voice_file = get_cue_filename(charid, sndid, !game.options[OPT_VOICECLIPNAMERULE]);
    ScriptAudioChannel *achan = play_voice_clip_impl(voice_file, true, true);
    if (!achan)
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
    if ((game.options[OPT_SPEECHTYPE] == kSpeechStyle_SierraBackground) && (play.no_textbg_when_voice > 0)) {
        game.options[OPT_SPEECHTYPE] = kSpeechStyle_SierraTransparent;
        play.no_textbg_when_voice = 2;
    }
    return true;
}

ScriptAudioChannel *play_voice_nonblocking(int charid, int sndid, bool as_speech, int priority, int repeat)
{
    // don't play voice if we're skipping a cutscene
    if (!play.ShouldPlayVoiceSpeech())
        return nullptr;
    // don't play voice if there's a blocking speech with voice-over already
    if (play.IsBlockingVoiceSpeech())
        return nullptr;

    String voice_file = get_cue_filename(charid, sndid, !game.options[OPT_VOICECLIPNAMERULE]);
    return play_voice_clip_impl(voice_file, as_speech, false);
}

void stop_blocking_voice_speech()
{
    if (play.speech_blocking_voice_chan == AUDIO_CHANNEL_UNDEFINED)
        return;

    // Note we call update_voice_state() and on_blocking_voice_stop() inside stop_and_destroy_channel()
    stop_and_destroy_channel(play.speech_blocking_voice_chan);
}
