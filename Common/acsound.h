/*
** ACSOUND - AGS sound system wrapper
** Copyright (C) 2002-2003, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __ACSOUND_H
#define __ACSOUND_H

#if defined(PSP_VERSION)
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspthreadman.h>
#elif defined(LINUX_VERSION)
#include <pthread.h>
#elif defined(WINDOWS_VERSION)
#include <windows.h>
#endif

#define MUS_MIDI 1
#define MUS_MP3  2
#define MUS_WAVE 3
#define MUS_MOD  4
#define MUS_OGG  5

struct SOUNDCLIP
{
  int done;
  int priority;
  int soundType;
  int vol;
  int volAsPercentage;
  int originalVolAsPercentage;
  int volModifier;
  int paused;
  int panning;
  int panningAsPercentage;
  int xSource, ySource;
  int maximumPossibleDistanceAway;
  int directionalVolModifier;
  bool repeat;
  void *sourceClip;
  bool ready;

#if defined(PSP_VERSION)
  SceUID mutex;
#elif defined(LINUX_VERSION)
  pthread_mutex_t mutex;
#elif defined(WINDOWS_VERSION)
  HANDLE mutex;
#endif

  virtual int poll() = 0;
  virtual void destroy() = 0;
  virtual void set_volume(int) = 0;
  virtual void restart() = 0;
  virtual void seek(int) = 0;
  virtual int get_pos() = 0;    // return 0 to indicate seek not supported
  virtual int get_pos_ms() = 0; // this must always return valid value if poss
  virtual int get_length_ms() = 0; // return total track length in ms (or 0)
  virtual int get_voice() = 0;  // return the allegro voice number (or -1 if none)
  virtual int get_sound_type() = 0;
  virtual int play() = 0;

  virtual int play_from(int position) 
  {
    int retVal = play();
    if ((retVal != 0) && (position > 0))
    {
      seek(position);
    }
    return retVal;
  }

  virtual void set_panning(int newPanning) {
    int voice = get_voice();
    if (voice >= 0) {
      voice_set_pan(voice, newPanning);
      panning = newPanning;
    }
  }

  virtual void pause() {
    int voice = get_voice();
    if (voice >= 0) {
      voice_stop(voice);
      paused = 1;
    }
  }
  virtual void resume() {
    int voice = get_voice();
    if (voice >= 0)
      voice_start(voice);
    paused = 0;
  }

  SOUNDCLIP() {
    ready = false;
    done = 0;
    paused = 0;
    priority = 50;
    panning = 128;
    panningAsPercentage = 0;
    soundType = -1;
    sourceClip = NULL;
    volModifier = 0;
    repeat = false;
    xSource = -1;
    ySource = -1;
    maximumPossibleDistanceAway = 0;
    directionalVolModifier = 0;
    createMutex();
  }

  ~SOUNDCLIP()
  {
    destroyMutex();
  }

  inline void createMutex()
  {
#if defined(PSP_VERSION)
    mutex = sceKernelCreateSema("SoundMutex", 0, 1, 1, 0);
#elif defined(LINUX_VERSION)
    pthread_mutex_init(&mutex, NULL);
#elif defined(WINDOWS_VERSION)
    mutex = CreateMutex(NULL, FALSE, NULL); 
#endif
  }

  inline void lockMutex()
  {
#if defined(PSP_VERSION)
    sceKernelWaitSema(mutex, 1, 0);
#elif defined(LINUX_VERSION)
    pthread_mutex_lock(&mutex);
#elif defined(WINDOWS_VERSION)
    WaitForSingleObject(mutex, INFINITE);
#endif
  }

  inline void releaseMutex()
  {
#if defined(PSP_VERSION)
    sceKernelSignalSema(mutex, 1);
#elif defined(LINUX_VERSION)
    pthread_mutex_unlock(&mutex);
#elif defined(WINDOWS_VERSION)
    ReleaseMutex(mutex);
#endif
  }

  inline void destroyMutex()
  {
#if defined(PSP_VERSION)
    sceKernelDeleteSema(mutex);
#elif defined(LINUX_VERSION)
    pthread_mutex_destroy(&mutex);
#elif defined(WINDOWS_VERSION)
    CloseHandle(mutex); 
#endif
  }
};

SOUNDCLIP *my_load_wave(const char *filename, int voll, int loop);
SOUNDCLIP *my_load_mp3(const char *filname, int voll);
SOUNDCLIP *my_load_static_mp3(const char *filname, int voll, bool loop);
SOUNDCLIP *my_load_static_ogg(const char *filname, int voll, bool loop);
SOUNDCLIP *my_load_ogg(const char *filname, int voll);
SOUNDCLIP *my_load_midi(const char *filname, int repet);
SOUNDCLIP *my_load_mod(const char *filname, int repet);

int  init_mod_player(int numVoices);
void remove_mod_player();

#endif // __ACSOUND_H
