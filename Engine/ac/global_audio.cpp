//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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
#include "ac/timer.h"
#include "util/string_compat.h"

using namespace AGS::Common;

extern GameState play;
extern GameSetupStruct game;
extern RoomStruct thisroom;
extern std::vector<SpeechLipSyncLine> splipsync;
extern int numLipLines, curLipLine, curLipLinePhoneme;

void StopAmbientSound (int channel) {
    if ((channel < NUM_SPEECH_CHANS) || (channel >= game.numGameChannels))
        quitprintf("!StopAmbientSound: invalid channel %d, supported %d - %d",
            channel, NUM_SPEECH_CHANS, MAX_GAME_CHANNELS - 1);

    if (ambient[channel].channel == 0)
        return;

    stop_and_destroy_channel(channel);
    ambient[channel].channel = 0;
}

void PlayAmbientSound (int channel, int sndnum, int vol, int x, int y) {
    // the channel parameter is to allow multiple ambient sounds in future
    if ((channel < 1) || (channel == SCHAN_SPEECH) || (channel >= game.numGameChannels))
        quit("!PlayAmbientSound: invalid channel number");
    if ((vol < 1) || (vol > 255))
        quit("!PlayAmbientSound: volume must be 1 to 255");

    ScriptAudioClip *aclip = GetAudioClipForOldStyleNumber(game, false, sndnum);
    if (aclip && !is_audiotype_allowed_to_play((AudioFileType)aclip->fileType))
        return;

    // only play the sound if it's not already playing
    if ((ambient[channel].channel < 1) || (!AudioChans::ChannelIsPlaying(ambient[channel].channel)) ||
        (ambient[channel].num != sndnum)) {

            StopAmbientSound(channel);
            // in case a normal non-ambient sound was playing, stop it too
            stop_and_destroy_channel(channel);

            SOUNDCLIP *asound = aclip ? load_sound_and_play(aclip, true) : nullptr;
            if (asound == nullptr) {
                debug_script_warn ("Cannot load ambient sound %d", sndnum);
                debug_script_log("FAILED to load ambient sound %d", sndnum);
                return;
            }

            debug_script_log("Playing ambient sound %d on channel %d", sndnum, channel);
            ambient[channel].channel = channel;
            asound->priority = 15;  // ambient sound higher priority than normal sfx
            AudioChans::SetChannel(channel, std::unique_ptr<SOUNDCLIP>(asound));
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
    for (int i = SCHAN_NORMAL; i < game.numGameChannels; i++) {
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

    if ((channel < SCHAN_NORMAL) || (channel >= game.numGameChannels))
        quitprintf("!PlaySoundEx: invalid channel specified, must be %d-%d", SCHAN_NORMAL, game.numGameChannels - 1);

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

    SOUNDCLIP *soundfx = aclip ? load_sound_and_play(aclip, false) : nullptr;
    if (soundfx == nullptr) {
        debug_script_warn("Sound sample load failure: cannot load sound %d", val1);
        debug_script_log("FAILED to load sound %d", val1);
        return -1;
    }

    soundfx->priority = 10;
    soundfx->set_volume255(play.sound_volume);
    AudioChans::SetChannel(channel, std::unique_ptr<SOUNDCLIP>(soundfx));
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

    auto *ch = AudioChans::GetChannel(SCHAN_MUSIC);
    ch->seek(position);
    debug_script_log("Seek MIDI position to %d", position);
}

int GetMIDIPosition () {
    if (play.fast_forward)
        return 99999;
    if (play.silent_midi == 0 && current_music_type != MUS_MIDI)
        return -1; // returns -1 on failure according to old manuals
    
    auto* ch = AudioChans::GetChannelIfPlaying(SCHAN_MUSIC);
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

    auto *ch = AudioChans::GetChannel(SCHAN_MUSIC);
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

    auto* ch = AudioChans::GetChannelIfPlaying(SCHAN_MUSIC);
    if (ch) {
        ch->seek (patnum);
        debug_script_log("Seek MOD/XM to pattern %d", patnum);
    }
}

void SeekMP3PosMillis (int posn) {
    if (current_music_type != MUS_MP3 && current_music_type != MUS_OGG)
        return;

    auto *mus_ch = AudioChans::GetChannel(SCHAN_MUSIC);
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

    auto* ch = AudioChans::GetChannelIfPlaying(SCHAN_MUSIC);
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
    Game_SetAudioTypeVolume(AUDIOTYPE_LEGACY_AMBIENT_SOUND, (newvol * 100) / 255, VOL_BOTH);
    Game_SetAudioTypeVolume(AUDIOTYPE_LEGACY_SOUND, (newvol * 100) / 255, VOL_BOTH);
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
    std::unique_ptr<SOUNDCLIP> clip(load_sound_clip(asset_name, "", do_loop));
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

    play.silent_midi = mnum;
    play.silent_midi_channel = SCHAN_SPEECH;
    stop_and_destroy_channel(play.silent_midi_channel);
    // No idea why it uses speech voice channel, but since it does (and until this is changed)
    // we have to correctly reset speech voice in case there was a nonblocking speech
    if (play.IsNonBlockingVoiceSpeech())
        stop_voice_nonblocking();

    SOUNDCLIP *clip = load_sound_clip_from_old_style_number(true, mnum, false);
    if (clip == nullptr)
    {
        quitprintf("!PlaySilentMIDI: failed to load aMusic%d", mnum);
    }
    if (clip->play()) {
        AudioChans::SetChannel(play.silent_midi_channel, std::unique_ptr<SOUNDCLIP>(clip));
        clip->set_volume100(0);
    } else {
        delete clip;
        quitprintf("!PlaySilentMIDI: failed to play aMusic%d", mnum);
    }
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
        if (game.chars2[charid].scrname_new[0] == 'c')
            script_name.SetString(game.chars2[charid].scrname_new.GetCStr() + 1, 4);
        else
            script_name.SetString(game.chars2[charid].scrname_new.GetCStr(), 4);
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
        return false;
    }

    std::unique_ptr<SOUNDCLIP> voice_clip(load_sound_clip(apath, "", false));
    if (voice_clip != nullptr) {
        voice_clip->set_volume255(play.speech_volume);
        if (!voice_clip->play())
            voice_clip.reset();
    }

    if (!voice_clip) {
        debug_script_warn("Speech load failure: '%s'", voice_name.GetCStr());
        return false;
    }

    AudioChans::SetChannel(SCHAN_SPEECH, std::move(voice_clip));
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
    play.music_vol_was = play.music_master_volume;
    // Negative value means set exactly; positive means drop that amount
    if (play.speech_music_drop < 0)
        play.music_master_volume = -play.speech_music_drop;
    else
        play.music_master_volume -= play.speech_music_drop;
    apply_volume_drop_modifier(true);
    update_music_volume();
    update_ambient_sound_vol();
    return true;
}

// Stop voice-over clip and schedule audio volume reset
static void stop_voice_clip_impl()
{
    play.music_master_volume = play.music_vol_was;
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
