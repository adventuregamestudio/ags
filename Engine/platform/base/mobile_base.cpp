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
#include "core/platform.h"

#if AGS_PLATFORM_MOBILE
#include "platform/base/mobile_base.h"

using namespace AGS::Common;


bool WriteConfiguration(const MobileSetup &setup, const char *filename)
{
    ConfigTree cfg;
    CfgWriteInt(cfg, "misc", "config_enabled", setup.config_enabled);
    CfgWriteInt(cfg, "misc", "rotation", setup.rotation);
    CfgWriteString(cfg, "misc", "translation", setup.translation);

    CfgWriteInt(cfg, "controls", "mouse_emulation", setup.mouse_emulation);
    CfgWriteInt(cfg, "controls", "mouse_method", setup.mouse_control_mode);
    CfgWriteInt(cfg, "controls", "mouse_speed", setup.mouse_speed);

    CfgWriteInt(cfg, "compatibility", "clear_cache_on_room_change", setup.clear_cache_on_room_change);

    CfgWriteInt(cfg, "sound", "enabled", setup.audio_enabled);
    CfgWriteInt(cfg, "sound", "cache_size", setup.audio_cachesize);

    CfgWriteInt(cfg, "video", "framedrop", setup.video_framedrop);

    CfgWriteInt(cfg, "graphics", "renderer", setup.gfx_renderer);
    CfgWriteInt(cfg, "graphics", "smoothing", setup.gfx_smoothing);
    CfgWriteInt(cfg, "graphics", "scaling", setup.gfx_scaling);
    CfgWriteInt(cfg, "graphics", "super_sampling", setup.gfx_super_sampling);
    CfgWriteInt(cfg, "graphics", "smooth_sprites", setup.gfx_smooth_sprites);

    CfgWriteInt(cfg, "debug", "show_fps", setup.show_fps);
    CfgWriteInt(cfg, "debug", "logging", setup.debug_write_to_logcat);

    return IniUtil::Merge(filename, cfg);
}

void ResetConfiguration(MobileSetup &setup)
{
    ReadConfiguration(setup, nullptr, true);
}

// Reads config from a given file; pass nullptr instead of filename
// to perform a config reset (all variables will get default values)
bool ReadConfiguration(MobileSetup &setup, const char* filename, bool read_everything)
{
    ConfigTree cfg;
    if (filename && !IniUtil::Read(filename, cfg))
        return false;

    setup.translation = CfgReadString(cfg, "misc", "translation", "default");

    setup.config_enabled = CfgReadBoolInt(cfg, "misc", "config_enabled", false);
    if (!setup.config_enabled && !read_everything)
        return true;

    setup.debug_write_to_logcat = CfgReadBoolInt(cfg, "debug", "logging", false);
    setup.show_fps = CfgReadBoolInt(cfg, "debug", "show_fps", false);

    setup.rotation = CfgReadInt(cfg, "misc", "rotation", 0, 2, 0);

    setup.clear_cache_on_room_change = CfgReadBoolInt(cfg, "compatibility", "clear_cache_on_room_change", false);

    setup.audio_enabled = CfgReadBoolInt(cfg, "sound", "enabled", true);
    setup.audio_cachesize = CfgReadInt(cfg, "sound", "cache_size", 32 * 1024);

    setup.video_framedrop = CfgReadBoolInt(cfg, "video", "framedrop", true);

    setup.gfx_renderer = CfgReadInt(cfg, "graphics", "renderer", 0, 2, 0);
    setup.gfx_smoothing = CfgReadBoolInt(cfg, "graphics", "smoothing", true);
    setup.gfx_scaling = CfgReadInt(cfg, "graphics", "scaling", 0, 2, 1);
    setup.gfx_super_sampling = CfgReadBoolInt(cfg, "graphics", "super_sampling", true);
    setup.gfx_smooth_sprites = CfgReadBoolInt(cfg, "graphics", "smooth_sprites", true);

    setup.mouse_emulation = CfgReadInt(cfg, "controls", "mouse_emulation", 0, 2, 1);
    setup.mouse_control_mode = CfgReadInt(cfg, "controls", "mouse_method", 0, 1, 0);
    setup.mouse_speed = CfgReadInt(cfg, "controls", "mouse_speed", 1, 30, 10);

    return true;
}

void ApplyEngineConfiguration(const MobileSetup &setup, ConfigTree &cfg)
{
    // Mobile ports always run in fullscreen mode
    CfgWriteInt(cfg, "graphics", "windowed", 0);

    // gfx_renderer - rendering mode
    //    * 0 - software renderer
    //    * 1 - hardware, render to screen
    //    * 2 - hardware, render to texture
    if (setup.gfx_renderer == 0)
    {
        CfgWriteString(cfg, "graphics", "driver", "Software");
        CfgWriteInt(cfg, "graphics", "render_at_screenres", 1);
    }
    else
    {
        CfgWriteString(cfg, "graphics", "driver", "OGL");
        CfgWriteInt(cfg, "graphics", "render_at_screenres", setup.gfx_renderer == 1);
    }

    // gfx_scaling - scaling style:
    //    * 0 - no scaling
    //    * 1 - stretch and preserve aspect ratio
    //    * 2 - stretch to whole screen
    if (setup.gfx_scaling == 0)
        CfgWriteString(cfg, "graphics", "game_scale_fs", "1");
    else if (setup.gfx_scaling == 1)
        CfgWriteString(cfg, "graphics", "game_scale_fs", "proportional");
    else
        CfgWriteString(cfg, "graphics", "game_scale_fs", "stretch");

    // gfx_smoothing - scaling filter:
    //    * 0 - nearest-neighbour
    //    * 1 - linear
    if (setup.gfx_smoothing == 0)
        CfgWriteString(cfg, "graphics", "filter", "StdScale");
    else
        CfgWriteString(cfg, "graphics", "filter", "Linear");

    // gfx_super_sampling - enable super sampling,
    //  - only for hardware renderer and when rendering to texture:
    //    * 0 - x1
    //    * 1 - x2
    if (setup.gfx_renderer == 2)
        CfgWriteInt(cfg, "graphics", "supersampling", setup.gfx_super_sampling + 1);
    else
        CfgWriteInt(cfg, "graphics", "supersampling", 0);

    // gfx_rotation - scaling style:
    //    * 0 - unlocked, let the user rotate as wished.
    //    * 1 - portrait
    //    * 2 - landscape
    CfgWriteInt(cfg, "graphics", "rotation", setup.rotation);

    // sound
    CfgWriteInt(cfg, "sound", "enabled", setup.audio_enabled);
    CfgWriteInt(cfg, "sound", "cache_size", setup.audio_cachesize);

    // touch-to-mouse emulation mode
    //    * 0 - off
    //    * 1 - one finger (holding, dragging)
    //    * 2 - two fingers (tapping)
    const char *emul_mode_str[] = { "off", "one_finger", "two_fingers" };
    CfgWriteString(cfg, "touch", "emul_mouse_mode", emul_mode_str[std::max(0, std::min(2, setup.mouse_emulation))]);
    // mouse_control_mode - enable relative mouse mode
    //    * 1 - relative mouse touch controls
    //    * 0 - direct touch mouse control
    CfgWriteInt(cfg, "touch", "emul_mouse_relative", setup.mouse_control_mode);
    CfgWriteFloat(cfg, "mouse", "speed", (float)std::max(setup.mouse_speed, 1)/10.f);

    // translations
    CfgWriteString(cfg, "language", "translation", setup.translation);

    // miscellaneous
    CfgWriteInt(cfg, "misc", "antialias", setup.gfx_smooth_sprites != 0);
    CfgWriteInt(cfg, "misc", "clear_cache_on_room_change", setup.clear_cache_on_room_change != 0);
    CfgWriteInt(cfg, "misc", "load_latest_save", setup.load_latest_savegame != 0);
    CfgWriteInt(cfg, "misc", "show_fps", setup.show_fps);
}

#endif
