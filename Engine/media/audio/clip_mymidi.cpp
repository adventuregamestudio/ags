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

#include "media/audio/audiodefines.h"
#include "util/wgt2allg.h"
#include "media/audio/clip_mymidi.h"
#include "media/audio/audiointernaldefs.h"

int MYMIDI::poll()
{
    if (done || !tune) {
        done = 1;
        return done;
    }

    if (midi_pos < 0)
        done = 1;

    return done;
}

void MYMIDI::adjust_volume()
{
    ::set_volume(-1, get_final_volume());
}

void MYMIDI::set_volume(int newvol)
{
    vol = newvol;
    adjust_volume();
}

void MYMIDI::destroy()
{
    stop_midi();
    if (tune) {
        destroy_midi(tune);
    }
    tune = nullptr;
    done = 1;
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
    if (!tune) { return 0; }
    lengthInSeconds = get_midi_length(tune);
    if (::play_midi(tune, repeat)) {
        delete this;
        return 0;
    }

    return 1;
}

MYMIDI::MYMIDI() : SOUNDCLIP() {
    tune = NULL;
    lengthInSeconds = 0;
}
