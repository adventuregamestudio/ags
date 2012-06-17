#ifndef __AC_MUSIC_H
#define __AC_MUSIC_H

#include "acaudio/ac_soundclip.h"

extern int current_music_type;

void newmusic(int mnum);
SOUNDCLIP *load_music_from_disk(int mnum, bool doRepeat);
void play_new_music(int mnum, SOUNDCLIP *music);
void update_polled_stuff(bool checkForDebugMessages);
void update_music_volume();
void update_polled_stuff_and_crossfade ();

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

#endif // __AC_MUSIC_H