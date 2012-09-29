
#include "util/wgt2allg.h"
#include "media/audio/audiodefines.h"
#include "media/audio/clip_mywave.h"
#include "media/audio/audiointernaldefs.h"
#include "media/audio/soundcache.h"

#include "platform/base/agsplatformdriver.h"


int MYWAVE::poll()
{
    _mutex.Lock();

    if (!done && _destroyThis)
    {
      internal_destroy();
      _destroyThis = false;
    }

    if (wave == NULL)
    {
        _mutex.Unlock();
        return 1;
    }
    if (paused)
    {
        _mutex.Unlock();
        return 0;
    }

    if (firstTime) {
        // need to wait until here so that we have been assigned a channel
        //sample_update_callback(wave, voice);
        firstTime = 0;
    }

    if (voice_get_position(voice) < 0)
        done = 1;

    _mutex.Unlock();

    return done;
}

void MYWAVE::set_volume(int newvol)
{
    vol = newvol;

    if (voice >= 0)
    {
        newvol += volModifier + directionalVolModifier;
        if (newvol < 0) newvol = 0;
        voice_set_volume(voice, newvol);
    }
}

void MYWAVE::internal_destroy()
{
    // Stop sound and decrease reference count.
    stop_sample(wave);
    sound_cache_free((char*)wave, true);
    wave = NULL;

    _destroyThis = false;
    done = 1;
}

void MYWAVE::destroy()
{
    _mutex.Lock();

    if (psp_audio_multithreaded)
      _destroyThis = true;
    else
      internal_destroy();

    _mutex.Unlock();

    while (!done)
      AGSPlatformDriver::GetDriver()->YieldCPU();
}

void MYWAVE::seek(int pos)
{
    voice_set_position(voice, pos);
}

int MYWAVE::get_pos()
{
    return voice_get_position(voice);
}

int MYWAVE::get_pos_ms()
{
    // convert the offset in samples into the offset in ms
    //return ((1000000 / voice_get_frequency(voice)) * voice_get_position(voice)) / 1000;

    if (voice_get_frequency(voice) < 100)
        return 0;
    // (number of samples / (samples per second / 100)) * 10 = ms
    return (voice_get_position(voice) / (voice_get_frequency(voice) / 100)) * 10;
}

int MYWAVE::get_length_ms()
{
    if (wave->freq < 100)
        return 0;
    return (wave->len / (wave->freq / 100)) * 10;
}

void MYWAVE::restart()
{
    if (wave != NULL) {
        done = 0;
        paused = 0;
        stop_sample(wave);
        voice = play_sample(wave, vol, panning, 1000, 0);
    }
}

int MYWAVE::get_voice()
{
    return voice;
}

int MYWAVE::get_sound_type() {
    return MUS_WAVE;
}

int MYWAVE::play() {
    voice = play_sample(wave, vol, panning, 1000, repeat);

    return 1;
}

MYWAVE::MYWAVE() : SOUNDCLIP() {
    voice = -1;
}
