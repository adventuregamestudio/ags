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

#include "main/graphics_mode.h"


// Mouse control activation type
enum MouseControl
{
    kMouseCtrl_Never,       // never control mouse (track system mouse position)
    kMouseCtrl_Fullscreen,  // control mouse in fullscreen only
    kMouseCtrl_Always,      // always control mouse (fullscreen and windowed)
    kNumMouseCtrlOptions
};

// Mouse speed definition, specifies how the speed setting is applied to the mouse movement
enum MouseSpeedDef
{
    kMouseSpeed_Absolute,       // apply speed multiplier directly
    kMouseSpeed_CurrentDisplay, // keep speed/resolution relation based on current system display mode
    kNumMouseSpeedDefs
};

struct GameSetup {
    int digicard;
    int midicard;
    int mod_player;
    int textheight; // text height used on the certain built-in GUI
    bool  no_speech_pack;
    bool  enable_antialiasing;
    bool  force_hicolor_mode;
    bool  disable_exception_handling;
    AGS::Common::String data_files_dir;
    AGS::Common::String main_data_filename;
    AGS::Common::String install_dir; // optional custom install dir path
    AGS::Common::String install_audio_dir; // optional custom install audio dir path
    AGS::Common::String install_voice_dir; // optional custom install voice-over dir path
    AGS::Common::String user_data_dir; // directory to write savedgames and user files to
    AGS::Common::String shared_data_dir; // directory to write shared game files to
    AGS::Common::String translation;
    bool  mouse_auto_lock;
    int   override_script_os;
    char  override_multitasking;
    bool  override_upscale;
    float mouse_speed;
    MouseControl mouse_control;
    MouseSpeedDef mouse_speed_def;
    bool  RenderAtScreenRes; // render sprites at screen resolution, as opposed to native one

    ScreenSetup Screen;

    GameSetup();
};

// TODO: setup object is used for two purposes: temporarily storing config
// options before engine is initialized, and storing certain runtime variables.
// Perhaps it makes sense to separate those two group of vars at some point.
extern GameSetup usetup;

#endif // __AC_GAMESETUP_H