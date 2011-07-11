/*
  ACSOUND - AGS sound system wrapper

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.
*/
#define WGT2ALLEGRO_NOFUNCTIONS
#include "wgt2allg.h"
#include "acsound.h"
#include "alogg.h"
#include "almp3.h"

// PSP: Additional header for the sound cache.
#include <pspsdk.h>
#include <psprtc.h>

#if (defined(LINUX_VERSION) || defined(WINDOWS_VERSION) || defined(MAC_VERSION))
#define DUMB_MOD_PLAYER
#else
#define JGMOD_MOD_PLAYER
#endif

#if defined(MAC_VERSION) || defined(LINUX_VERSION)
// for toupper
#include <ctype.h>
#endif

#if ALLEGRO_DATE > 20050101
// because we have to use Allegro 4.2
// and the packfile format has changed slightly
// 'todo' has been put in a structure called 'normal'
#define todo normal.todo
#endif

extern "C" {
  extern int alogg_is_end_of_oggstream(ALOGG_OGGSTREAM *ogg);
  extern int alogg_is_end_of_ogg(ALOGG_OGG *ogg);
  extern int alogg_get_ogg_freq(ALOGG_OGG *ogg);
  extern int alogg_get_ogg_stereo(ALOGG_OGG *ogg);
}

PACKFILE *mp3in;
extern int use_extra_sound_offset;
extern int our_eip;
extern void quit(char *);
extern void write_log(char*msg) ;
//extern void sample_update_callback(SAMPLE *sample, int voice);


// PSP: A simple sound cache. The size can be configured in the config file.
// The data rate while reading from disk on the PSP is usually between 500 to 900 kiB/s,
// caching the last used sound files therefore improves game performance.

extern int psp_use_sound_cache;
extern int psp_sound_cache_max_size;
extern int psp_audio_cachesize;

int dont_disturb = 0;

typedef struct
{
  char file_name[20];
  int number;
  int free;
  u64 last_used;
  unsigned int size;
  char* data;
  int reference;
} sound_cache_entry_t;

sound_cache_entry_t* sound_cache_entries = NULL;


void clear_sound_cache()
{
  if (sound_cache_entries)
  {
    int i;
    for (i = 0; i < psp_audio_cachesize; i++)
    {
      if (sound_cache_entries[i].data)
      {
        free(sound_cache_entries[i].data);
        sound_cache_entries[i].data = NULL;
        sound_cache_entries[i].reference = 0;
      }
    }
  }
  else
  {
    sound_cache_entries = (sound_cache_entry_t*)malloc(psp_audio_cachesize * sizeof(sound_cache_entry_t));
	memset(sound_cache_entries, 0, psp_audio_cachesize * sizeof(sound_cache_entry_t));
  }
}

void sound_cache_free(char* buffer)
{
  int i;
  for (i = 0; i < psp_audio_cachesize; i++)
  {
    if (sound_cache_entries[i].data == buffer)
    {
      if (sound_cache_entries[i].reference > 0)
      {
        sound_cache_entries[i].reference--;
        return;
      }
    }
  }
}


char* get_cached_sound(const char* filename, long* size)
{
  *size = 0;

  int i;
  for (i = 0; i < psp_audio_cachesize; i++)
  {
    if (sound_cache_entries[i].data == NULL)
      continue;

    if (strcmp(filename, sound_cache_entries[i].file_name) == 0)
    {
      sceRtcGetCurrentTick(&sound_cache_entries[i].last_used);
      *size = sound_cache_entries[i].size;
      return sound_cache_entries[i].data;
    }
  }

  // Not found
  PACKFILE *mp3in = pack_fopen(filename, "rb");
  if (mp3in == NULL)
    return NULL;

  // Find free slot
  for (i = 0; i < psp_audio_cachesize; i++)
  {
    if (sound_cache_entries[i].data == NULL)
      break;
  }

  // Not free slot?
  if (i == psp_audio_cachesize)
  {
    u64 oldest;
    sceRtcGetCurrentTick(&oldest);
    int index = 0;

    for (i = 0; i < psp_audio_cachesize; i++)
    {
      if (sound_cache_entries[i].reference == 0)
      {
        if (sound_cache_entries[i].last_used < oldest)
        {
          oldest = sound_cache_entries[i].last_used;
          index = i;
        }
      }
    }

    i = index;
  }

  sound_cache_entries[i].size = mp3in->todo;
  sound_cache_entries[i].data = (char *)malloc(sound_cache_entries[i].size);

  if (sound_cache_entries[i].data == NULL)
  {
    pack_fclose(mp3in);
    return NULL;
  }

  pack_fread(sound_cache_entries[i].data, sound_cache_entries[i].size, mp3in);
  pack_fclose(mp3in);

  strcpy(sound_cache_entries[i].file_name, filename);
  *size = sound_cache_entries[i].size;
  sound_cache_entries[i].reference++;
  sceRtcGetCurrentTick(&sound_cache_entries[i].last_used);

  return sound_cache_entries[i].data;
}





// My new MP3STREAM wrapper
struct MYWAVE:public SOUNDCLIP
{
  SAMPLE *wave;
  int voice;
  int firstTime;
  int repeat;

  int poll()
  {
    if (wave == NULL)
      return 1;
    if (paused)
      return 0;

    if (dont_disturb)
      return done;
    dont_disturb = 1;

    if (firstTime) {
      // need to wait until here so that we have been assigned a channel
      //sample_update_callback(wave, voice);
      firstTime = 0;
    }

    if (voice_get_position(voice) < 0)
      done = 1;

    dont_disturb = 0;

    return done;
  }

  void set_volume(int newvol)
  {
    vol = newvol;

    if (voice >= 0)
    {
      newvol += volModifier + directionalVolModifier;
      if (newvol < 0) newvol = 0;
      voice_set_volume(voice, newvol);
    }
  }

  void destroy()
  {
    destroy_sample(wave);
    wave = NULL;
  }

  void seek(int pos)
  {
    voice_set_position(voice, pos);
  }

  int get_pos()
  {
    return voice_get_position(voice);
  }

  int get_pos_ms()
  {
    // convert the offset in samples into the offset in ms
    //return ((1000000 / voice_get_frequency(voice)) * voice_get_position(voice)) / 1000;

    if (voice_get_frequency(voice) < 100)
      return 0;
    // (number of samples / (samples per second / 100)) * 10 = ms
    return (voice_get_position(voice) / (voice_get_frequency(voice) / 100)) * 10;
  }

  int get_length_ms()
  {
    if (wave->freq < 100)
      return 0;
    return (wave->len / (wave->freq / 100)) * 10;
  }

  void restart()
  {
    if (wave != NULL) {
      done = 0;
      paused = 0;
      stop_sample(wave);
      voice = play_sample(wave, vol, panning, 1000, 0);
    }
  }

  int get_voice()
  {
    return voice;
  }

  int get_sound_type() {
    return MUS_WAVE;
  }

  int play() {
    voice = play_sample(wave, vol, panning, 1000, repeat);

    return 1;
  }

  MYWAVE() : SOUNDCLIP() {
    voice = -1;
  }
};

MYWAVE *thiswave;
SOUNDCLIP *my_load_wave(const char *filename, int voll, int loop)
{
#ifdef MAC_VERSION
  SAMPLE *new_sample = load_wav(filename);
#else
  SAMPLE *new_sample = load_sample(filename);
#endif

  if (new_sample == NULL)
    return NULL;

  thiswave = new MYWAVE();
  thiswave->wave = new_sample;
  thiswave->vol = voll;
  thiswave->firstTime = 1;
  thiswave->repeat = loop;

  return thiswave;
}

//#define MP3CHUNKSIZE 100000
#define MP3CHUNKSIZE 32768

#ifndef NO_MP3_PLAYER

struct MYMP3:public SOUNDCLIP
{
  ALMP3_MP3STREAM *stream;
  PACKFILE *in;
  long  filesize;
  char *buffer;
  int chunksize;

  int poll()
  {
    if (done)
      return done;
    if (paused)
      return 0;

    if (dont_disturb)
      return done;
    dont_disturb = 1;

    if (!done) {
      // update the buffer
      char *tempbuf = (char *)almp3_get_mp3stream_buffer(stream);
      if (tempbuf != NULL) {
        int free_val = -1;
        if (chunksize > in->todo) {
          chunksize = in->todo;
          free_val = chunksize;
        }
        pack_fread(tempbuf, chunksize, in);
        almp3_free_mp3stream_buffer(stream, free_val);
      }
    }

    if (almp3_poll_mp3stream(stream) == ALMP3_POLL_PLAYJUSTFINISHED)
      done = 1;

    dont_disturb = 0;

    return done;
  }

  void set_volume(int newvol)
  {
    // boost MP3 volume
    newvol += 20;
    if (newvol > 255)
      newvol = 255;

    vol = newvol;
    newvol += volModifier + directionalVolModifier;
    if (newvol < 0) newvol = 0;
    almp3_adjust_mp3stream(stream, newvol, panning, 1000);
  }

  void destroy()
  {
    if (!done)
      almp3_stop_mp3stream(stream);

    almp3_destroy_mp3stream(stream);
    stream = NULL;

    if (buffer != NULL)
      free(buffer);

    buffer = NULL;
    pack_fclose(in);
  }

  void seek(int pos)
  {
    quit("Tried to seek an mp3stream");
  }

  int get_pos()
  {
    return 0; // Return 0 to signify that Seek is not supported
              // return almp3_get_pos_msecs_mp3stream (stream);
  }

  int get_pos_ms()
  {
    return almp3_get_pos_msecs_mp3stream(stream);
  }

  int get_length_ms()
  {
    return almp3_get_length_msecs_mp3stream(stream, filesize);
  }

  void restart()
  {
    if (stream != NULL) {
      // need to reset file pointer for this to work
      almp3_play_mp3stream(stream, MP3CHUNKSIZE, vol, panning);
      done = 0;
      paused = 0;
      poll();
    }
  }

  int get_voice()
  {
    AUDIOSTREAM *ast = almp3_get_audiostream_mp3stream(stream);
    if (ast)
      return ast->voice;
    return -1;
  }

  int get_sound_type() {
    return MUS_MP3;
  }

  int play() {
    almp3_play_mp3stream(stream, chunksize, (vol > 230) ? vol : vol + 20, panning);
    poll();

    return 1;
  }

  MYMP3() : SOUNDCLIP() {
  }
};

MYMP3 *thistune;
SOUNDCLIP *my_load_mp3(const char *filname, int voll)
{
  mp3in = pack_fopen(filname, "rb");
  if (mp3in == NULL)
    return NULL;

  char *tmpbuffer = (char *)malloc(MP3CHUNKSIZE);
  if (tmpbuffer == NULL) {
    pack_fclose(mp3in);
    return NULL;
  }
  thistune = new MYMP3();
  thistune->in = mp3in;
  thistune->chunksize = MP3CHUNKSIZE;
  thistune->filesize = mp3in->todo;
  thistune->done = 0;
  thistune->vol = voll;

  if (thistune->chunksize > mp3in->todo)
    thistune->chunksize = mp3in->todo;

  pack_fread(tmpbuffer, thistune->chunksize, mp3in);

  thistune->buffer = (char *)tmpbuffer;
  thistune->stream = almp3_create_mp3stream(tmpbuffer, thistune->chunksize, (mp3in->todo < 1));

  if (thistune->stream == NULL) {
    free(tmpbuffer);
    pack_fclose(mp3in);
    delete thistune;
    return NULL;
  }

  return thistune;
}

// pre-loaded (non-streaming) MP3 file
struct MYSTATICMP3:public SOUNDCLIP
{
  ALMP3_MP3 *tune;
  char *mp3buffer;

  int poll()
  {
    if (dont_disturb)
      return done;
    dont_disturb = 1;

    int oldeip = our_eip;
      our_eip = 5997;
    if (tune == NULL) ;
    else if (almp3_poll_mp3(tune) == ALMP3_POLL_PLAYJUSTFINISHED) {
      if (!repeat)
        done = 1;
    }
    our_eip = oldeip;

    dont_disturb = 0;

    return done;
  }

  void set_volume(int newvol)
  {
    vol = newvol;

    if (tune != NULL)
    {
      newvol += volModifier + directionalVolModifier;
      if (newvol < 0) newvol = 0;
      almp3_adjust_mp3(tune, newvol, panning, 1000, repeat);
    }

  }

  void destroy()
  {
    if (tune != NULL) {
      almp3_stop_mp3(tune);
      almp3_destroy_mp3(tune);
      tune = NULL;
    }
    if (mp3buffer != NULL) {
      sound_cache_free(mp3buffer);
    }

  }

  void seek(int pos)
  {
    almp3_seek_abs_msecs_mp3(tune, pos);
  }

  int get_pos()
  {
    return almp3_get_pos_msecs_mp3(tune);
  }

  int get_pos_ms()
  {
    return get_pos();
  }

  int get_length_ms()
  {
    return almp3_get_length_msecs_mp3(tune);
  }

  void restart()
  {
    if (tune != NULL) {
      almp3_stop_mp3(tune);
      almp3_rewind_mp3(tune);
      almp3_play_mp3(tune, 16384, vol, panning);
      done = 0;
      poll();
    }
  }

  int get_voice()
  {
    AUDIOSTREAM *ast = almp3_get_audiostream_mp3(tune);
    if (ast)
      return ast->voice;
    return -1;
  }

  int get_sound_type() {
    return MUS_MP3;
  }

  int play() {
    if (almp3_play_ex_mp3(tune, 16384, vol, panning, 1000, repeat) != ALMP3_OK) {
      destroy();
      delete this;
      return 0;
    }

    poll();
    return 1;
  }

  MYSTATICMP3() : SOUNDCLIP() {
  }

};

MYSTATICMP3 *thismp3;
SOUNDCLIP *my_load_static_mp3(const char *filname, int voll, bool loop)
{
  // Load via soundcache.
  long muslen = 0;
  char* mp3buffer = get_cached_sound(filname, &muslen);
  if (mp3buffer == NULL)
    return NULL;

  // now, create an MP3 structure for it
  thismp3 = new MYSTATICMP3();
  if (thismp3 == NULL) {
    free(mp3buffer);
    return NULL;
  }
  thismp3->vol = voll;
  thismp3->mp3buffer = NULL;
  thismp3->repeat = loop;
  thismp3->tune = almp3_create_mp3(mp3buffer, muslen);
  thismp3->done = 0;

  if (thismp3->tune == NULL) {
    free(mp3buffer);
    delete thismp3;
    return NULL;
  }

  thismp3->mp3buffer = mp3buffer;

  return thismp3;
}

#else

SOUNDCLIP *my_load_mp3(const char *filname, int voll)
{
  return NULL;
}

SOUNDCLIP *my_load_static_mp3(const char *filname, int voll, bool loop)
{
  return NULL;
}

#endif // NO_MP3_PLAYER

// pre-loaded (non-streaming) OGG file
struct MYSTATICOGG:public SOUNDCLIP
{
  ALOGG_OGG *tune;
  char *mp3buffer;
  int mp3buffersize;
  int extraOffset;

  int poll()
  {
    if (dont_disturb)
      return done;
    dont_disturb = 1;

    if (tune == NULL)
      ; // Do nothing
    else if (alogg_poll_ogg(tune) == ALOGG_POLL_PLAYJUSTFINISHED) {
      if (!repeat)
        done = 1;
    }
    else get_pos();  // call this to keep the last_but_one stuff up to date

    dont_disturb = 0;

    return done;
  }

  void set_volume(int newvol)
  {
    vol = newvol;

    if (tune != NULL)
    {
      newvol += volModifier + directionalVolModifier;
      if (newvol < 0) newvol = 0;
      alogg_adjust_ogg(tune, newvol, panning, 1000, repeat);
    }
  }

  void destroy()
  {
    if (tune != NULL) {
      alogg_stop_ogg(tune);
      alogg_destroy_ogg(tune);
      tune = NULL;
    }
 
    if (mp3buffer != NULL) {
      sound_cache_free(mp3buffer);
    }
  }

  void seek(int pos)
  {
    // we stop and restart it because otherwise the buffer finishes
    // playing first and the seek isn't quite accurate
    alogg_stop_ogg(tune);
    play_from(pos);
  }

  int get_pos()
  {
    return get_pos_ms();
  }

  int last_but_one_but_one;
  int last_but_one;
  int last_ms_offs;

  int get_pos_ms()
  {
    // Unfortunately the alogg_get_pos_msecs function
    // returns the ms offset that was last decoded, so it's always
    // ahead of the actual playback. Therefore we have this
    // hideous hack below to sort it out.
    if ((done) || (!alogg_is_playing_ogg(tune)))
      return 0;

    AUDIOSTREAM *str = alogg_get_audiostream_ogg(tune);
    long offs = (voice_get_position(str->voice) * 1000) / str->samp->freq;

    if (last_ms_offs != alogg_get_pos_msecs_ogg(tune)) {
      last_but_one_but_one = last_but_one;
      last_but_one = last_ms_offs;
      last_ms_offs = alogg_get_pos_msecs_ogg(tune);
    }

    // just about to switch buffers
    if (offs < 0)
      return last_but_one;

    int end_of_stream = alogg_is_end_of_ogg(tune);

    if ((str->active == 1) && (last_but_one_but_one > 0) && (str->locked == NULL)) {
      switch (end_of_stream) {
      case 0:
      case 2:
        offs -= (last_but_one - last_but_one_but_one);
        break;
      case 1:
        offs -= (last_but_one - last_but_one_but_one);
        break;
      }
    }

/*    char tbuffer[260];
    sprintf(tbuffer,"offs: %d  last_but_one_but_one: %d  last_but_one: %d  active:%d  locked: %p   EOS: %d",
       offs, last_but_one_but_one, last_but_one, str->active, str->locked, end_of_stream);
    write_log(tbuffer);*/

    if (end_of_stream == 1) {
      
      return offs + last_but_one + extraOffset;
    }

    return offs + last_but_one_but_one + extraOffset;
  }

  int get_length_ms()
  {
    return alogg_get_length_msecs_ogg(tune);
  }

  void restart()
  {
    if (tune != NULL) {
      alogg_stop_ogg(tune);
      alogg_rewind_ogg(tune);
      alogg_play_ogg(tune, 16384, vol, panning);
      last_ms_offs = 0;
      last_but_one = 0;
      last_but_one_but_one = 0;
      done = 0;
      poll();
    }
  }

  int get_voice()
  {
    AUDIOSTREAM *ast = alogg_get_audiostream_ogg(tune);
    if (ast)
      return ast->voice;
    return -1;
  }

  int get_sound_type() {
    return MUS_OGG;
  }

  virtual int play_from(int position)
  {
    if (use_extra_sound_offset) 
      extraOffset = ((16384 / (alogg_get_wave_is_stereo_ogg(tune) ? 2 : 1)) * 1000) / alogg_get_wave_freq_ogg(tune);
    else
      extraOffset = 0;

    if (alogg_play_ex_ogg(tune, 16384, vol, panning, 1000, repeat) != ALOGG_OK) {
      destroy();
      delete this;
      return 0;
    }

    last_ms_offs = position;
    last_but_one = position;
    last_but_one_but_one = position;

    if (position > 0)
      alogg_seek_abs_msecs_ogg(tune, position);

    poll();
    return 1;
  }

  int play() {
    return play_from(0);
  }

  MYSTATICOGG() : SOUNDCLIP() {
    last_but_one = 0;
    last_ms_offs = 0;
    last_but_one_but_one = 0;
  }
};

MYSTATICOGG *thissogg;
SOUNDCLIP *my_load_static_ogg(const char *filname, int voll, bool loop)
{
  // Load via soundcache.
  long muslen = 0;
  char* mp3buffer = get_cached_sound(filname, &muslen);
  if (mp3buffer == NULL)
    return NULL;

  // now, create an OGG structure for it
  thissogg = new MYSTATICOGG();
  thissogg->vol = voll;
  thissogg->repeat = loop;
  thissogg->done = 0;
  thissogg->mp3buffer = mp3buffer;
  thissogg->mp3buffersize = muslen;

  thissogg->tune = alogg_create_ogg_from_buffer(mp3buffer, muslen);
  if (thissogg->tune == NULL) {
    thissogg->destroy();
    delete thissogg;
    return NULL;
  }

  return thissogg;
}

struct MYOGG:public SOUNDCLIP
{
  ALOGG_OGGSTREAM *stream;
  PACKFILE *in;
  char *buffer;
  int chunksize;

  int poll()
  {
    if (done)
      return done;
    if (paused)
      return 0;

    if (dont_disturb)
      return done;
    dont_disturb = 1;

    if ((!done) && (in->todo > 0))
    {
      // update the buffer
      char *tempbuf = (char *)alogg_get_oggstream_buffer(stream);
      if (tempbuf != NULL)
      {
        int free_val = -1;
        if (chunksize > in->todo)
        {
          chunksize = in->todo;
          free_val = chunksize;
        }
        pack_fread(tempbuf, chunksize, in);
        alogg_free_oggstream_buffer(stream, free_val);
      }
    }
    if (alogg_poll_oggstream(stream) == ALOGG_POLL_PLAYJUSTFINISHED) {
      done = 1;
    }
    else get_pos_ms();  // call this to keep the last_but_one stuff up to date
    
    dont_disturb = 0;

    return done;
  }

  void set_volume(int newvol)
  {
    // boost MP3 volume
    newvol += 20;
    if (newvol > 255)
      newvol = 255;
    vol = newvol;
    newvol += volModifier + directionalVolModifier;
    if (newvol < 0) newvol = 0;
    alogg_adjust_oggstream(stream, newvol, panning, 1000);
  }

  void destroy()
  {
    if (!done)
      alogg_stop_oggstream(stream);

    alogg_destroy_oggstream(stream);
    stream = NULL;
    if (buffer != NULL)
      free(buffer);
    buffer = NULL;
    pack_fclose(in);
  }

  void seek(int pos)
  {
    quit("Attempted to seek an oggstream; operation not permitted");
  }

  int get_pos()
  {
    return 0;
  }

  int last_but_one_but_one;
  int last_but_one;
  int last_ms_offs;

  int get_pos_ms()
  {
    // Unfortunately the alogg_get_pos_msecs_oggstream function
    // returns the ms offset that was last decoded, so it's always
    // ahead of the actual playback. Therefore we have this
    // hideous hack below to sort it out.
    if ((done) || (!alogg_is_playing_oggstream(stream)))
      return 0;

    AUDIOSTREAM *str = alogg_get_audiostream_oggstream(stream);
    long offs = (voice_get_position(str->voice) * 1000) / str->samp->freq;

    if (last_ms_offs != alogg_get_pos_msecs_oggstream(stream)) {
      last_but_one_but_one = last_but_one;
      last_but_one = last_ms_offs;
      last_ms_offs = alogg_get_pos_msecs_oggstream(stream);
    }

    // just about to switch buffers
    if (offs < 0)
      return last_but_one;

    int end_of_stream = alogg_is_end_of_oggstream(stream);

    if ((str->active == 1) && (last_but_one_but_one > 0) && (str->locked == NULL)) {
      switch (end_of_stream) {
      case 0:
      case 2:
        offs -= (last_but_one - last_but_one_but_one);
        break;
      case 1:
        offs -= (last_but_one - last_but_one_but_one);
        break;
      }
    }

/*    char tbuffer[260];
    sprintf(tbuffer,"offs: %d  last_but_one_but_one: %d  last_but_one: %d  active:%d  locked: %p   EOS: %d",
       offs, last_but_one_but_one, last_but_one, str->active, str->locked, end_of_stream);
    write_log(tbuffer);*/

    if (end_of_stream == 1) {
      
      return offs + last_but_one;
    }

    return offs + last_but_one_but_one;
  }

  int get_length_ms()
  {  // streamed OGG is variable bitrate so we don't know
    return 0;
  }

  void restart()
  {
    if (stream != NULL) {
      // need to reset file pointer for this to work
      quit("Attempted to restart OGG not currently supported");
      alogg_play_oggstream(stream, MP3CHUNKSIZE, vol, panning);
      done = 0;
      paused = 0;
      poll();
    }
  }

  int get_voice()
  {
    AUDIOSTREAM *ast = alogg_get_audiostream_oggstream(stream);
    if (ast)
      return ast->voice;
    return -1;
  }

  int get_sound_type() {
    return MUS_OGG;
  }
  
  int play() {
    alogg_play_oggstream(stream, MP3CHUNKSIZE, (vol > 230) ? vol : vol + 20, panning);

    poll();
    return 1;
  }

  MYOGG() : SOUNDCLIP() {
  }
};

MYOGG *thisogg;
SOUNDCLIP *my_load_ogg(const char *filname, int voll)
{

  mp3in = pack_fopen(filname, "rb");
  if (mp3in == NULL)
    return NULL;

  char *tmpbuffer = (char *)malloc(MP3CHUNKSIZE);
  if (tmpbuffer == NULL) {
    pack_fclose(mp3in);
    return NULL;
  }

  thisogg = new MYOGG();
  thisogg->in = mp3in;
  thisogg->vol = voll;
  thisogg->chunksize = MP3CHUNKSIZE;
  thisogg->done = 0;
  thisogg->last_but_one = 0;
  thisogg->last_ms_offs = 0;
  thisogg->last_but_one_but_one = 0;

  if (thisogg->chunksize > mp3in->todo)
    thisogg->chunksize = mp3in->todo;

  pack_fread(tmpbuffer, thisogg->chunksize, mp3in);

  thisogg->buffer = (char *)tmpbuffer;
  thisogg->stream = alogg_create_oggstream(tmpbuffer, thisogg->chunksize, (mp3in->todo < 1));

  if (thisogg->stream == NULL) {
    free(tmpbuffer);
    pack_fclose(mp3in);
    delete thisogg;
    return NULL;
  }

  return thisogg;
}

// MIDI
struct MYMIDI:public SOUNDCLIP
{
  MIDI *tune;
  int lengthInSeconds;

  int poll()
  {
    if (done)
      return done;

    if (midi_pos < 0)
      done = 1;

    return done;
  }

  void set_volume(int newvol)
  {
    vol = newvol;
    newvol += volModifier + directionalVolModifier;
    if (newvol < 0) newvol = 0;
    ::set_volume(-1, newvol);
  }

  void destroy()
  {
    stop_midi();
    destroy_midi(tune);
    tune = NULL;
  }

  void seek(int pos)
  {
    midi_seek(pos);
  }

  int get_pos()
  {
    return midi_pos;
  }

  int get_pos_ms()
  {
    return 0;                   // we don't know ms with midi
  }

  int get_length_ms()
  {
    return lengthInSeconds * 1000;
  }

  void restart()
  {
    if (tune != NULL) {
      stop_midi();
      done = 0;
      play_midi(tune, 0);
    }
  }

  int get_voice()
  {
    // voice is N/A for midi
    return -1;
  }

  virtual void pause() {
    midi_pause();
  }

  virtual void resume() {
    midi_resume();
  }

  int get_sound_type() {
    return MUS_MIDI;
  }

  int play() {
    lengthInSeconds = get_midi_length(tune);
    if (::play_midi(tune, repeat)) {
      delete this;
      return 0;
    }

    return 1;
  }

  MYMIDI() : SOUNDCLIP() {
    lengthInSeconds = 0;
  }
};

MYMIDI *thismidi;
SOUNDCLIP *my_load_midi(const char *filname, int repet)
{
  MIDI *midiPtr = load_midi(filname);
  if (midiPtr == NULL)
    return NULL;

  thismidi = new MYMIDI();
  thismidi->done = 0;
  thismidi->tune = midiPtr;
  thismidi->repeat = (repet != 0);

  return thismidi;
}


#ifdef JGMOD_MOD_PLAYER

#include "jgmod.h"

// MOD/XM (JGMOD)
struct MYMOD:public SOUNDCLIP
{
  JGMOD *tune;
  int repeat;

  int poll()
  {
    if (done)
      return done;

    if (is_mod_playing() == 0)
      done = 1;

    return done;
  }

  void set_volume(int newvol)
  {
    vol = newvol;
    if (!done)
      set_mod_volume(newvol);
  }

  void destroy()
  {
    stop_mod();
    destroy_mod(tune);
    tune = NULL;
  }

  void seek(int patnum)
  {
    if (is_mod_playing() != 0)
      goto_mod_track(patnum);
  }

  int get_pos()
  {
    if (!is_mod_playing())
      return -1;
    return mi.trk;
  }

  int get_pos_ms()
  {
    return 0;                   // we don't know ms offset
  }

  int get_length_ms()
  {  // we don't know ms
    return 0;
  }

  void restart()
  {
    if (tune != NULL) {
      stop_mod();
      done = 0;
      play_mod(tune, 0);
    }
  }

  int get_voice()
  {
    // MOD uses so many different voices it's not practical to keep track
    return -1;
  }

  int get_sound_type() {
    return MUS_MOD;
  }

  int play() {
    play_mod(tune, repeat);

    return 1;
  }

  MYMOD() : SOUNDCLIP() {
  }
};

MYMOD *thismod = NULL;
SOUNDCLIP *my_load_mod(const char *filname, int repet)
{

  JGMOD *modPtr = load_mod((char *)filname);
  if (modPtr == NULL)
    return NULL;

  thismod = new MYMOD();
  thismod->done = 0;
  thismod->tune = modPtr;
  thismod->repeat = repet;

  return thismod;
}

int init_mod_player(int numVoices) {
  return install_mod(numVoices);
}

void remove_mod_player() {
  remove_mod();
}

#endif   // JGMOD_MOD_PLAYER


#ifdef DUMB_MOD_PLAYER

#include "aldumb.h"

#define VOLUME_TO_DUMB_VOL(vol) ((float)vol) / 256.0

void al_duh_set_loop(AL_DUH_PLAYER *dp, int loop) {
  DUH_SIGRENDERER *sr = al_duh_get_sigrenderer(dp);
  DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(sr);
  if (itsr == NULL)
    return;

  if (loop)
    dumb_it_set_loop_callback(itsr, NULL, NULL);
  else
    dumb_it_set_loop_callback(itsr, dumb_it_callback_terminate, itsr);
}

// MOD/XM (DUMB)
struct MYMOD : public SOUNDCLIP
{
  DUH *tune;
  AL_DUH_PLAYER *duhPlayer;

  int poll()
  {
    if (done)
      return done;

    if (duhPlayer == NULL) {
      done = 1;
      return done;
    }

    if (al_poll_duh(duhPlayer))
      done = 1;

    return done;
  }

  void set_volume(int newvol)
  {
    vol = newvol;

    if (duhPlayer)
    {
      newvol += volModifier + directionalVolModifier;
      if (newvol < 0) newvol = 0;
      al_duh_set_volume(duhPlayer, VOLUME_TO_DUMB_VOL(newvol));
    }
  }

  void destroy()
  {
    if (duhPlayer) {
      al_stop_duh(duhPlayer);
      duhPlayer = NULL;
    }
    if (tune) {
      unload_duh(tune);
      tune = NULL;
    }
  }

  void seek(int patnum)
  {
    if ((!done) && (duhPlayer)) {
      al_stop_duh(duhPlayer);
      done = 0;
      DUH_SIGRENDERER *sr = dumb_it_start_at_order(tune, 2, patnum);
      duhPlayer = al_duh_encapsulate_sigrenderer(sr, VOLUME_TO_DUMB_VOL(vol), 8192, 22050);
      if (!duhPlayer)
        duh_end_sigrenderer(sr);
      else
        al_duh_set_loop(duhPlayer, repeat);
    }
  }

  int get_pos()
  {
    if ((duhPlayer == NULL) || (done))
      return -1;

    // determine the current track number (DUMB calls them 'orders')
    DUH_SIGRENDERER *sr = al_duh_get_sigrenderer(duhPlayer);
    DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(sr);
    if (itsr == NULL)
      return -1;

    return dumb_it_sr_get_current_order(itsr);
  }

  int get_pos_ms()
  {
    return (get_pos() * 10) / 655;
  }

  int get_length_ms()
  {
    if (tune == NULL)
      return 0;

    // duh_get_length represents time as 65536ths of a second
    return (duh_get_length(tune) * 10) / 655;
  }

  void restart()
  {
    if (tune != NULL) {
      al_stop_duh(duhPlayer);
      done = 0;
      duhPlayer = al_start_duh(tune, 2, 0, 1.0, 8192, 22050);
    }
  }

  int get_voice()
  {
    // MOD uses so many different voices it's not practical to keep track
    return -1;
  }

  virtual void pause() {
    if (tune != NULL) {
      al_pause_duh(duhPlayer);
    }
  }

  virtual void resume() {
    if (tune != NULL) {
      al_resume_duh(duhPlayer);
    }
  }

  int get_sound_type() {
    return MUS_MOD;
  }

  int play() {
    duhPlayer = al_start_duh(tune, 2, 0, 1.0, 8192, 22050);
    al_duh_set_loop(duhPlayer, repeat);
    set_volume(vol);

    return 1;
  }  

  MYMOD() : SOUNDCLIP() {
    duhPlayer = NULL;
  }
};

MYMOD *thismod = NULL;
SOUNDCLIP *my_load_mod(const char *filname, int repet)
{

  DUH *modPtr = NULL;
  // determine the file extension
  const char *lastDot = strrchr(filname, '.');
  if (lastDot == NULL)
    return NULL;
  // get the first char of the extensin
  int charAfterDot = toupper(lastDot[1]);

  // use the appropriate loader
  if (charAfterDot == 'I') {
    modPtr = dumb_load_it(filname);
  }
  else if (charAfterDot == 'X') {
    modPtr = dumb_load_xm(filname);
  }
  else if (charAfterDot == 'S') {
    modPtr = dumb_load_s3m(filname);
  }
  else if (charAfterDot == 'M') {
    modPtr = dumb_load_mod(filname);
  }

  if (modPtr == NULL)
    return NULL;

  thismod = new MYMOD();
  thismod->done = 0;
  thismod->tune = modPtr;
  thismod->vol = 255;
  thismod->repeat = (repet != 0);
  
  return thismod;
}

int init_mod_player(int numVoices) {
  dumb_register_packfiles();
  return 0;
}

void remove_mod_player() {
  dumb_exit();
}

#endif  // DUMB_MOD_PLAYER
