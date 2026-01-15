//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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
#include "ac/runtime_defines.h"
#include "ac/speech.h"
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
enum MouseSpeedDefinition
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

// Accessibility options are meant to make playing the game easier, by modifying certain
// game properties which are non-critical for the game progression.
struct AccessibilityGameConfig
{
    SkipSpeechStyle SpeechSkipStyle = kSkipSpeechNone; // speech skip style; none here means "use defaults"
    SkipSpeechStyle TextSkipStyle   = kSkipSpeechNone; // display box skip style
    int    TextReadSpeed            = 0; // text reading speed (chars per second)
};

// Override options are for overriding game behavior in various non-standard ways.
struct OverrideGameConfig
{
    int   ScriptOS          = eOS_Unknown; // pretend engine is running on this eScriptSystemOSID
    int8_t Multitasking     = -1; // -1 for none, 0 or 1 to lock in the on/off mode (TODO: make enum)
    bool  NoPlugins         = false; // disable loading plugins
    bool  UpscaleResolution = false; // whether upscale old games that supported that
    // Optional keys for calling built-in save/restore dialogs;
    // primarily meant for the test runs of the games where save functionality
    // is not implemented (or does not work correctly).
    int   KeySaveGame       = eAGSKeyCodeNone;
    int   KeyRestoreGame    = eAGSKeyCodeNone;
    // Optional override for the max save slot
    int   MaxSaveSlot       = 0;
};


struct GameConfig
{
#if AGS_PLATFORM_OS_ANDROID || AGS_PLATFORM_OS_IOS
    static const size_t DefSpriteCacheSize  = (32 * 1024); // 32 MB
#else
    static const size_t DefSpriteCacheSize  = (128 * 1024); // 128 MB
#endif
    static const size_t DefTexCacheSize     = (128 * 1024); // 128 MB
    static const size_t DefSoundLoadAtOnce  = 1024; // 1 MB
    static const size_t DefSoundCache       = 1024u * 32; // 32 MB

    // Display configuration
    DisplayModeSetup Display;
    String  SoftwareRenderDriver;      // Driver for the final output when using Software renderer

    // Graphic options (additional)
    bool    RenderAtScreenRes    = false; // render sprites at screen resolution, as opposed to native one
    bool    AntialiasSprites     = false;  // apply AA (linear) scaling to game sprites, regardless of final filter

    // For mobile devices
    ScreenRotation Rotation      = kScreenRotation_Unlocked; // how to display the game on mobile screen

    // Audio options
    bool    AudioEnabled         = false;
    String  AudioDriverID;
    bool    UseVoicePack         = false;

    // Control options
    bool    MouseAutoLock        = false;
    float   MouseSpeed           = 1.f;
    // Touch-to-mouse emulation mode (how the touches are handled overall)
    TouchMouseEmulation TouchEmulateMouse = kTouchMouse_None; // touch-to-mouse style
    bool    TouchMotionRelative  = false; // touch control abs/relative mode
    // Mouse-to-touch emulation (LMB creates touch events)
    bool    MouseEmulateTouch    = false;

    // Cache options
    size_t  SpriteCacheSize      = DefSpriteCacheSize; // in KB
    size_t  TextureCacheSize     = DefTexCacheSize; // in KB
    size_t  SoundCacheSize       = DefSoundCache; // sound cache limit, in KB
    size_t  SoundLoadAtOnceSize  = DefSoundLoadAtOnce; // threshold for loading sounds immediately, in KB

    // Misc options
    String  Translation;

    // Custom paths
    String  UserConfPath; // a read-only config path (if set the regular config is ignored)
    String  UserConfDir;  // directory to read and write user config in
    String  UserSaveDir;  // directory to write savedgames and user files to
    String  AppDataDir;   // directory to write shared game files to

    // Misc engine options
    bool    LoadLatestSave       = false; // load latest saved game on launch
    bool    CompressSaves        = true;
    bool    ClearCacheOnRoomChange = false; // for low-end devices: clear resource caches on room change
    bool    RunInBackground      = false; // whether run on background, when game is switched out
    bool    ShowFps              = false;

    // Accessibility options
    AccessibilityGameConfig Access;

    GameConfig() = default;
};

// TODO: reconsider the purpose of this struct in the future.
// Currently it's been used as both initial storage for config options
// before they are used to initialize engine, and as persistent storage
// for options that may be changed at runtime (and later written back
// to the config file).
struct GameSetup : public GameConfig
{
    // Disables handling exceptions and displaying exception message on exit
    bool   DisableExceptionHandling = false;

    // Game data paths, some are calculated from enviroment,
    // other may be use-defined or passed from IDE when doing a test run.
    String  StartupDir; // directory where the default game config is located (usually same as MainDataDir)
    String  MainDataDir; // main data directory
    String  MainDataFile; // full path to main data file
    // Optional dirs are currently for compatibility with Editor only (debug runs);
    // but could be used for something else in theory.
    String  OptInstallDir; // optional custom install dir path (also used as extra data dir)
    std::vector<std::pair<String, String>> OptDataDirs; // optional data dirs with asset filters
    
    // More precise control over how the mouse speed is applied.
    // CHECKME: review this, may be redundant after we upgraded to SDL2 (or not...)
    MouseControlWhen MouseCtrlWhen = kMouseCtrl_Never; // when to apply mouse control (as opposed to system behavior)
    bool    MouseCtrlEnabled = false; // whether mouse control is to be enabled (applying speed)
    MouseSpeedDefinition MouseSpeedDef = kMouseSpeed_Absolute; // meaning of mouse speed (absolute vs relative)

    // User's overrides and hacks
    OverrideGameConfig Override;

    GameSetup() = default;
};

// TODO: setup object is used for two purposes: temporarily storing config
// options before engine is initialized, and storing certain runtime variables.
// Perhaps it makes sense to separate those two group of vars at some point.
extern GameSetup usetup;

#endif // __AC_GAMESETUP_H
