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

//
// Game configuration
//
#include <ctype.h> // toupper
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/path_helper.h"
#include "ac/spritecache.h"
#include "ac/system.h"
#include "core/platform.h"
#include "debug/debugger.h"
#include "debug/debug_log.h"
#include "device/mousew32.h"
#include "main/config.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "util/directory.h"
#include "util/ini_util.h"
#include "util/textstreamreader.h"
#include "util/path.h"
#include "util/string_utils.h"


using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern GameSetup usetup;
extern SpriteCache spriteset;
extern GameState play;

// Filename of the default config file, the one found in the game installation
const String DefaultConfigFileName = "acsetup.cfg";

bool INIreaditem(const ConfigTree &cfg, const String &sectn, const String &item, String &value)
{
    ConfigNode sec_it = cfg.find(sectn);
    if (sec_it != cfg.end())
    {
        StrStrOIter item_it = sec_it->second.find(item);
        if (item_it != sec_it->second.end())
        {
            value = item_it->second;
            return true;
        }
    }
    return false;
}

int INIreadint(const ConfigTree &cfg, const String &sectn, const String &item, int def_value)
{
    String str;
    if (!INIreaditem(cfg, sectn, item, str))
        return def_value;

    return atoi(str.GetCStr());
}

float INIreadfloat(const ConfigTree &cfg, const String &sectn, const String &item, float def_value)
{
    String str;
    if (!INIreaditem(cfg, sectn, item, str))
        return def_value;

    return atof(str.GetCStr());
}

String INIreadstring(const ConfigTree &cfg, const String &sectn, const String &item, const String &def_value)
{
    String str;
    if (!INIreaditem(cfg, sectn, item, str))
        return def_value;
    return str;
}

void INIwriteint(ConfigTree &cfg, const String &sectn, const String &item, int value)
{
    cfg[sectn][item] = StrUtil::IntToString(value);
}

void INIwritestring(ConfigTree &cfg, const String &sectn, const String &item, const String &value)
{
    cfg[sectn][item] = value;
}


WindowSetup parse_window_mode(const String &option, bool as_windowed, WindowSetup def_value)
{
    // "full_window" option means pseudo fullscreen ("borderless fullscreen window")
    if (!as_windowed && (option.CompareNoCase("full_window") == 0))
        return WindowSetup(kWnd_FullDesktop);
    // Check supported options for explicit resolution or scale factor,
    // in which case we'll use either a resizing window or a REAL fullscreen mode
    const WindowMode exp_wmode = as_windowed ? kWnd_Windowed : kWnd_Fullscreen;
    // Note that for "desktop" we return "default" for windowed, this will result
    // in refering to the  desktop size but resizing in accordance to the scaling style
    if (option.CompareNoCase("desktop") == 0)
        return as_windowed ? WindowSetup(exp_wmode) : WindowSetup(get_desktop_size(), exp_wmode);
    // "Native" means using game resolution as a window size
    if (option.CompareNoCase("native") == 0)
        return WindowSetup(game.GetGameRes(), exp_wmode);
    // Try parse an explicit resolution type or game scale factor --
    size_t at = option.FindChar('x');
    if (at == 0)
    { // try parse as a scale (xN)
        int scale = StrUtil::StringToInt(option.Mid(1));
        if (scale > 0) return WindowSetup(scale, exp_wmode);
    }
    else if (at != -1)
    { // else try parse as a "width x height"
        Size sz = Size(StrUtil::StringToInt(option.Mid(0, at)),
            StrUtil::StringToInt(option.Mid(at + 1)));
        if (!sz.IsNull()) return WindowSetup(sz, exp_wmode);
    }
    // In case of "default" option, or any format mistake, return the default
    return def_value;
}

// Legacy screen size definition
enum ScreenSizeDefinition
{
    kScreenDef_Undefined = -1,
    kScreenDef_Explicit,        // define by width & height
    kScreenDef_ByGameScaling,   // define by game scale factor
    kScreenDef_MaxDisplay,      // set to maximal supported (desktop/device screen size)
    kNumScreenDef
};

static ScreenSizeDefinition parse_legacy_screendef(const String &option)
{
    return StrUtil::ParseEnum<ScreenSizeDefinition>(option,
        CstrArr<kNumScreenDef>{"explicit", "scaling", "max"}, kScreenDef_Undefined);
}

FrameScaleDef parse_scaling_option(const String &option, FrameScaleDef def_value)
{
    // Backward compatible option name from the previous versions
    if (option.CompareNoCase("max_round") == 0) return kFrame_Round;
    return StrUtil::ParseEnum<FrameScaleDef>(option,
        CstrArr<kNumFrameScaleDef>{"round", "stretch", "proportional"}, def_value);
}

static FrameScaleDef parse_legacy_scaling_option(const String &option, int &scale)
{
    FrameScaleDef frame = parse_scaling_option(option, kFrame_Undefined);
    if (frame == kFrame_Undefined)
    {
        scale = StrUtil::StringToInt(option);
        return scale > 0 ? kFrame_Round : kFrame_Undefined;
    }
    return frame;
}

// Parses legacy filter ID and converts it into current scaling options
bool parse_legacy_frame_config(const String &scaling_option, String &filter_id,
                               FrameScaleDef &frame, int &scale_factor)
{
    struct
    {
        String LegacyName;
        String CurrentName;
        int    Scaling;
    } legacy_filters[6] = { {"none", "none", -1}, {"max", "StdScale", 0}, {"StdScale", "StdScale", -1},
                           {"AAx", "Linear", -1}, {"Hq2x", "Hqx", 2}, {"Hq3x", "Hqx", 3} };

    for (int i = 0; i < 6; i++)
    {
        if (scaling_option.CompareLeftNoCase(legacy_filters[i].LegacyName) == 0)
        {
            filter_id = legacy_filters[i].CurrentName;
            frame = kFrame_Round;
            scale_factor = legacy_filters[i].Scaling >= 0 ? legacy_filters[i].Scaling :
                scaling_option.Mid(legacy_filters[i].LegacyName.GetLength()).ToInt();
            return true;
        }
    }
    return false;
}

String make_window_mode_option(const WindowSetup &ws, const Size &game_res, const Size &desktop_res)
{
    if (ws.Mode == kWnd_FullDesktop)
        return "full_window";
    else if (ws.IsDefaultSize())
        return "default";
    else if (ws.Size.IsNull())
        return String::FromFormat("x%d", ws.Scale);
    else if (ws.Size == desktop_res)
        return "desktop";
    else if (ws.Size == game_res)
        return "native";
    return String::FromFormat("%dx%d", ws.Size.Width, ws.Size.Height);
}

String make_scaling_option(FrameScaleDef scale_def)
{
    switch (scale_def)
    {
    case kFrame_Stretch:
        return "stretch";
    case kFrame_Proportional:
        return "proportional";
    default:
        return "round";
    }
}

uint32_t convert_scaling_to_fp(int scale_factor)
{
    if (scale_factor >= 0)
        return scale_factor <<= kShift;
    else
        return kUnit / abs(scale_factor);
}

int convert_fp_to_scaling(uint32_t scaling)
{
    if (scaling == 0)
        return 0;
    return scaling >= kUnit ? (scaling >> kShift) : -kUnit / (int32_t)scaling;
}

String find_default_cfg_file()
{
    return Path::ConcatPaths(usetup.startup_dir, DefaultConfigFileName);
}

String find_user_global_cfg_file()
{
    return Path::ConcatPaths(GetGlobalUserConfigDir().FullDir, DefaultConfigFileName);
}

String find_user_cfg_file()
{
    return Path::ConcatPaths(GetGameUserConfigDir().FullDir, DefaultConfigFileName);
}

void config_defaults()
{
#if AGS_PLATFORM_OS_WINDOWS
    usetup.Screen.DriverID = "D3D9";
#else
    usetup.Screen.DriverID = "OGL";
#endif
    // Defaults for the window style are max resizing window and "fullscreen desktop"
    usetup.Screen.FsSetup = WindowSetup(kWnd_FullDesktop);
    usetup.Screen.WinSetup = WindowSetup(kWnd_Windowed);
    usetup.audio_backend = 1;
    usetup.translation = "";
}

static void read_legacy_graphics_config(const ConfigTree &cfg)
{
    // Pre-3.* game resolution setup
    int default_res = INIreadint(cfg, "misc", "defaultres", 0);
    int screen_res = INIreadint(cfg, "misc", "screenres", 0);
    if ((default_res == kGameResolution_320x200 ||
        default_res == kGameResolution_320x240) && screen_res > 0)
    {
        usetup.override_upscale = true; // run low-res game in high-res mode
    }

    usetup.Screen.Windowed = INIreadint(cfg, "misc", "windowed") > 0;
    usetup.Screen.DriverID = INIreadstring(cfg, "misc", "gfxdriver", usetup.Screen.DriverID);

    // Window setup: style and size definition, game frame style
    {
        String legacy_filter = INIreadstring(cfg, "misc", "gfxfilter");
        if (!legacy_filter.IsEmpty())
        {
            // Legacy scaling config is applied only to windowed setting
            int scale_factor = 0;
            parse_legacy_frame_config(legacy_filter, usetup.Screen.Filter.ID, usetup.Screen.WinGameFrame,
                scale_factor);
            if (scale_factor > 0)
                usetup.Screen.WinSetup = WindowSetup(scale_factor);

            // AGS 3.2.1 and 3.3.0 aspect ratio preferences for fullscreen
            if (!usetup.Screen.Windowed)
            {
                bool allow_borders = 
                    (INIreadint(cfg, "misc", "sideborders") > 0 || INIreadint(cfg, "misc", "forceletterbox") > 0 ||
                     INIreadint(cfg, "misc", "prefer_sideborders") > 0 || INIreadint(cfg, "misc", "prefer_letterbox") > 0);
                usetup.Screen.FsGameFrame = allow_borders ? kFrame_Proportional : kFrame_Stretch;
            }
        }

        // AGS 3.4.0 - 3.4.1-rc uniform scaling option
        String uniform_frame_scale = INIreadstring(cfg, "graphics", "game_scale");
        if (!uniform_frame_scale.IsEmpty())
        {
            int src_scale = 1;
            FrameScaleDef frame = parse_legacy_scaling_option(uniform_frame_scale, src_scale);
            usetup.Screen.FsGameFrame = frame;
            usetup.Screen.WinGameFrame = frame;
        }

        // AGS 3.5.* gfx mode with screen definition
        const bool is_windowed = INIreadint(cfg, "graphics", "windowed") != 0;
        WindowSetup &ws = is_windowed ? usetup.Screen.WinSetup : usetup.Screen.FsSetup;
        const WindowMode wm = is_windowed ? kWnd_Windowed : kWnd_Fullscreen;
        ScreenSizeDefinition scr_def = parse_legacy_screendef(INIreadstring(cfg, "graphics", "screen_def"));
        switch (scr_def)
        {
        case kScreenDef_Explicit:
            {
                Size sz(
                    INIreadint(cfg, "graphics", "screen_width"),
                    INIreadint(cfg, "graphics", "screen_height"));
                ws = WindowSetup(sz, wm);
            }
            break;
        case kScreenDef_ByGameScaling:
            {
                int src_scale;
                is_windowed ?
                    parse_legacy_scaling_option(INIreadstring(cfg, "graphics", "game_scale_win"), src_scale) :
                    parse_legacy_scaling_option(INIreadstring(cfg, "graphics", "game_scale_fs"), src_scale);
                ws = WindowSetup(src_scale, wm);
            }
            break;
        case kScreenDef_MaxDisplay:
            ws = is_windowed ? WindowSetup() : WindowSetup(kWnd_FullDesktop);
            break;
        default:
            break;
        }
    }

    usetup.Screen.Params.RefreshRate = INIreadint(cfg, "misc", "refresh");
}

// Variables used for mobile port configs
extern int psp_gfx_renderer;
extern int psp_gfx_scaling;
extern int psp_gfx_super_sampling;
extern int psp_gfx_smoothing;
extern int psp_gfx_smooth_sprites;
extern char psp_translation[];
#if AGS_PLATFORM_OS_ANDROID
extern int config_mouse_control_mode;
#endif

void override_config_ext(ConfigTree &cfg)
{
    // Mobile ports always run in fullscreen mode
#if AGS_PLATFORM_OS_ANDROID || AGS_PLATFORM_OS_IOS
    INIwriteint(cfg, "graphics", "windowed", 0);
#endif

    // psp_gfx_renderer - rendering mode
    //    * 0 - software renderer
    //    * 1 - hardware, render to screen
    //    * 2 - hardware, render to texture
    if (psp_gfx_renderer == 0)
    {
        INIwritestring(cfg, "graphics", "driver", "Software");
        INIwriteint(cfg, "graphics", "render_at_screenres", 1);
    }
    else
    {
        INIwritestring(cfg, "graphics", "driver", "OGL");
        INIwriteint(cfg, "graphics", "render_at_screenres", psp_gfx_renderer == 1);
    }

    // psp_gfx_scaling - scaling style:
    //    * 0 - no scaling
    //    * 1 - stretch and preserve aspect ratio
    //    * 2 - stretch to whole screen
    if (psp_gfx_scaling == 0)
        INIwritestring(cfg, "graphics", "game_scale_fs", "1");
    else if (psp_gfx_scaling == 1)
        INIwritestring(cfg, "graphics", "game_scale_fs", "proportional");
    else
        INIwritestring(cfg, "graphics", "game_scale_fs", "stretch");

    // psp_gfx_smoothing - scaling filter:
    //    * 0 - nearest-neighbour
    //    * 1 - linear
    if (psp_gfx_smoothing == 0)
        INIwritestring(cfg, "graphics", "filter", "StdScale");
    else
        INIwritestring(cfg, "graphics", "filter", "Linear");

    // psp_gfx_super_sampling - enable super sampling
    //    * 0 - x1
    //    * 1 - x2
    if (psp_gfx_renderer == 2)
        INIwriteint(cfg, "graphics", "supersampling", psp_gfx_super_sampling + 1);
    else
        INIwriteint(cfg, "graphics", "supersampling", 0);

#if AGS_PLATFORM_OS_ANDROID
    // config_mouse_control_mode - enable relative mouse mode
    //    * 1 - relative mouse touch controls
    //    * 0 - direct touch mouse control
    INIwriteint(cfg, "mouse", "control_enabled", config_mouse_control_mode);
#endif

    INIwriteint(cfg, "misc", "antialias", psp_gfx_smooth_sprites != 0);
    INIwritestring(cfg, "language", "translation", psp_translation);
}

void apply_config(const ConfigTree &cfg)
{
    {
        usetup.audio_backend = INIreadint(cfg, "sound", "enabled", usetup.audio_backend);

        // Legacy graphics settings has to be translated into new options;
        // they must be read first, to let newer options override them, if ones are present
        read_legacy_graphics_config(cfg);

        // Graphics mode
        usetup.Screen.DriverID = INIreadstring(cfg, "graphics", "driver", usetup.Screen.DriverID);
        usetup.Screen.Windowed = INIreadint(cfg, "graphics", "windowed", usetup.Screen.Windowed ? 1 : 0) > 0;
        usetup.Screen.FsSetup =
            parse_window_mode(INIreadstring(cfg, "graphics", "fullscreen", "default"), false, usetup.Screen.FsSetup);
        usetup.Screen.WinSetup =
            parse_window_mode(INIreadstring(cfg, "graphics", "window", "default"), true, usetup.Screen.WinSetup);

        // TODO: move to config overrides (replace values during config load)
#if AGS_PLATFORM_OS_MACOS
        usetup.Screen.Filter.ID = "none";
#else
        usetup.Screen.Filter.ID = INIreadstring(cfg, "graphics", "filter", "StdScale");
        usetup.Screen.FsGameFrame =
            parse_scaling_option(INIreadstring(cfg, "graphics", "game_scale_fs", "proportional"), usetup.Screen.FsGameFrame);
        usetup.Screen.WinGameFrame =
            parse_scaling_option(INIreadstring(cfg, "graphics", "game_scale_win", "round"), usetup.Screen.WinGameFrame);
#endif

        usetup.Screen.Params.RefreshRate = INIreadint(cfg, "graphics", "refresh");
        usetup.Screen.Params.VSync = INIreadint(cfg, "graphics", "vsync") > 0;
        usetup.RenderAtScreenRes = INIreadint(cfg, "graphics", "render_at_screenres") > 0;
        usetup.Supersampling = INIreadint(cfg, "graphics", "supersampling", 1);

        usetup.enable_antialiasing = INIreadint(cfg, "misc", "antialias") > 0;

        // This option is backwards (usevox is 0 if no_speech_pack)
        usetup.no_speech_pack = INIreadint(cfg, "sound", "usespeech", 1) == 0;

        usetup.user_data_dir = INIreadstring(cfg, "misc", "user_data_dir");
        usetup.shared_data_dir = INIreadstring(cfg, "misc", "shared_data_dir");

        usetup.translation = INIreadstring(cfg, "language", "translation");

        int cache_size_kb = INIreadint(cfg, "misc", "cachemax", DEFAULTCACHESIZE_KB);
        if (cache_size_kb > 0)
            spriteset.SetMaxCacheSize((size_t)cache_size_kb * 1024);

        usetup.mouse_auto_lock = INIreadint(cfg, "mouse", "auto_lock") > 0;

        usetup.mouse_speed = INIreadfloat(cfg, "mouse", "speed", 1.f);
        if (usetup.mouse_speed <= 0.f)
            usetup.mouse_speed = 1.f;
        String mouse_str = INIreadstring(cfg, "mouse", "control_when", "fullscreen");
        usetup.mouse_ctrl_when = StrUtil::ParseEnum<MouseControlWhen>(
            mouse_str, CstrArr<kNumMouseCtrlOptions>{ "never", "fullscreen", "always" },
                usetup.mouse_ctrl_when);
        usetup.mouse_ctrl_enabled = INIreadint(cfg, "mouse", "control_enabled", usetup.mouse_ctrl_enabled) > 0;
        mouse_str = INIreadstring(cfg, "mouse", "speed_def", "current_display");
        usetup.mouse_speed_def = StrUtil::ParseEnum<MouseSpeedDef>(
            mouse_str, CstrArr<kNumMouseSpeedDefs>{ "absolute", "current_display" }, usetup.mouse_speed_def);

        usetup.override_multitasking = INIreadint(cfg, "override", "multitasking", -1);
        String override_os = INIreadstring(cfg, "override", "os");
        usetup.override_script_os = StrUtil::ParseEnum<eScriptSystemOSID>(
            override_os, CstrArr<5>{"", "dos", "win", "linux", "mac"}, (eScriptSystemOSID)-1);
        usetup.override_upscale = INIreadint(cfg, "override", "upscale", usetup.override_upscale) > 0;
    }

    // Apply logging configuration
    apply_debug_config(cfg);
}

void post_config()
{
    if (usetup.Screen.DriverID.IsEmpty() || usetup.Screen.DriverID.CompareNoCase("DX5") == 0)
        usetup.Screen.DriverID = "Software";

    // FIXME: this correction is needed at the moment because graphics driver
    // implementation requires some filter to be created anyway
    usetup.Screen.Filter.UserRequest = usetup.Screen.Filter.ID;
    if (usetup.Screen.Filter.ID.IsEmpty() || usetup.Screen.Filter.ID.CompareNoCase("none") == 0)
    {
        usetup.Screen.Filter.ID = "StdScale";
    }
}

void save_config_file()
{
    ConfigTree cfg;
    // Last display mode
    cfg["graphics"]["windowed"] = String::FromFormat("%d", System_GetWindowed() != 0 ? 1 : 0);
    // Other game options that could be changed at runtime
    if (game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_UserDefined)
        cfg["graphics"]["render_at_screenres"] = String::FromFormat("%d", usetup.RenderAtScreenRes ? 1 : 0);
    cfg["mouse"]["control_enabled"] = String::FromFormat("%d", usetup.mouse_ctrl_enabled ? 1 : 0);
    cfg["mouse"]["speed"] = String::FromFormat("%f", Mouse::GetSpeed());
    cfg["language"]["translation"] = usetup.translation;

    String cfg_file = PreparePathForWriting(GetGameUserConfigDir(), DefaultConfigFileName);
    if (!cfg_file.IsEmpty())
        IniUtil::Merge(cfg_file, cfg);
}
