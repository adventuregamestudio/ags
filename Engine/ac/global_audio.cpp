
#include "ac/global_audio.h"
#include "wgt2allg.h"
#include "ac/gamestate.h"
#include "ac/gamesetup.h"
#include "media/audio/audio.h"
#include "media/audio/sound.h"
#include "ac/ac_common.h"
#include "debug/debug.h"
#include "ac/ac_roomstruct.h"
#include "ac/game_audio.h"
#include "ac/file.h"

extern GameSetup usetup;
extern GameState play;
extern roomstruct thisroom;

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

    if (usetup.digicard == DIGI_NONE)
        return;

    // only play the sound if it's not already playing
    if ((ambient[channel].channel < 1) || (channels[ambient[channel].channel] == NULL) ||
        (channels[ambient[channel].channel]->done == 1) ||
        (ambient[channel].num != sndnum)) {

            StopAmbientSound(channel);
            // in case a normal non-ambient sound was playing, stop it too
            stop_and_destroy_channel(channel);

            SOUNDCLIP *asound = load_sound_from_path(sndnum, vol, true);

            if (asound == NULL) {
                debug_log ("Cannot load ambient sound %d", sndnum);
                DEBUG_CONSOLE("FAILED to load ambient sound %d", sndnum);
                return;
            }

            DEBUG_CONSOLE("Playing ambient sound %d on channel %d", sndnum, channel);
            ambient[channel].channel = channel;
            channels[channel] = asound;
            channels[channel]->priority = 15;  // ambient sound higher priority than normal sfx
    }
    // calculate the maximum distance away the player can be, using X
    // only (since X centred is still more-or-less total Y)
    ambient[channel].maxdist = ((x > thisroom.width / 2) ? x : (thisroom.width - x)) - AMBIENCE_FULL_DIST;
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

    // if no sound, ignore it
    if (usetup.digicard == DIGI_NONE)
        return -1;

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
    if ((last_sound_played[channel] == val1) && (channels[channel] != NULL)) {
        DEBUG_CONSOLE("Playing sound %d on channel %d; cached", val1, channel);
        channels[channel]->restart();
        channels[channel]->set_volume (play.sound_volume);
        return channel;
    }
    // free the old sound
    stop_and_destroy_channel (channel);
    DEBUG_CONSOLE("Playing sound %d on channel %d", val1, channel);

    last_sound_played[channel] = val1;

    SOUNDCLIP *soundfx = load_sound_from_path(val1, play.sound_volume, 0);

    if (soundfx == NULL) {
        debug_log("Sound sample load failure: cannot load sound %d", val1);
        DEBUG_CONSOLE("FAILED to load sound %d", val1);
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
        DEBUG_CONSOLE("Seek MIDI position to %d", position);
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

    if (usetup.midicard == MIDI_NONE)
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
        DEBUG_CONSOLE("Too many queued music, cannot add %d", musnum);
        return 0;
    }

    if ((play.music_queue_size > 0) && 
        (play.music_queue[play.music_queue_size - 1] >= QUEUED_MUSIC_REPEAT)) {
            quit("!PlayMusicQueued: cannot queue music after a repeating tune has been queued");
    }

    if (play.music_repeat) {
        DEBUG_CONSOLE("Queuing music %d to loop", musnum);
        musnum += QUEUED_MUSIC_REPEAT;
    }
    else {
        DEBUG_CONSOLE("Queuing music %d", musnum);
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
    if (current_music_type == MUS_MOD) {
        channels[SCHAN_MUSIC]->seek (patnum);
        DEBUG_CONSOLE("Seek MOD/XM to pattern %d", patnum);
    }
}
void SeekMP3PosMillis (int posn) {
    if (current_music_type) {
        DEBUG_CONSOLE("Seek MP3/OGG to %d ms", posn);
        if (crossFading)
            channels[crossFading]->seek (posn);
        else
            channels[SCHAN_MUSIC]->seek (posn);
    }
}

int GetMP3PosMillis () {
    // in case they have "while (GetMP3PosMillis() < 5000) "
    if (play.fast_forward)
        return 999999;

    if (current_music_type) {
        int result = channels[SCHAN_MUSIC]->get_pos_ms();
        if (result >= 0)
            return result;

        return channels[SCHAN_MUSIC]->get_pos ();
    }

    return 0;
}

void SetMusicVolume(int newvol) {
    if ((newvol < -3) || (newvol > 5))
        quit("!SetMusicVolume: invalid volume number. Must be from -3 to 5.");
    thisroom.options[ST_VOLUME]=newvol;
    update_music_volume();
}

void SetMusicMasterVolume(int newvol) {
    if ((newvol<0) | (newvol>100))
        quit("!SetMusicMasterVolume: invalid volume - must be from 0-100");
    play.music_master_volume=newvol+60;
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

void PlayMP3File (char *filename) {
    if (strlen(filename) >= PLAYMP3FILE_MAX_FILENAME_LEN)
        quit("!PlayMP3File: filename too long");

    DEBUG_CONSOLE("PlayMP3File %s", filename);

    char pathToFile[MAX_PATH];
    get_current_dir_path(pathToFile, filename);

    int useChan = prepare_for_new_music ();
    bool doLoop = (play.music_repeat > 0);

    if ((channels[useChan] = my_load_static_ogg(pathToFile, 150, doLoop)) != NULL) {
        channels[useChan]->play();
        current_music_type = MUS_OGG;
        play.cur_music_number = 1000;
        // save the filename (if it's not what we were supplied with)
        if (filename != &play.playmp3file_name[0])
            strcpy (play.playmp3file_name, filename);
    }
    else if ((channels[useChan] = my_load_static_mp3(pathToFile, 150, doLoop)) != NULL) {
        channels[useChan]->play();
        current_music_type = MUS_MP3;
        play.cur_music_number = 1000;
        // save the filename (if it's not what we were supplied with)
        if (filename != &play.playmp3file_name[0])
            strcpy (play.playmp3file_name, filename);
    }
    else
        debug_log ("PlayMP3File: file '%s' not found or cannot play", filename);

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
    channels[play.silent_midi_channel]->set_volume(0);
    channels[play.silent_midi_channel]->volAsPercentage = 0;
}

