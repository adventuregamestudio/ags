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

#include "util/wgt2allg.h"
#include "media/audio/audiodefines.h"
#include "media/audio/soundclip.h"
#include "media/audio/audiointernaldefs.h"

SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1]; // needed for UPDATE_MP3_THREAD macro

int SOUNDCLIP::play_from(int position) 
{
    int retVal = play();
    if ((retVal != 0) && (position > 0))
    {
        seek(position);
    }
    return retVal;
}

void SOUNDCLIP::set_panning(int newPanning) {
    int voice = get_voice();
    if (voice >= 0) {
        voice_set_pan(voice, newPanning);
        panning = newPanning;
    }
}

void SOUNDCLIP::pause() {
    int voice = get_voice();
    if (voice >= 0) {
        voice_stop(voice);
        paused = 1;
    }
}
void SOUNDCLIP::resume() {
    int voice = get_voice();
    if (voice >= 0)
        voice_start(voice);
    paused = 0;
}

SOUNDCLIP::SOUNDCLIP() {
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
    _destroyThis = false;
}

SOUNDCLIP::~SOUNDCLIP()
{
}
