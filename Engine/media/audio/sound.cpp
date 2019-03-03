//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// ACSOUND - AGS sound system wrapper
//
//=============================================================================

#include "util/wgt2allg.h"
#include "ac/file.h"
#include "media/audio/audiodefines.h"
#include "media/audio/sound.h"
#include "media/audio/audiointernaldefs.h"
#include "media/audio/clip_mywave.h"
#ifndef NO_MP3_PLAYER
#include "media/audio/clip_mymp3.h"
#include "media/audio/clip_mystaticmp3.h"
#endif
#include "media/audio/clip_myogg.h"
#include "media/audio/clip_mystaticogg.h"
#include "media/audio/clip_mymidi.h"
#ifdef DUMB_MOD_PLAYER
#include "media/audio/clip_mydumbmod.h"
#endif
#include "media/audio/soundcache.h"

#if defined JGMOD_MOD_PLAYER && defined DUMB_MOD_PLAYER
#error JGMOD_MOD_PLAYER and DUMB_MOD_PLAYER macros cannot be defined at the same time.
#endif

#if !defined PSP_NO_MOD_PLAYBACK && !defined JGMOD_MOD_PLAYER && !defined DUMB_MOD_PLAYER
#error Either JGMOD_MOD_PLAYER or DUMB_MOD_PLAYER should be defined.
#endif

extern "C"
{
// Load MIDI from PACKFILE stream
MIDI *load_midi_pf(PACKFILE *pf);
}

#if !defined (WINDOWS_VERSION)
// for toupper
#include <ctype.h>
#endif


int use_extra_sound_offset = 0;



MYWAVE *thiswave;
SOUNDCLIP *my_load_wave(const AssetPath &asset_name, int voll, int loop)
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN

    // Load via soundcache.
    long dummy;
    SAMPLE *new_sample = (SAMPLE*)get_cached_sound(asset_name, true, &dummy);

    if (new_sample == NULL)
        return NULL;

    thiswave = new MYWAVE();
    thiswave->wave = new_sample;
    thiswave->vol = voll;
    thiswave->firstTime = 1;
    thiswave->repeat = (loop != 0);

    return thiswave;
}

PACKFILE *mp3in;

#ifndef NO_MP3_PLAYER

MYMP3 *thistune;
SOUNDCLIP *my_load_mp3(const AssetPath &asset_name, int voll)
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN

    mp3in = PackfileFromAsset(asset_name);
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
SOUNDCLIP *my_load_static_mp3(const AssetPath &asset_name, int voll, bool loop)
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN

    // Load via soundcache.
    long muslen = 0;
    char* mp3buffer = get_cached_sound(asset_name, false, &muslen);
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

#else // NO_MP3_PLAYER

SOUNDCLIP *my_load_mp3(const AssetPath &asset_name, int voll)
{
    return NULL;
}

SOUNDCLIP *my_load_static_mp3(const AssetPath &asset_name, int voll, bool loop)
{
    return NULL;
}

#endif // NO_MP3_PLAYER



MYSTATICOGG *thissogg;
SOUNDCLIP *my_load_static_ogg(const AssetPath &asset_name, int voll, bool loop)
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN

    // Load via soundcache.
    long muslen = 0;
    char* mp3buffer = get_cached_sound(asset_name, false, &muslen);
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

MYOGG *thisogg;
SOUNDCLIP *my_load_ogg(const AssetPath &asset_name, int voll)
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN

    mp3in = PackfileFromAsset(asset_name);
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
SOUNDCLIP *my_load_midi(const AssetPath &asset_name, int repet)
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN

    // The first a midi is played, preload all patches.
    if (!thismidi && psp_midi_preload_patches)
        load_midi_patches();

    PACKFILE *pf = PackfileFromAsset(asset_name);
    if (!pf)
        return NULL;

    MIDI* midiPtr = load_midi_pf(pf);
    pack_fclose(pf);

    if (midiPtr == NULL)
        return NULL;

    thismidi = new MYMIDI();
    thismidi->done = 0;
    thismidi->tune = midiPtr;
    thismidi->repeat = (repet != 0);

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
    thismod->repeat = (repet != 0);

    return thismod;
}

int init_mod_player(int numVoices) {
    return install_mod(numVoices);
}

void remove_mod_player() {
    remove_mod();
}

//#endif   // JGMOD_MOD_PLAYER
#elif defined DUMB_MOD_PLAYER

MYMOD *thismod = NULL;
SOUNDCLIP *my_load_mod(const AssetPath &asset_name, int repet)
{
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN

    DUMBFILE *df = DUMBfileFromAsset(asset_name);
    if (!df)
        return NULL;

    DUH *modPtr = NULL;
    // determine the file extension
    const char *lastDot = strrchr(asset_name.second, '.');
    if (lastDot == NULL)
    {
        dumbfile_close(df);
        return NULL;
    }
    // get the first char of the extensin
    int charAfterDot = toupper(lastDot[1]);

    // use the appropriate loader
    if (charAfterDot == 'I') {
        modPtr = dumb_read_it(df);
    }
    else if (charAfterDot == 'X') {
        modPtr = dumb_read_xm(df);
    }
    else if (charAfterDot == 'S') {
        modPtr = dumb_read_s3m(df);
    }
    else if (charAfterDot == 'M') {
        modPtr = dumb_read_mod(df);
    }

    dumbfile_close(df);
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
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    dumb_register_packfiles();
    return 0;
}

void remove_mod_player() {
    AGS_AUDIO_SYSTEM_CRITICAL_SECTION_BEGIN
    dumb_exit();
}

#endif  // DUMB_MOD_PLAYER
