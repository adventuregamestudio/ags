/*
ACSOUND - AGS sound system wrapper

Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
All rights reserved.

The AGS Editor Source Code is provided under the Artistic License 2.0
http://www.opensource.org/licenses/artistic-license-2.0.php

You MAY NOT compile your own builds of the engine without making it EXPLICITLY
CLEAR that the code has been altered from the Standard Version.
*/
#include "wgt2allg.h"
#include "acaudio/ac_sound.h"
#include "acaudio/ac_soundinternaldefs.h"
#include "acaudio/ac_soundcache.h"
#include "acaudio/ac_mywave.h"
#ifndef NO_MP3_PLAYER
#include "acaudio/ac_mymp3.h"
#include "acaudio/ac_mystaticmp3.h"
#endif
#include "acaudio/ac_myogg.h"
#include "acaudio/ac_mystaticogg.h"
#include "acaudio/ac_mymidi.h"
#ifdef JGMOD_MOD_PLAYER
#include "acaudio/ac_myjgmod.h"
#endif
#ifdef DUMB_MOD_PLAYER
#include "acaudio/ac_mydumbmod.h"
#endif



#if defined(MAC_VERSION) || defined(LINUX_VERSION)
// for toupper
#include <ctype.h>
#endif

/*
extern "C" {
    extern int alogg_is_end_of_oggstream(ALOGG_OGGSTREAM *ogg);
    extern int alogg_is_end_of_ogg(ALOGG_OGG *ogg);
    extern int alogg_get_ogg_freq(ALOGG_OGG *ogg);
    extern int alogg_get_ogg_stereo(ALOGG_OGG *ogg);
}


extern int use_extra_sound_offset;
extern int our_eip;
extern void quit(char *);
extern void write_log(char*msg) ;
//extern void sample_update_callback(SAMPLE *sample, int voice);
*/



MYWAVE *thiswave;
SOUNDCLIP *my_load_wave(const char *filename, int voll, int loop)
{
#ifdef MAC_VERSION
    SAMPLE *new_sample = load_wav(filename);
#else
    // Load via soundcache.
    long dummy;
    SAMPLE *new_sample = (SAMPLE*)get_cached_sound(filename, true, &dummy);
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

#ifndef NO_MP3_PLAYER

PACKFILE *mp3in;

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



MYSTATICMP3 *thismp3;
SOUNDCLIP *my_load_static_mp3(const char *filname, int voll, bool loop)
{
    // Load via soundcache.
    long muslen = 0;
    char* mp3buffer = get_cached_sound(filname, false, &muslen);
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
    thismp3->ready = true;

    if (thismp3->tune == NULL) {
        free(mp3buffer);
        delete thismp3;
        return NULL;
    }

    thismp3->mp3buffer = mp3buffer;

    return thismp3;
}

#else // NO_MP3_PLAYER

SOUNDCLIP *my_load_mp3(const char *filname, int voll)
{
    return NULL;
}

SOUNDCLIP *my_load_static_mp3(const char *filname, int voll, bool loop)
{
    return NULL;
}

#endif // NO_MP3_PLAYER



MYSTATICOGG *thissogg;
SOUNDCLIP *my_load_static_ogg(const char *filname, int voll, bool loop)
{
    // Load via soundcache.
    long muslen = 0;
    char* mp3buffer = get_cached_sound(filname, false, &muslen);
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
    thissogg->ready = true;

    if (thissogg->tune == NULL) {
        thissogg->destroy();
        delete thissogg;
        return NULL;
    }

    return thissogg;
}

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



MYMIDI *thismidi;
SOUNDCLIP *my_load_midi(const char *filname, int repet)
{
    // The first a midi is played, preload all patches.
    if (!thismidi && psp_midi_preload_patches)
        load_midi_patches();

    MIDI* midiPtr = load_midi(filname);

    if (midiPtr == NULL)
        return NULL;

    thismidi = new MYMIDI();
    thismidi->done = 0;
    thismidi->tune = midiPtr;
    thismidi->repeat = (repet != 0);
    thismidi->initializing = true;

    return thismidi;
}


#ifdef JGMOD_MOD_PLAYER

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
