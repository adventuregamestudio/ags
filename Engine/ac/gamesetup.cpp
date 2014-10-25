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
    digicard=DIGI_AUTODETECT;
    midicard=MIDI_AUTODETECT;
    mod_player=1;
    mp3_player=1;
    windowed = false;
    vsync = false;
    no_speech_pack = false;
    refresh = 0;
    enable_antialiasing = false;
    force_hicolor_mode = false;
    disable_exception_handling = false;
    override_script_os = -1;
    override_multitasking = -1;
    override_upscale = false;
    gfxFilterID = "StdScale";
    screen_sz_def = kScreenDef_MaxDisplay;
    filter_scaling_max_uniform = true;
    filter_scaling_x = 0;
    filter_scaling_y = 0;
    match_device_ratio = false;
    game_frame_placement = kPlaceCenter;
}
