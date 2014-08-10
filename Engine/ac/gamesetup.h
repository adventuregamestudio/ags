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

#ifndef __AC_GAMESETUP_H
#define __AC_GAMESETUP_H

#include "util/geometry.h"
#include "util/string.h"

enum ScreenSizeDefinition
{
    kScreenDef_Explicit,        // define by width & height
    kScreenDef_ByGameScaling,   // define by game scale factor
    kScreenDef_MaxDisplay,      // set to maximal supported (desktop/device screen size)
    kNumScreenDef
};

struct GameSetup {
    int digicard;
    int midicard;
    int mod_player;
    int textheight;
    int mp3_player;
    bool windowed;
    bool vsync;
    short refresh;
    bool  no_speech_pack;
    bool  enable_antialiasing;
    bool  force_hicolor_mode;
    bool  disable_exception_handling;
    AGS::Common::String data_files_dir;
    AGS::Common::String main_data_filename;
    char *translation;
    AGS::Common::String gfxFilterID;
    AGS::Common::String gfxDriverID;
    int   override_script_os;
    char  override_multitasking;
    bool  override_upscale;
    ScreenSizeDefinition screen_sz_def;
    Size  screen_size;
    int filter_scaling_x;
    int filter_scaling_y;
    bool filter_scaling_max_uniform;
    bool match_device_ratio;
    RectPlacement game_frame_placement;

    GameSetup();
};

extern GameSetup usetup;

#endif // __AC_GAMESETUP_H