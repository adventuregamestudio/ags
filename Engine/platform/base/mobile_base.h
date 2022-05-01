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
#include "core/platform.h"

#if (AGS_PLATFORM_OS_ANDROID) || (AGS_PLATFORM_OS_IOS)

#include "util/ini_util.h"

// Mobile platform options
struct MobileSetup
{
    AGS::Common::String game_file_name;

    // Mobile platform options
    int ignore_acsetup_cfg_file = 0;
    int clear_cache_on_room_change = 0;
    int rotation = 0;
    int config_enabled = 0;
    AGS::Common::String translation;

    // Mouse option
    int mouse_control_mode = 0;

    // Graphic options
    int gfx_scaling = 0;
    int gfx_smoothing = 0;

    // Audio options from the Allegro library.
    unsigned int audio_samplerate = 0;
    int audio_enabled = 0;
    int audio_multithreaded = 0;
    int audio_cachesize = 0;
    int midi_enabled = 0;
    int midi_preload_patches = 0;

    // Video playback options
    int video_framedrop = 0;

    int gfx_renderer = 0;
    int gfx_super_sampling = 0;
    int gfx_smooth_sprites = 0;

    int debug_write_to_logcat = 0;

    int mouse_longclick = 0;

    int display_fps = 0;

    bool load_latest_savegame = false;
};

// Writes mobile platform config
bool WriteConfiguration(const MobileSetup &setup, const char *filename);
// Reads mobile platform config
bool ReadConfiguration(MobileSetup &setup, const char *filename, bool read_everything);
// Reset config variables
void ResetConfiguration(MobileSetup &setup);
// Copy mobile options to the config tree meant for the engine
void ApplyEngineConfiguration(const MobileSetup &setup, AGS::Common::ConfigTree &cfg);

#endif
