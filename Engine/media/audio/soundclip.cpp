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
#include "media/audio/audio.h"
#include "media/audio/audiodefines.h"
#include "media/audio/soundclip.h"
#include "media/audio/audiointernaldefs.h"

SOUNDCLIP::SOUNDCLIP() {
    priority = 50;
    panning = 128;
    panningAsPercentage = 0;
    speed = 1000;
    sourceClipType = 0;
    sourceClip = nullptr;
    vol = 0;
    volAsPercentage = 0;
    volModifier = 0;
    muted = false;
    repeat = false;
    xSource = -1;
    ySource = -1;
    maximumPossibleDistanceAway = 0;
    directionalVolModifier = 0;
}

SOUNDCLIP::~SOUNDCLIP() = default;
