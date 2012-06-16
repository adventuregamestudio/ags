
#include "acmain/ac_maindefines.h"


bool AmbientSound::IsPlaying () {
    if (channel <= 0)
        return false;
    return (channels[channel] != NULL) ? true : false;
}



void force_audiostream_include() {
    // This should never happen, but the call is here to make it
    // link the audiostream libraries
    stop_audio_stream(NULL);
}


AmbientSound ambient[MAX_SOUND_CHANNELS + 1];  // + 1 just for safety on array iterations

int get_volume_adjusted_for_distance(int volume, int sndX, int sndY, int sndMaxDist)
{
  int distx = playerchar->x - sndX;
  int disty = playerchar->y - sndY;
  // it uses Allegro's "fix" sqrt without the ::
  int dist = (int)::sqrt((double)(distx*distx + disty*disty));

  // if they're quite close, full volume
  int wantvol = volume;

  if (dist >= AMBIENCE_FULL_DIST)
  {
    // get the relative volume
    wantvol = ((dist - AMBIENCE_FULL_DIST) * volume) / sndMaxDist;
    // closer is louder
    wantvol = volume - wantvol;
  }

  return wantvol;
}

void update_directional_sound_vol()
{
  for (int chan = 1; chan < MAX_SOUND_CHANNELS; chan++) 
  {
    if ((channels[chan] != NULL) && (channels[chan]->done == 0) &&
        (channels[chan]->xSource >= 0)) 
    {
      channels[chan]->directionalVolModifier = 
        get_volume_adjusted_for_distance(channels[chan]->vol, 
                channels[chan]->xSource,
                channels[chan]->ySource,
                channels[chan]->maximumPossibleDistanceAway) -
        channels[chan]->vol;

      channels[chan]->set_volume(channels[chan]->vol);
    }
  }
}

void update_ambient_sound_vol () {

  for (int chan = 1; chan < MAX_SOUND_CHANNELS; chan++) {

    AmbientSound *thisSound = &ambient[chan];

    if (thisSound->channel == 0)
      continue;

    int sourceVolume = thisSound->vol;

    if ((channels[SCHAN_SPEECH] != NULL) && (channels[SCHAN_SPEECH]->done == 0)) {
      // Negative value means set exactly; positive means drop that amount
      if (play.speech_music_drop < 0)
        sourceVolume = -play.speech_music_drop;
      else
        sourceVolume -= play.speech_music_drop;

      if (sourceVolume < 0)
        sourceVolume = 0;
      if (sourceVolume > 255)
        sourceVolume = 255;
    }

    // Adjust ambient volume so it maxes out at overall sound volume
    int ambientvol = (sourceVolume * play.sound_volume) / 255;

    int wantvol;

    if ((thisSound->x == 0) && (thisSound->y == 0)) {
      wantvol = ambientvol;
    }
    else {
      wantvol = get_volume_adjusted_for_distance(ambientvol, thisSound->x, thisSound->y, thisSound->maxdist);
    }

    if (channels[thisSound->channel] == NULL)
      quit("Internal error: the ambient sound channel is enabled, but it has been destroyed");

    channels[thisSound->channel]->set_volume(wantvol);
  }
}


void StopAmbientSound (int channel) {
  if ((channel < 0) || (channel >= MAX_SOUND_CHANNELS))
    quit("!StopAmbientSound: invalid channel");

  if (ambient[channel].channel == 0)
    return;

  stop_and_destroy_channel(channel);
  ambient[channel].channel = 0;
}

SOUNDCLIP *load_sound_from_path(int soundNumber, int volume, bool repeat) 
{
  SOUNDCLIP *soundfx = load_sound_clip_from_old_style_number(false, soundNumber, repeat);

  if (soundfx != NULL) {
    if (soundfx->play() == 0)
      soundfx = NULL;
  }

  return soundfx;
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


void stop_all_sound_and_music() 
{
    int a;
    stopmusic();
    // make sure it doesn't start crossfading when it comes back
    crossFading = 0;
    // any ambient sound will be aborted
    for (a = 0; a <= MAX_SOUND_CHANNELS; a++)
        stop_and_destroy_channel(a);
}

void shutdown_sound() 
{
    stop_all_sound_and_music();

#ifndef PSP_NO_MOD_PLAYBACK
    if (opts.mod_player)
        remove_mod_player();
#endif
    remove_sound();
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
