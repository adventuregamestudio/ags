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
