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

#include "util/wgt2allg.h" // DIGI_AUTODETECT & MIDI_AUTODETECT
#include "ac/gamesetup.h"

GameSetup::GameSetup()
{
    digicard=DIGI_AUTODETECT; midicard=MIDI_AUTODETECT;
    mod_player=1; mp3_player=1;
    want_letterbox=0; windowed = 0;
    no_speech_pack = 0;
    refresh = 0;
    enable_antialiasing = 0;
    force_hicolor_mode = 0;
    disable_exception_handling = 0;
    enable_side_borders = 1;
    base_width = 320;
    base_height = 200;
    override_script_os = -1;
    override_multitasking = -1;
    override_upscale = false;
}
