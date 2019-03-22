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
#include "media/audio/audio_system.h"

using namespace AGS::Common;

extern GameSetup usetup;
extern GameState play;
extern GameSetupStruct game;
extern RoomStruct thisroom;
extern SpeechLipSyncLine *splipsync;
extern int numLipLines, curLipLine, curLipLinePhoneme;

void StopAmbientSound (int channel) {
    if ((channel < 0) || (channel >= MAX_SOUND_CHANNELS))
        quit("!StopAmbientSound: invalid channel");

    if (ambient[channel].channel == 0)
        return;

    stop_and_destroy_channel(channel);
    ambient[channel].channel = 0;
}

void PlayAmbientSound (int channel, int sndnum, int vol, int x, int y) {
    // the channel parameter is to allow multiple ambient sounds in future
    if ((channel < 1) || (channel == SCHAN_SPEECH) || (channel >= MAX_SOUND_CHANNELS))
        quit("!PlayAmbientSound: invalid channel number");
    if ((vol < 1) || (vol > 255))
        quit("!PlayAmbientSound: volume must be 1 to 255");

    ScriptAudioClip *aclip = GetAudioClipForOldStyleNumber(game, false, sndnum);
    if (aclip && !is_audiotype_allowed_to_play((AudioFileType)aclip->fileType))
        return;

    // only play the sound if it's not already playing
    if ((ambient[channel].channel < 1) || (channels[ambient[channel].channel] == NULL) ||
        (channels[ambient[channel].channel]->done == 1) ||
        (ambient[channel].num != sndnum)) {

            StopAmbientSound(channel);
            // in case a normal non-ambient sound was playing, stop it too
            stop_and_destroy_channel(channel);

            SOUNDCLIP *asound = aclip ? load_sound_and_play(aclip, true) : NULL;
            if (asound == NULL) {
                debug_script_warn ("Cannot load ambient sound %d", sndnum);
                debug_script_log("FAILED to load ambient sound %d", sndnum);
                return;
            }

            debug_script_log("Playing ambient sound %d on channel %d", sndnum, channel);
            ambient[channel].channel = channel;
            channels[channel] = asound;
            channels[channel]->priority = 15;  // ambient sound higher priority than normal sfx
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

    if ((chan < 0) || (chan >= MAX_SOUND_CHANNELS))
        quit("!IsChannelPlaying: invalid sound channel");

    if ((channels[chan] != NULL) && (channels[chan]->done == 0))
        return 1;

    return 0;
}

int IsSoundPlaying() {
    if (play.fast_forward)
        return 0;

    // find if there's a sound playing
    for (int i = SCHAN_NORMAL; i < numSoundChannels; i++) {
        if ((channels[i] != NULL) && (channels[i]->done == 0))
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

    if ((channel < SCHAN_NORMAL) || (channel >= MAX_SOUND_CHANNELS))
        quit("!PlaySoundEx: invalid channel specified, must be 3-7");

    // if an ambient sound is playing on this channel, abort it
    StopAmbientSound(channel);

    if (val1 < 0) {
        stop_and_destroy_channel (channel);
        return -1;
    }
    // if skipping a cutscene, don't try and play the sound
    if (play.fast_forward)
        return -1;

    // that sound is already in memory, play it
    if (!psp_audio_multithreaded)
    {
        if ((last_sound_played[channel] == val1) && (channels[channel] != NULL)) {
            debug_script_log("Playing sound %d on channel %d; cached", val1, channel);
            channels[channel]->restart();
            channels[channel]->set_volume (play.sound_volume);
            return channel;
        }
    }

    // free the old sound
    stop_and_destroy_channel (channel);
    debug_script_log("Playing sound %d on channel %d", val1, channel);

    last_sound_played[channel] = val1;

    SOUNDCLIP *soundfx = aclip ? load_sound_and_play(aclip, false) : NULL;
    if (soundfx == NULL) {
        debug_script_warn("Sound sample load failure: cannot load sound %d", val1);
        debug_script_log("FAILED to load sound %d", val1);
        return -1;
    }

    channels[channel] = soundfx;
    channels[channel]->priority = 10;
    channels[channel]->set_volume (play.sound_volume);
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
    if (play.silent_midi)
        midi_seek (position);
    if (current_music_type == MUS_MIDI) {
        midi_seek(position);
        debug_script_log("Seek MIDI position to %d", position);
    }
}

int GetMIDIPosition () {
    if (play.silent_midi)
        return midi_pos;
    if (current_music_type != MUS_MIDI)
        return -1;
    if (play.fast_forward)
        return 99999;

    return midi_pos;
}

int IsMusicPlaying() {
    // in case they have a "while (IsMusicPlaying())" loop
    if ((play.fast_forward) && (play.skip_until_char_stops < 0))
        return 0;

    if (current_music_type != 0) {
        if (channels[SCHAN_MUSIC] == NULL)
            current_music_type = 0;
        else if (channels[SCHAN_MUSIC]->done == 0)
            return 1;
        else if ((crossFading > 0) && (channels[crossFading] != NULL))
            return 1;
        return 0;
    }

    return 0;
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
    if (current_music_type == MUS_MOD && channels[SCHAN_MUSIC]) {
        channels[SCHAN_MUSIC]->seek (patnum);
        debug_script_log("Seek MOD/XM to pattern %d", patnum);
    }
}
void SeekMP3PosMillis (int posn) {
    if (current_music_type) {
        debug_script_log("Seek MP3/OGG to %d ms", posn);
        if (crossFading && channels[crossFading])
            channels[crossFading]->seek (posn);
        else if (channels[SCHAN_MUSIC])
            channels[SCHAN_MUSIC]->seek (posn);
    }
}

int GetMP3PosMillis () {
    // in case they have "while (GetMP3PosMillis() < 5000) "
    if (play.fast_forward)
        return 999999;

    if (current_music_type && channels[SCHAN_MUSIC]) {
        int result = channels[SCHAN_MUSIC]->get_pos_ms();
        if (result >= 0)
            return result;

        return channels[SCHAN_MUSIC]->get_pos ();
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
    if ((chan < 0) || (chan >= MAX_SOUND_CHANNELS))
        quit("!SetChannelVolume: invalid channel id");

    if ((channels[chan] != NULL) && (channels[chan]->done == 0)) {
        if (chan == ambient[chan].channel) {
            ambient[chan].vol = newvol;
            update_ambient_sound_vol();
        }
        else
            channels[chan]->set_volume (newvol);
    }
}

void SetDigitalMasterVolume (int newvol) {
    if ((newvol<0) | (newvol>100))
        quit("!SetDigitalMasterVolume: invalid volume - must be from 0-100");
    play.digital_master_volume = newvol;
    set_volume ((newvol * 255) / 100, -1);
}

int GetCurrentMusic() {
    return play.cur_music_number;
}

void SetMusicRepeat(int loopflag) {
    play.music_repeat=loopflag;
}

void PlayMP3File (const char *filename) {
    if (strlen(filename) >= PLAYMP3FILE_MAX_FILENAME_LEN)
        quit("!PlayMP3File: filename too long");

    debug_script_log("PlayMP3File %s", filename);

    AssetPath asset_name("", filename);

    int useChan = prepare_for_new_music ();
    bool doLoop = (play.music_repeat > 0);

    if ((channels[useChan] = my_load_static_ogg(asset_name, 150, doLoop)) != NULL) {
        channels[useChan]->play();
        current_music_type = MUS_OGG;
        play.cur_music_number = 1000;
        // save the filename (if it's not what we were supplied with)
        if (filename != &play.playmp3file_name[0])
            strcpy (play.playmp3file_name, filename);
    }
    else if ((channels[useChan] = my_load_static_mp3(asset_name, 150, doLoop)) != NULL) {
        channels[useChan]->play();
        current_music_type = MUS_MP3;
        play.cur_music_number = 1000;
        // save the filename (if it's not what we were supplied with)
        if (filename != &play.playmp3file_name[0])
            strcpy (play.playmp3file_name, filename);
    }
    else
        debug_script_warn ("PlayMP3File: file '%s' not found or cannot play", filename);

    post_new_music_check(useChan);

    update_music_volume();
}

void PlaySilentMIDI (int mnum) {
    if (current_music_type == MUS_MIDI)
        quit("!PlaySilentMIDI: proper midi music is in progress");

    set_volume (-1, 0);
    play.silent_midi = mnum;
    play.silent_midi_channel = SCHAN_SPEECH;
    stop_and_destroy_channel(play.silent_midi_channel);
    channels[play.silent_midi_channel] = load_sound_clip_from_old_style_number(true, mnum, false);
    if (channels[play.silent_midi_channel] == NULL)
    {
        quitprintf("!PlaySilentMIDI: failed to load aMusic%d", mnum);
    }
    channels[play.silent_midi_channel]->play();
    channels[play.silent_midi_channel]->set_volume_percent(0);
}

void SetSpeechVolume(int newvol) {
    if ((newvol<0) | (newvol>255))
        quit("!SetSpeechVolume: invalid volume - must be from 0-255");

    if (channels[SCHAN_SPEECH])
        channels[SCHAN_SPEECH]->set_volume (newvol);

    play.speech_volume = newvol;
}

void __scr_play_speech(int who, int which) {
    // *** implement this - needs to call stop_speech as well
    // to reset the volume
    quit("PlaySpeech not yet implemented");
}

// 0 = text only
// 1 = voice & text
// 2 = voice only
void SetVoiceMode (int newmod) {
    if ((newmod < 0) | (newmod > 2))
        quit("!SetVoiceMode: invalid mode number (must be 0,1,2)");
    // If speech is turned off, store the mode anyway in case the
    // user adds the VOX file later
    if (play.want_speech < 0)
        play.want_speech = (-newmod) - 1;
    else
        play.want_speech = newmod;
}

int GetVoiceMode()
{
    return play.want_speech >= 0 ? play.want_speech : -(play.want_speech + 1);
}

int IsVoxAvailable() {
    if (play.want_speech < 0)
        return 0;
    return 1;
}

int IsMusicVoxAvailable () {
    return play.separate_music_lib;
}

int play_speech(int charid,int sndid) {
    stop_and_destroy_channel (SCHAN_SPEECH);

    // don't play speech if we're skipping a cutscene
    if (play.fast_forward)
        return 0;
    if ((play.want_speech < 1) || (speech_file.IsEmpty()))
        return 0;

    SOUNDCLIP *speechmp3;
    String script_name;

    if (charid >= 0) {
        // append the first 4 characters of the script name to the filename
        if (game.chars[charid].scrname[0] == 'c')
            script_name.SetString(&game.chars[charid].scrname[1], 4);
        else
            script_name.SetString(game.chars[charid].scrname, 4);
    }
    else
        script_name = "NARR";

    // append the speech number and create voice file name
    String voice_file = String::FromFormat("%s%d", script_name.GetCStr(), sndid);

    int ii;  // Compare the base file name to the .pam file name
    curLipLine = -1;  // See if we have voice lip sync for this line
    curLipLinePhoneme = -1;
    for (ii = 0; ii < numLipLines; ii++) {
        if (stricmp(splipsync[ii].filename, voice_file) == 0) {
            curLipLine = ii;
            break;
        }
    }
    // if the lip-sync is being used for voice sync, disable
    // the text-related lipsync
    if (numLipLines > 0)
        game.options[OPT_LIPSYNCTEXT] = 0;

    String asset_name = voice_file;
    asset_name.Append(".wav");
    speechmp3 = my_load_wave(get_voice_over_assetpath(asset_name), play.speech_volume, 0);

    if (speechmp3 == NULL) {
        asset_name.ReplaceMid(asset_name.GetLength() - 3, 3, "ogg");
        speechmp3 = my_load_ogg(get_voice_over_assetpath(asset_name), play.speech_volume);
    }

    if (speechmp3 == NULL) {
        asset_name.ReplaceMid(asset_name.GetLength() - 3, 3, "mp3");
        speechmp3 = my_load_mp3(get_voice_over_assetpath(asset_name), play.speech_volume);
    }

    if (speechmp3 != NULL) {
        if (speechmp3->play() == 0)
            speechmp3 = NULL;
    }

    if (speechmp3 == NULL) {
        debug_script_warn("Speech load failure: '%s'", voice_file.GetCStr());
        curLipLine = -1;
        return 0;
    }

    channels[SCHAN_SPEECH] = speechmp3;
    play.music_vol_was = play.music_master_volume;

    // Negative value means set exactly; positive means drop that amount
    if (play.speech_music_drop < 0)
        play.music_master_volume = -play.speech_music_drop;
    else
        play.music_master_volume -= play.speech_music_drop;

    apply_volume_drop_modifier(true);
    update_music_volume();
    update_music_at = 0;
    mvolcounter = 0;

    update_ambient_sound_vol();

    // change Sierra w/bgrnd  to Sierra without background when voice
    // is available (for Tierra)
    if ((game.options[OPT_SPEECHTYPE] == 2) && (play.no_textbg_when_voice > 0)) {
        game.options[OPT_SPEECHTYPE] = 1;
        play.no_textbg_when_voice = 2;
    }

    return 1;
}

void stop_speech() {
    if (channels[SCHAN_SPEECH] != NULL) {
        play.music_master_volume = play.music_vol_was;
        // update the music in a bit (fixes two speeches follow each other
        // and music going up-then-down)
        update_music_at = 20;
        mvolcounter = 1;
        stop_and_destroy_channel (SCHAN_SPEECH);
        curLipLine = -1;

        if (play.no_textbg_when_voice == 2) {
            // set back to Sierra w/bgrnd
            play.no_textbg_when_voice = 1;
            game.options[OPT_SPEECHTYPE] = 2;
        }
    }
}
