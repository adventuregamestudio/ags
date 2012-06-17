
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acaudio/ac_music.h"
#include "acrun/ac_gamestate.h"
#include "acaudio/ac_sound.h"
#include "acdebug/ac_debug.h"
#include "acrun/ac_gamesetup.h"
#include "acaudio/ac_audio.h"
#include "ac/ac_common.h"
#include "acrun/ac_runninggame.h"
#include "acmain/ac_file.h"


int current_music_type = 0;
// crossFading is >0 (channel number of new track), or -1 (old
// track fading out, no new track)
int crossFading = 0, crossFadeVolumePerStep = 0, crossFadeStep = 0;
int crossFadeVolumeAtStart = 0;
SOUNDCLIP *cachedQueuedMusic = NULL;

volatile bool update_mp3_thread_running = false;
int musicPollIterator; // long name so it doesn't interfere with anything else

volatile int mvolcounter = 0;
int update_music_at=0;

void PlayMusicResetQueue(int newmus) {
    play.music_queue_size = 0;
    newmusic(newmus);
}

/*
#include "almp3_old.h"
ALLEGRO_MP3 *mp3ptr;
int mp3vol=128;

void amp_setvolume(int newvol) { mp3vol=newvol; }
int load_amp(char*namm,int loop) {
  mp3ptr = new ALLEGRO_MP3(namm);
  if (mp3ptr == NULL) return 0;
  if (mp3ptr->get_error_code() != 0) {
    delete mp3ptr;
    return 0;
    }
  mp3ptr->play(mp3vol, 8192);
  return 1;
  }
void install_amp() { }
void unload_amp() {
  mp3ptr->stop();
  delete mp3ptr;
  }
int amp_decode() {
  mp3ptr->poll();
  if (mp3ptr->is_finished()) {
    if (play.music_repeat)
      mp3ptr->play(mp3vol, 8192);
    else return -1;
    }
  return 0;
  }
*/
//#endif




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

void clear_music_cache() {

    if (cachedQueuedMusic != NULL) {
        cachedQueuedMusic->destroy();
        delete cachedQueuedMusic;
        cachedQueuedMusic = NULL;
    }

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

void play_next_queued() {
    // check if there's a queued one to play
    if (play.music_queue_size > 0) {

        int tuneToPlay = play.music_queue[0];

        if (tuneToPlay >= QUEUED_MUSIC_REPEAT) {
            // Loop it!
            play.music_repeat++;
            play_new_music(tuneToPlay - QUEUED_MUSIC_REPEAT, cachedQueuedMusic);
            play.music_repeat--;
        }
        else {
            // Don't loop it!
            int repeatWas = play.music_repeat;
            play.music_repeat = 0;
            play_new_music(tuneToPlay, cachedQueuedMusic);
            play.music_repeat = repeatWas;
        }

        // don't free the memory, as it has been transferred onto the
        // main music channel
        cachedQueuedMusic = NULL;

        play.music_queue_size--;
        for (int i = 0; i < play.music_queue_size; i++)
            play.music_queue[i] = play.music_queue[i + 1];

        if (play.music_queue_size > 0)
            cachedQueuedMusic = load_music_from_disk(play.music_queue[0], 0);
    }

}

int calculate_max_volume() {
    // quieter so that sounds can be heard better
    int newvol=play.music_master_volume + ((int)thisroom.options[ST_VOLUME]) * 30;
    if (newvol>255) newvol=255;
    if (newvol<0) newvol=0;

    if (play.fast_forward)
        newvol = 0;

    return newvol;
}

void update_polled_stuff_if_runtime()
{
    update_polled_stuff(true);
}

// add/remove the volume drop to the audio channels while speech is playing
void apply_volume_drop_modifier(bool applyModifier)
{
    for (int i = 0; i < MAX_SOUND_CHANNELS; i++) 
    {
        if ((channels[i] != NULL) && (channels[i]->done == 0) && (channels[i]->sourceClip != NULL))
        {
            if (applyModifier)
            {
                int audioType = ((ScriptAudioClip*)channels[i]->sourceClip)->type;
                channels[i]->volModifier = -(game.audioClipTypes[audioType].volume_reduction_while_speech_playing * 255 / 100);
            }
            else
                channels[i]->volModifier = 0;

            channels[i]->set_volume(channels[i]->vol);
        }
    }
}

void update_polled_stuff(bool checkForDebugMessages) {
    UPDATE_MP3

        if (want_exit) {
            want_exit = 0;
            quit("||exit!");
        }
        if (mvolcounter > update_music_at) {
            update_music_volume();
            apply_volume_drop_modifier(false);
            update_music_at = 0;
            mvolcounter = 0;
            update_ambient_sound_vol();
        }

        if ((editor_debugging_initialized) && (checkForDebugMessages))
            check_for_messages_from_editor();
}

// Update the music, and advance the crossfade on a step
// (this should only be called once per game loop)
void update_polled_stuff_and_crossfade () {
    update_polled_stuff_if_runtime ();

    audio_update_polled_stuff();

    if (crossFading) {
        crossFadeStep++;
        update_music_volume();
    }

    // Check if the current music has finished playing
    if ((play.cur_music_number >= 0) && (play.fast_forward == 0)) {
        if (IsMusicPlaying() == 0) {
            // The current music has finished
            play.cur_music_number = -1;
            play_next_queued();
        }
        else if ((game.options[OPT_CROSSFADEMUSIC] > 0) &&
            (play.music_queue_size > 0) && (!crossFading)) {
                // want to crossfade, and new tune in the queue
                int curpos = channels[SCHAN_MUSIC]->get_pos_ms();
                int muslen = channels[SCHAN_MUSIC]->get_length_ms();
                if ((curpos > 0) && (muslen > 0)) {
                    // we want to crossfade, and we know how far through
                    // the tune we are
                    int takesSteps = calculate_max_volume() / game.options[OPT_CROSSFADEMUSIC];
                    int takesMs = (takesSteps * 1000) / frames_per_second;
                    if (curpos >= muslen - takesMs)
                        play_next_queued();
                }
        }
    }

}


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
