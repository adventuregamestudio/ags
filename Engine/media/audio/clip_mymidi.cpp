
#include "ac/audiodefines.h"
#include "util/wgt2allg.h"
#include "media/audio/clip_mymidi.h"
#include "media/audio/audiointernaldefs.h"

int MYMIDI::poll()
{
    if (initializing)
        return 0;

    if (done)
        return done;

    if (midi_pos < 0)
        done = 1;

    return done;
}

void MYMIDI::set_volume(int newvol)
{
    vol = newvol;
    newvol += volModifier + directionalVolModifier;
    if (newvol < 0) newvol = 0;
    ::set_volume(-1, newvol);
}

void MYMIDI::destroy()
{
    stop_midi();
    destroy_midi(tune);
    tune = NULL;
}

void MYMIDI::seek(int pos)
{
    midi_seek(pos);
}

int MYMIDI::get_pos()
{
    return midi_pos;
}

int MYMIDI::get_pos_ms()
{
    return 0;                   // we don't know ms with midi
}

int MYMIDI::get_length_ms()
{
    return lengthInSeconds * 1000;
}

void MYMIDI::restart()
{
    if (tune != NULL) {
        stop_midi();
        done = 0;
        play_midi(tune, 0);
    }
}

int MYMIDI::get_voice()
{
    // voice is N/A for midi
    return -1;
}

void MYMIDI::pause() {
    midi_pause();
}

void MYMIDI::resume() {
    midi_resume();
}

int MYMIDI::get_sound_type() {
    return MUS_MIDI;
}

int MYMIDI::play() {
    lengthInSeconds = get_midi_length(tune);
    if (::play_midi(tune, repeat)) {
        delete this;
        return 0;
    }
    initializing = false;

    return 1;
}

MYMIDI::MYMIDI() : SOUNDCLIP() {
    lengthInSeconds = 0;
}
