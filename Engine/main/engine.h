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
#ifndef __AGS_EE_MAIN__ENGINE_H
#define __AGS_EE_MAIN__ENGINE_H

#include "util/ini_util.h"

const char *get_engine_name();
const char *get_engine_version();
AGS::Common::String get_engine_version_and_build();
void        show_preload();
void        engine_init_game_settings();
int         initialize_engine(const AGS::Common::ConfigTree &startup_opts);

struct DisplayModeSetup;
// Try to set new graphics mode deduced from given configuration;
// if requested mode fails, tries to find any compatible mode close to the
// requested one.
bool        engine_try_set_gfxmode_any(const DisplayModeSetup &setup);
// Tries to switch between fullscreen and windowed mode; uses previously saved
// setup if it is available, or default settings for the new mode
bool        engine_try_switch_windowed_gfxmode();
// Update graphic renderer and render frame when window size changes
void        engine_on_window_changed(const Size &sz);
// Shutdown graphics mode (used before shutting down tha application)
void        engine_shutdown_gfxmode();

using AGS::Common::String;
// Defines a package file location
struct PackLocation
{
    String Name; // filename, for the reference or to use as an ID
    String Path; // full path
};
// Game resource paths
// TODO: the asset path configuration should certainly be revamped at some
// point, with uniform method of configuring auxiliary paths and packages.
struct ResourcePaths
{
    PackLocation GamePak;    // main game package
    PackLocation AudioPak;   // audio package
    PackLocation SpeechPak;  // voice-over package
    String       DataDir;    // path to the data directory
    bool         VoiceAvail = false; // tells whether voice files available in either location
    // NOTE: optional directories are currently only for compatibility with Editor (game test runs)
    // This is bit ugly, but remain so until more flexible configuration is designed
    String       DataDir2;   // optional data directory
    String       AudioDir2;  // optional audio directory
    String       VoiceDir2;  // optional voice-over directory (base)
    String       VoiceDirSub;// full voice-over directory with optional sub-dir
};
extern ResourcePaths ResPaths;

// (Re-)Assign all known asset search paths to the AssetManager
void engine_assign_assetpaths();

// Register a callback that will be called before engine is initialised.
// Used for apps to register their own plugins and other configuration
typedef void (*t_engine_pre_init_callback)(void);
extern void engine_set_pre_init_callback(t_engine_pre_init_callback callback);

#endif // __AGS_EE_MAIN__ENGINE_H
