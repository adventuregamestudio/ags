
#include "acmain/ac_maindefines.h"


void stopmusic() {

  if (crossFading > 0) {
    // stop in the middle of a new track fading in
    // Abort the new track, and let the old one finish fading out
    stop_and_destroy_channel (crossFading);
    crossFading = -1;
  }
  else if (crossFading < 0) {
    // the music is already fading out
    if (game.options[OPT_CROSSFADEMUSIC] <= 0) {
      // If they have since disabled crossfading, stop the fadeout
      stop_and_destroy_channel(SCHAN_MUSIC);
      crossFading = 0;
      crossFadeStep = 0;
      update_music_volume();
    }
  }
  else if ((game.options[OPT_CROSSFADEMUSIC] > 0)
      && (channels[SCHAN_MUSIC] != NULL)
      && (channels[SCHAN_MUSIC]->done == 0)
      && (current_music_type != 0)
      && (current_music_type != MUS_MIDI)
      && (current_music_type != MUS_MOD)) {

    crossFading = -1;
    crossFadeStep = 0;
    crossFadeVolumePerStep = game.options[OPT_CROSSFADEMUSIC];
    crossFadeVolumeAtStart = calculate_max_volume();
  }
  else
    stop_and_destroy_channel (SCHAN_MUSIC);

  play.cur_music_number = -1;
  current_music_type = 0;
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

int Game_GetMODPattern() {
  if (current_music_type == MUS_MOD) {
    return channels[SCHAN_MUSIC]->get_pos();
  }
  return -1;
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

void update_music_volume() {

  if ((current_music_type) || (crossFading < 0)) 
  {
    // targetVol is the maximum volume we're fading in to
    // newvol is the starting volume that we faded out from
    int targetVol = calculate_max_volume();
    int newvol;
    if (crossFading)
      newvol = crossFadeVolumeAtStart;
    else
      newvol = targetVol;

    // fading out old track, target volume is silence
    if (crossFading < 0)
      targetVol = 0;

    if (crossFading) {
      int curvol = crossFadeVolumePerStep * crossFadeStep;

      if ((curvol > targetVol) && (curvol > newvol)) {
        // it has fully faded to the new track
        newvol = targetVol;
        stop_and_destroy_channel_ex(SCHAN_MUSIC, false);
        if (crossFading > 0) {
          channels[SCHAN_MUSIC] = channels[crossFading];
          channels[crossFading] = NULL;
        }
        crossFading = 0;
      }
      else {
        if (crossFading > 0)
          channels[crossFading]->set_volume((curvol > targetVol) ? targetVol : curvol);

        newvol -= curvol;
        if (newvol < 0)
          newvol = 0;
      }
    }
    if (channels[SCHAN_MUSIC])
      channels[SCHAN_MUSIC]->set_volume (newvol);
  }
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

int System_GetVolume() 
{
  return play.digital_master_volume;
}

void System_SetVolume(int newvol) 
{
  if ((newvol < 0) || (newvol > 100))
    quit("!System.Volume: invalid volume - must be from 0-100");

  if (newvol == play.digital_master_volume)
    return;

  play.digital_master_volume = newvol;
  set_volume((newvol * 255) / 100, (newvol * 255) / 100);

  // allegro's set_volume can lose the volumes of all the channels
  // if it was previously set low; so restore them
  for (int i = 0; i <= MAX_SOUND_CHANNELS; i++) 
  {
    if ((channels[i] != NULL) && (channels[i]->done == 0)) 
    {
      channels[i]->set_volume(channels[i]->vol);
    }
  }
}

int GetCurrentMusic() {
  return play.cur_music_number;
  }

void SetMusicRepeat(int loopflag) {
  play.music_repeat=loopflag;
}

// Ensures crossfader is stable after loading (or failing to load)
// new music
void post_new_music_check (int newchannel) {
  if ((crossFading > 0) && (channels[crossFading] == NULL)) {
    crossFading = 0;
    // Was fading out but then they played invalid music, continue
    // to fade out
    if (channels[SCHAN_MUSIC] != NULL)
      crossFading = -1;
  }

}

// Sets up the crossfading for playing the new music track,
// and returns the channel number to use
int prepare_for_new_music () {
  int useChannel = SCHAN_MUSIC;
  
  if ((game.options[OPT_CROSSFADEMUSIC] > 0)
      && (channels[SCHAN_MUSIC] != NULL)
      && (channels[SCHAN_MUSIC]->done == 0)
      && (current_music_type != MUS_MIDI)
      && (current_music_type != MUS_MOD)) {
      
    if (crossFading > 0) {
      // It's still crossfading to the previous track
      stop_and_destroy_channel_ex(SCHAN_MUSIC, false);
      channels[SCHAN_MUSIC] = channels[crossFading];
      channels[crossFading] = NULL;
      crossFading = 0;
      update_music_volume();
    }
    else if (crossFading < 0) {
      // an old track is still fading out, no new music yet
      // Do nothing, and keep the current crossfade step
    }
    else {
      // start crossfading
      crossFadeStep = 0;
      crossFadeVolumePerStep = game.options[OPT_CROSSFADEMUSIC];
      crossFadeVolumeAtStart = calculate_max_volume();
    }
    useChannel = SPECIAL_CROSSFADE_CHANNEL;
    crossFading = useChannel;
  }
  else {
    // crossfading is now turned off
    stopmusic();
    // ensure that any traces of old tunes fading are eliminated
    // (otherwise the new track will be faded out)
    crossFading = 0;
  }

  // Just make sure, because it will be overwritten in a sec
  if (channels[useChannel] != NULL)
    stop_and_destroy_channel (useChannel);

  return useChannel;
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

SOUNDCLIP *load_music_from_disk(int mnum, bool doRepeat) {

  if (mnum >= QUEUED_MUSIC_REPEAT) {
    mnum -= QUEUED_MUSIC_REPEAT;
    doRepeat = true;
  }

  SOUNDCLIP *loaded = load_sound_clip_from_old_style_number(true, mnum, doRepeat);

  if ((loaded == NULL) && (mnum > 0)) 
  {
    debug_log("Music %d not found",mnum);
    DEBUG_CONSOLE("FAILED to load music %d", mnum);
  }

  return loaded;
}


void play_new_music(int mnum, SOUNDCLIP *music) {
  if (debug_flags & DBG_NOMUSIC)
    return;
  if (usetup.midicard == MIDI_NONE)
    return;

  if ((play.cur_music_number == mnum) && (music == NULL)) {
    DEBUG_CONSOLE("PlayMusic %d but already playing", mnum);
    return;  // don't play the music if it's already playing
  }

  int useChannel = SCHAN_MUSIC;
  DEBUG_CONSOLE("Playing music %d", mnum);

  if (mnum<0) {
    stopmusic();
    return;
  }

  if (play.fast_forward) {
    // while skipping cutscene, don't change the music
    play.end_cutscene_music = mnum;
    return;
  }

  useChannel = prepare_for_new_music ();

  play.cur_music_number=mnum;
  current_music_type = 0;
  channels[useChannel] = NULL;

  play.current_music_repeating = play.music_repeat;
  // now that all the previous music is unloaded, load in the new one

  if (music != NULL) {
    channels[useChannel] = music;
    music = NULL;
  }
  else {
    channels[useChannel] = load_music_from_disk(mnum, (play.music_repeat > 0));
  }

  if (channels[useChannel] != NULL) {

    if (channels[useChannel]->play() == 0)
      channels[useChannel] = NULL;
    else
      current_music_type = channels[useChannel]->get_sound_type();
  }

  post_new_music_check(useChannel);

  update_music_volume();

}

void newmusic(int mnum) {
  play_new_music(mnum, NULL);
}
