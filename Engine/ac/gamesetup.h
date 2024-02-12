//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_GAMESETUP_H
#define __AC_GAMESETUP_H

#include "ac/game_version.h"
#include "ac/sys_events.h"
#include "main/graphics_mode.h"
#include "util/string.h"


// Mouse control activation type
enum MouseControlWhen
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

// Screen rotation mode on supported platforms and devices
enum ScreenRotation
{
    kScreenRotation_Unlocked = 0,     // player can freely rotate the screen if possible
    kScreenRotation_Portrait,         // screen can only be in portrait orientation
    kScreenRotation_Landscape,        // screen can only be in landscape orientation
    kNumScreenRotationOptions
};

using AGS::Common::String;

// TODO: reconsider the purpose of this struct in the future.
// Currently it's been used as both initial storage for config options
// before they are used to initialize engine, and as persistent storage
// for options that may be changed at runtime (and later written back
// to the config file).
struct GameSetup
{
#if AGS_PLATFORM_OS_ANDROID || AGS_PLATFORM_OS_IOS
    static const size_t DefSpriteCacheSize = (32 * 1024); // 32 MB
#else
    static const size_t DefSpriteCacheSize = (128 * 1024); // 128 MB
#endif
    static const size_t DefTexCacheSize = (128 * 1024); // 128 MB
    static const size_t DefSoundLoadAtOnce = 1024; // 1 MB
    static const size_t DefSoundCache = 1024u * 32; // 32 MB


    bool  audio_enabled;
    String audio_driver;
    int   textheight; // text height used on the certain built-in GUI // TODO: move out to game class?
    bool  no_speech_pack;
    bool  enable_antialiasing;
    bool  disable_exception_handling;
    String startup_dir; // directory where the default game config is located (usually same as main_data_dir)
    String main_data_dir; // main data directory
    String main_data_file; // full path to main data file
    // Following 4 optional dirs are currently for compatibility with Editor only (debug runs)
    // This is bit ugly, but remain so until more flexible configuration is designed
    String install_dir; // optional custom install dir path (also used as extra data dir)
    String opt_data_dir; // optional data dir number 2
    String opt_audio_dir; // optional custom install audio dir path
    String opt_voice_dir; // optional custom install voice-over dir path
    //
    String conf_path; // a read-only config path (if set the regular config is ignored)
    String user_conf_dir; // directory to read and write user config in
    String user_data_dir; // directory to write savedgames and user files to
    String shared_data_dir; // directory to write shared game files to
    String translation;
    bool  mouse_auto_lock;
    float mouse_speed;
    MouseControlWhen mouse_ctrl_when;
    bool  mouse_ctrl_enabled;
    MouseSpeedDef mouse_speed_def;
    // touch-to-mouse emulation mode (how the touches are handled overall)
    TouchMouseEmulation touch_emulate_mouse;
    // touch control abs/relative mode
    bool  touch_motion_relative;
    //
    bool  RenderAtScreenRes; // render sprites at screen resolution, as opposed to native one
    int   Supersampling;
    size_t SpriteCacheSize = DefSpriteCacheSize; // in KB
    size_t TextureCacheSize = DefTexCacheSize; // in KB
    size_t SoundLoadAtOnceSize = DefSoundLoadAtOnce; // threshold for loading sounds immediately, in KB
    size_t SoundCacheSize = DefSoundCache; // sound cache limit, in KB
    bool  clear_cache_on_room_change; // for low-end devices: clear resource caches on room change
    bool  load_latest_save; // load latest saved game on launch
    ScreenRotation rotation;
    bool  show_fps;
    bool  multitasking = false; // whether run on background, when game is switched out

    DisplayModeSetup Screen;
    String software_render_driver;

    // User's overrides and hacks
    int   override_script_os; // pretend engine is running on this eScriptSystemOSID
    char  override_multitasking; // -1 for none, 0 or 1 to lock in the on/off mode
    bool  override_upscale; // whether upscale old games that supported that
    // Optional keys for calling built-in save/restore dialogs;
    // primarily meant for the test runs of the games where save functionality
    // is not implemented (or does not work correctly).
    int   key_save_game = 0;
    int   key_restore_game = 0;

    GameSetup();
};

// TODO: setup object is used for two purposes: temporarily storing config
// options before engine is initialized, and storing certain runtime variables.
// Perhaps it makes sense to separate those two group of vars at some point.
extern GameSetup usetup;

#endif // __AC_GAMESETUP_H