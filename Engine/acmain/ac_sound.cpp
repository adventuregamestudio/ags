
#include "acmain/ac_maindefines.h"


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

// the sound will only be played if there is a free channel or
// it has a priority >= an existing sound to override
int play_sound_priority (int val1, int priority) {
    int lowest_pri = 9999, lowest_pri_id = -1;

    // find a free channel to play it on
    for (int i = SCHAN_NORMAL; i < numSoundChannels; i++) {
        if (val1 < 0) {
            // Playing sound -1 means iterate through and stop all sound
            if ((channels[i] != NULL) && (channels[i]->done == 0))
                stop_and_destroy_channel (i);
        }
        else if ((channels[i] == NULL) || (channels[i]->done != 0)) {
            if (PlaySoundEx(val1, i) >= 0)
                channels[i]->priority = priority;
            return i;
        }
        else if (channels[i]->priority < lowest_pri) {
            lowest_pri = channels[i]->priority;
            lowest_pri_id = i;
        }

    }
    if (val1 < 0)
        return -1;

    // no free channels, see if we have a high enough priority
    // to override one
    if (priority >= lowest_pri) {
        if (PlaySoundEx(val1, lowest_pri_id) >= 0) {
            channels[lowest_pri_id]->priority = priority;
            return lowest_pri_id;
        }
    }

    return -1;
}

int play_sound(int val1) {
    return play_sound_priority(val1, 10);
}
