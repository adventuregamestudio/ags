#ifndef __AC_MUSIC_H
#define __AC_MUSIC_H

#include "acaudio/ac_soundclip.h"

extern int current_music_type;

void PlayMusicResetQueue(int newmus);
void SeekMIDIPosition (int position);
int GetMIDIPosition ();
int IsMusicPlaying();
void clear_music_cache();
int PlayMusicQueued(int musnum);
void play_next_queued();
int calculate_max_volume();
void update_polled_stuff_if_runtime();
// add/remove the volume drop to the audio channels while speech is playing
void apply_volume_drop_modifier(bool applyModifier);
void update_polled_stuff(bool checkForDebugMessages);
// Update the music, and advance the crossfade on a step
// (this should only be called once per game loop);
void update_polled_stuff_and_crossfade ();
void stopmusic();
void scr_StopMusic();
void SeekMODPattern(int patnum);
int Game_GetMODPattern();
void SeekMP3PosMillis (int posn);
int GetMP3PosMillis ();
void update_music_volume();
void SetMusicVolume(int newvol);
void SetMusicMasterVolume(int newvol);
void SetSoundVolume(int newvol);
void SetChannelVolume(int chan, int newvol);
void SetDigitalMasterVolume (int newvol);
int System_GetVolume();
void System_SetVolume(int newvol);
int GetCurrentMusic();
void SetMusicRepeat(int loopflag);

void post_new_music_check (int newchannel);

int prepare_for_new_music ();
void PlayMP3File (char *filename);
void PlaySilentMIDI (int mnum);
SOUNDCLIP *load_music_from_disk(int mnum, bool doRepeat);
void play_new_music(int mnum, SOUNDCLIP *music);
void newmusic(int mnum);

#define UPDATE_MP3 \
    if (!psp_audio_multithreaded) \
{ UPDATE_MP3_THREAD }

//#define UPDATE_MP3 update_polled_stuff_if_runtime();

// PSP: Update in thread if wanted.
//extern volatile int psp_audio_multithreaded;
extern volatile bool update_mp3_thread_running;
extern int musicPollIterator; // long name so it doesn't interfere with anything else
#define UPDATE_MP3_THREAD \
    while (switching_away_from_game) { } \
    for (musicPollIterator = 0; musicPollIterator <= MAX_SOUND_CHANNELS; musicPollIterator++) { \
    if ((channels[musicPollIterator] != NULL) && (channels[musicPollIterator]->done == 0)) \
    channels[musicPollIterator]->poll(); \
    }

extern volatile int mvolcounter;
extern int update_music_at;

// crossFading is >0 (channel number of new track), or -1 (old
// track fading out, no new track)
extern int crossFading, crossFadeVolumePerStep, crossFadeStep;
extern int crossFadeVolumeAtStart;

extern SOUNDCLIP *cachedQueuedMusic;

#endif // __AC_MUSIC_H