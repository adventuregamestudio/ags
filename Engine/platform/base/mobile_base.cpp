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
#include "platform/base/mobile_base.h"
#include "util/ini_util.h"

using namespace AGS::Common;

// Mobile platform options
int psp_ignore_acsetup_cfg_file = 1;
int psp_clear_cache_on_room_change = 0;
int psp_rotation = 0;
int psp_config_enabled = 0;
char psp_translation[100]{};
char* psp_translations[100]{};

// Mouse option
int config_mouse_control_mode;

// Graphic options
int psp_gfx_scaling;
int psp_gfx_smoothing;

// Audio options from the Allegro library.
unsigned int psp_audio_samplerate = 44100;
int psp_audio_enabled = 1;
int psp_audio_multithreaded = 1;
int psp_audio_cachesize = 10;
int psp_midi_enabled = 1;
int psp_midi_preload_patches = 0;

int psp_video_framedrop = 0;

int psp_gfx_renderer = 0;
int psp_gfx_super_sampling = 0;
int psp_gfx_smooth_sprites = 0;

int psp_debug_write_to_logcat = 1;

int config_mouse_longclick = 0;

// defined in the engine
extern int display_fps;


bool WriteConfiguration(const char *filename)
{
    ConfigTree cfg;
    CfgWriteInt(cfg, "misc", "config_enabled", psp_config_enabled);
    CfgWriteInt(cfg, "misc", "rotation", psp_rotation);
    CfgWriteString(cfg, "misc", "translation", psp_translation);

    CfgWriteInt(cfg, "controls", "mouse_method", config_mouse_control_mode);
    CfgWriteInt(cfg, "controls", "mouse_longclick", config_mouse_longclick);

    CfgWriteInt(cfg, "compatibility", "clear_cache_on_room_change", psp_clear_cache_on_room_change);

    CfgWriteInt(cfg, "sound", "samplerate", psp_audio_samplerate);
    CfgWriteInt(cfg, "sound", "enabled", psp_audio_enabled);
    CfgWriteInt(cfg, "sound", "threaded", psp_audio_multithreaded);
    CfgWriteInt(cfg, "sound", "cache_size", psp_audio_cachesize);

    CfgWriteInt(cfg, "midi", "enabled", psp_midi_enabled);
    CfgWriteInt(cfg, "midi", "preload_patches", psp_midi_preload_patches);

    CfgWriteInt(cfg, "video", "framedrop", psp_video_framedrop);

    CfgWriteInt(cfg, "graphics", "renderer", psp_gfx_renderer);
    CfgWriteInt(cfg, "graphics", "smoothing", psp_gfx_smoothing);
    CfgWriteInt(cfg, "graphics", "scaling", psp_gfx_scaling);
    CfgWriteInt(cfg, "graphics", "super_sampling", psp_gfx_super_sampling);
    CfgWriteInt(cfg, "graphics", "smooth_sprites", psp_gfx_smooth_sprites);

    CfgWriteInt(cfg, "debug", "show_fps", (display_fps == 2) ? 1 : 0);
    CfgWriteInt(cfg, "debug", "logging", psp_debug_write_to_logcat);

    return IniUtil::Merge(filename, cfg);
}

void ResetConfiguration()
{
    ReadConfiguration(nullptr, true);
}

// Reads config from a given file; pass nullptr instead of filename
// to perform a config reset (all variables will get default values)
bool ReadConfiguration(const char* filename, bool read_everything)
{
    ConfigTree cfg;
    if (filename && !IniUtil::Read(filename, cfg))
        return false;

    CfgReadString(&psp_translation[0], sizeof(psp_translation), cfg, "misc", "translation", "default");

    psp_config_enabled = CfgReadBoolInt(cfg, "misc", "config_enabled", false);
    if (!psp_config_enabled && !read_everything)
        return true;

    psp_debug_write_to_logcat = CfgReadBoolInt(cfg, "debug", "logging", false);
    display_fps = CfgReadBoolInt(cfg, "debug", "show_fps", false);
    if (display_fps == 1)
        display_fps = 2;

    psp_rotation = CfgReadInt(cfg, "misc", "rotation", 0, 2, 0);

    psp_clear_cache_on_room_change = CfgReadBoolInt(cfg, "compatibility", "clear_cache_on_room_change", false);

    psp_audio_samplerate = CfgReadInt(cfg, "sound", "samplerate", 0, 44100, 44100);
    psp_audio_enabled = CfgReadBoolInt(cfg, "sound", "enabled", true);
    psp_audio_multithreaded = CfgReadBoolInt(cfg, "sound", "threaded", true);
    psp_audio_cachesize = CfgReadInt(cfg, "sound", "cache_size", 1, 50, 10);

    psp_midi_enabled = CfgReadBoolInt(cfg, "midi", "enabled", true);
    psp_midi_preload_patches = CfgReadBoolInt(cfg, "midi", "preload_patches", false);

    psp_video_framedrop = CfgReadBoolInt(cfg, "video", "framedrop", true);

    psp_gfx_renderer = CfgReadInt(cfg, "graphics", "renderer", 0, 2, 0);
    psp_gfx_smoothing = CfgReadBoolInt(cfg, "graphics", "smoothing", true);
    psp_gfx_scaling = CfgReadInt(cfg, "graphics", "scaling", 0, 2, 1);
    psp_gfx_super_sampling = CfgReadBoolInt(cfg, "graphics", "super_sampling", true);
    psp_gfx_smooth_sprites = CfgReadBoolInt(cfg, "graphics", "smooth_sprites", true);

    config_mouse_control_mode = CfgReadInt(cfg, "controls", "mouse_method", 0, 1, 0);
    config_mouse_longclick = CfgReadBoolInt(cfg, "controls", "mouse_longclick", true);

    return true;
}

#endif
