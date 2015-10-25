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

#include "util/string.h"

// game setup, read in from CFG file
// this struct is redefined in acdialog.cpp, any changes might
// need to be reflected there
// [IKM] 2012-06-27: now it isn't
struct GameSetup {
    int digicard;
    int midicard;
    int mod_player;
    int textheight;
    int mp3_player;
    bool want_letterbox; // defines whether game is displayed in letterboxed mode
    bool windowed;
    int base_width;
    int base_height;
    short refresh;
    bool  no_speech_pack;
    bool  enable_antialiasing;
    bool  force_hicolor_mode;
    bool  disable_exception_handling;
    bool  prefer_sideborders; // defines whether it is preferred to have side borders
    bool  prefer_letterbox;  // defines whether it is preferred to have letterbox
    AGS::Common::String data_files_dir;
    AGS::Common::String main_data_filename;
    AGS::Common::String translation;
    AGS::Common::String gfxFilterID;
    AGS::Common::String gfxDriverID;
    int   override_script_os;
    char  override_multitasking;
    bool  override_upscale;
    GameSetup();
};

extern GameSetup usetup;

#endif // __AC_GAMESETUP_H