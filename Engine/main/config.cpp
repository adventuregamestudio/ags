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
//
// Game configuration
//
#include <ctype.h> // toupper
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
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
extern SpriteCache spriteset;

// Filename of the default config file, the one found in the game installation
const String DefaultConfigFileName = "acsetup.cfg";


WindowSetup parse_window_mode(const String &option, bool as_windowed, const WindowSetup &def_value)
{
    // "full_window" option means pseudo fullscreen ("borderless fullscreen window")
    if (!as_windowed && (option.CompareNoCase("full_window") == 0))
        return WindowSetup(kWnd_FullDesktop);
    // Check supported options for explicit resolution or scale factor,
    // in which case we'll use either a resizing window or a REAL fullscreen mode
    const WindowMode exp_wmode = as_windowed ? kWnd_Windowed : kWnd_Fullscreen;
    // Note that for "desktop" we return "default" for windowed, this will result
    // in refering to the desktop size but resizing in accordance to the scaling style
    if (option.CompareNoCase("desktop") == 0)
        return as_windowed ? WindowSetup(exp_wmode) : WindowSetup(kWndSizeHint_Desktop, Size(), exp_wmode);
    // "Native" means using game resolution as a window size
    if (option.CompareNoCase("native") == 0)
        return WindowSetup(kWndSizeHint_GameNative, Size(), exp_wmode);
    // Try parse an explicit resolution type or game scale factor --
    size_t at = option.FindChar('x');
    if (at == 0)
    { // try parse as a scale (xN)
        int scale = StrUtil::StringToInt(option.Mid(1));
        if (scale > 0)
            return WindowSetup(scale, exp_wmode);
    }
    else if (at != String::NoIndex)
    { // else try parse as a "width x height"
        Size sz = Size(StrUtil::StringToInt(option.Mid(0, at)),
            StrUtil::StringToInt(option.Mid(at + 1)));
        if (!sz.IsNull())
            return WindowSetup(sz, exp_wmode);
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

SkipSpeechStyle parse_speechskip_style(const String &option, SkipSpeechStyle def_value)
{
    const std::array<std::pair<const char*, SkipSpeechStyle>, 4> skip_speech_arr{
        { { "default", kSkipSpeechNone }, { "input", kSkipSpeech_AnyInput }, { "any", kSkipSpeech_AnyInputOrTime }, { "time", kSkipSpeechTime } } };
    return StrUtil::ParseEnumOptions<SkipSpeechStyle>(option, skip_speech_arr, def_value);
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

void parse_asset_dirs(const String &option, std::vector<std::pair<String, String>> &opt_dirs)
{
    const auto asset_dirs = option.Split(',');
    for (const auto &adir : asset_dirs)
    {
        String filters;
        size_t sep_at = adir.FindCharReverse(':'); // cut from right, as the dir path may contain ':' separator
        if (sep_at != String::NoIndex)
            filters = adir.Mid(sep_at + 1);
        String dir_path = adir.Left(sep_at);
        dir_path = StrUtil::Undoublequote(dir_path);
        filters = StrUtil::Undoublequote(filters);
        opt_dirs.push_back(std::make_pair(dir_path, filters));
    }
}

String make_window_mode_option(const WindowSetup &ws)
{
    if (ws.Mode == kWnd_FullDesktop)
        return "full_window";
    else if (ws.SizeHint == kWndSizeHint_Desktop)
        return "desktop";
    else if (ws.SizeHint == kWndSizeHint_GameNative)
        return "native";
    else if (ws.IsDefaultSize())
        return "default";
    else if (ws.Size.IsNull())
        return String::FromFormat("x%d", ws.Scale);
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

String make_speechskip_option(SkipSpeechStyle style)
{
    switch (style)
    {
    case kSkipSpeech_AnyInput: return "input";
    case kSkipSpeech_AnyInputOrTime: return "any";
    case kSkipSpeechTime: return "time";
    default: return "default";
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
    return Path::ConcatPaths(usetup.StartupDir, DefaultConfigFileName);
}

String find_user_global_cfg_file()
{
    return Path::ConcatPaths(GetGlobalUserConfigDir().FullDir, DefaultConfigFileName);
}

String find_user_cfg_file()
{
    return Path::ConcatPaths(GetGameUserConfigDir().FullDir, DefaultConfigFileName);
}

void config_defaults(GameSetup &setup)
{
#if AGS_PLATFORM_OS_WINDOWS
    setup.Display.DriverID = "D3D9";
#else
    setup.Display.DriverID = "OGL";
#endif
    // Defaults for the window style are max resizing window and "fullscreen desktop"
    setup.Display.FsSetup = WindowSetup(kWnd_FullDesktop);
    setup.Display.WinSetup = WindowSetup(kWnd_Windowed);
    setup.Display.FsGameFrame = kFrame_Proportional;
    setup.Display.WinGameFrame = kFrame_Round;

    setup.AudioEnabled = true;
    setup.UseVoicePack = true;

    setup.MouseEnabled = true;
    setup.MouseCtrlWhen = kMouseCtrl_Fullscreen;
    setup.MouseCtrlEnabled = true;
    setup.MouseSpeedDef = kMouseSpeed_CurrentDisplay;

    setup.TouchEmulateMouse = kTouchMouse_OneFingerDrag;
    setup.TouchMotionRelative = false;
}

static void read_legacy_graphics_config(const ConfigTree &cfg, GameConfig &setup)
{
    setup.Display.Windowed = CfgReadBoolInt(cfg, "misc", "windowed");
    setup.Display.DriverID = CfgReadString(cfg, "misc", "gfxdriver", setup.Display.DriverID);

    // Window setup: style and size definition, game frame style
    {
        String legacy_filter = CfgReadString(cfg, "misc", "gfxfilter");
        if (!legacy_filter.IsEmpty())
        {
            // Legacy scaling config is applied only to windowed setting
            int scale_factor = 0;
            parse_legacy_frame_config(legacy_filter, setup.Display.Filter.ID, setup.Display.WinGameFrame,
                scale_factor);
            if (scale_factor > 0)
                setup.Display.WinSetup = WindowSetup(scale_factor);

            // AGS 3.2.1 and 3.3.0 aspect ratio preferences for fullscreen
            if (!setup.Display.Windowed)
            {
                bool allow_borders = 
                    (CfgReadBoolInt(cfg, "misc", "sideborders") || CfgReadBoolInt(cfg, "misc", "forceletterbox") ||
                     CfgReadBoolInt(cfg, "misc", "prefer_sideborders") || CfgReadBoolInt(cfg, "misc", "prefer_letterbox"));
                setup.Display.FsGameFrame = allow_borders ? kFrame_Proportional : kFrame_Stretch;
            }
        }

        // AGS 3.4.0 - 3.4.1-rc uniform scaling option
        String uniform_frame_scale = CfgReadString(cfg, "graphics", "game_scale");
        if (!uniform_frame_scale.IsEmpty())
        {
            int src_scale = 1;
            FrameScaleDef frame = parse_legacy_scaling_option(uniform_frame_scale, src_scale);
            setup.Display.FsGameFrame = frame;
            setup.Display.WinGameFrame = frame;
        }

        // AGS 3.5.* gfx mode with screen definition
        const bool is_windowed = CfgReadBoolInt(cfg, "graphics", "windowed");
        WindowSetup &ws = is_windowed ? setup.Display.WinSetup : setup.Display.FsSetup;
        const WindowMode wm = is_windowed ? kWnd_Windowed : kWnd_Fullscreen;
        ScreenSizeDefinition scr_def = parse_legacy_screendef(CfgReadString(cfg, "graphics", "screen_def"));
        switch (scr_def)
        {
        case kScreenDef_Explicit:
            {
                Size sz(
                    CfgReadInt(cfg, "graphics", "screen_width"),
                    CfgReadInt(cfg, "graphics", "screen_height"));
                ws = WindowSetup(sz, wm);
            }
            break;
        case kScreenDef_ByGameScaling:
            {
                int src_scale = 0;
                is_windowed ?
                    parse_legacy_scaling_option(CfgReadString(cfg, "graphics", "game_scale_win"), src_scale) :
                    parse_legacy_scaling_option(CfgReadString(cfg, "graphics", "game_scale_fs"), src_scale);
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

    setup.Display.RefreshRate = CfgReadInt(cfg, "misc", "refresh");
    setup.AntialiasSprites = CfgReadBoolInt(cfg, "misc", "antialias");
}

static void read_legacy_config(const ConfigTree &cfg, GameConfig &setup)
{
    read_legacy_graphics_config(cfg, setup);

    setup.SpriteCacheSize = CfgReadInt(cfg , "misc", "cachemax", setup.SpriteCacheSize);
}

static void read_legacy_meta_config(const ConfigTree &cfg, GameSetup &setup)
{
    // Pre-3.* game resolution setup
    int default_res = CfgReadInt(cfg, "misc", "defaultres", kGameResolution_Default);
    int screen_res = CfgReadInt(cfg, "misc", "screenres", 0);
    if (screen_res > 0 &&
       (default_res >= kGameResolution_Default && default_res <= kGameResolution_320x240))
    {
        setup.Override.UpscaleResolution = true; // run low-res game in high-res mode
    }
}

void override_config_ext(ConfigTree &cfg)
{
    platform->ReadConfiguration(cfg);
}

void load_common_config(const ConfigTree &cfg, GameConfig &setup)
{
    // Legacy settings have to be translated into new options;
    // they must be read first, to let newer options override them, if ones are present
    read_legacy_config(cfg, setup);

    // Graphics mode and options
    setup.Display.DriverID = CfgReadString(cfg, "graphics", "driver", setup.Display.DriverID);
    const int user_display_index = CfgReadInt(cfg, "graphics", "display", 0);
    setup.Display.UseDefaultDisplay = user_display_index <= 0;
    setup.Display.DisplayIndex = user_display_index <= 0 ? 0 : user_display_index - 1;
    setup.Display.Windowed = CfgReadBoolInt(cfg, "graphics", "windowed", setup.Display.Windowed);
    setup.Display.FsSetup =
        parse_window_mode(CfgReadString(cfg, "graphics", "fullscreen", "default"), false,
            setup.Display.FsSetup);
    setup.Display.WinSetup =
        parse_window_mode(CfgReadString(cfg, "graphics", "window", "default"), true,
            setup.Display.WinSetup);

    setup.Display.Filter.ID = CfgReadString(cfg, "graphics", "filter", "StdScale");
    setup.Display.FsGameFrame =
        parse_scaling_option(CfgReadString(cfg, "graphics", "game_scale_fs", "proportional"), setup.Display.FsGameFrame);
    setup.Display.WinGameFrame =
        parse_scaling_option(CfgReadString(cfg, "graphics", "game_scale_win", "round"), setup.Display.WinGameFrame);

    setup.Display.RefreshRate = CfgReadInt(cfg, "graphics", "refresh");
    setup.Display.VSync = CfgReadBoolInt(cfg, "graphics", "vsync");
    setup.RenderAtScreenRes = CfgReadBoolInt(cfg, "graphics", "render_at_screenres");
    setup.AntialiasSprites = CfgReadBoolInt(cfg, "graphics", "antialias", setup.AntialiasSprites);
    setup.SoftwareRenderDriver = CfgReadString(cfg, "graphics", "software_driver");

    String rotation_str = CfgReadString(cfg, "graphics", "rotation", "unlocked");
    setup.Rotation = StrUtil::ParseEnum<ScreenRotation>(
            rotation_str, CstrArr<kNumScreenRotationOptions>{ "unlocked", "portrait", "landscape" },
            setup.Rotation);

    // Audio options
    setup.AudioEnabled = CfgReadBoolInt(cfg, "sound", "enabled", setup.AudioEnabled);
    setup.AudioDriverID = CfgReadString(cfg, "sound", "driver");
    setup.UseVoicePack = CfgReadBoolInt(cfg, "sound", "usespeech", true);

    // Mouse options
    setup.MouseEnabled = CfgReadBoolInt(cfg, "mouse", "enabled", setup.MouseEnabled);
    setup.MouseAutoLock = CfgReadBoolInt(cfg, "mouse", "auto_lock", setup.MouseAutoLock);
    setup.MouseSpeed = CfgReadFloat(cfg, "mouse", "speed", setup.MouseSpeed);
    if (setup.MouseSpeed <= 0.f)
        setup.MouseSpeed = 1.f;

    // Touch options
    setup.TouchEmulateMouse = StrUtil::ParseEnum<TouchMouseEmulation>(
        CfgReadString(cfg, "touch", "emul_mouse_mode", "one_finger"),
        CstrArr<kNumTouchMouseModes>{ "off", "one_finger", "two_fingers" }, setup.TouchEmulateMouse);
    setup.TouchMotionRelative = CfgReadBoolInt(cfg, "touch", "emul_mouse_relative");

    // Translation / localization
    setup.Translation = CfgReadString(cfg, "language", "translation");

    // Custom paths
    setup.UserSaveDir = CfgReadString(cfg, "misc", "user_data_dir");
    setup.AppDataDir = CfgReadString(cfg, "misc", "shared_data_dir");

    // Resource caches and options
    // Clamp the values to the max supported for this platform
    setup.SpriteCacheSize = std::min<uint64_t>(
        CfgReadUInt64(cfg, "graphics", "sprite_cache_size", setup.SpriteCacheSize),
        SIZE_MAX / 1024);
    setup.TextureCacheSize = std::min<uint64_t>(
        CfgReadUInt64(cfg, "graphics", "texture_cache_size", setup.TextureCacheSize),
        SIZE_MAX / 1024);
    setup.SoundCacheSize = std::min<uint64_t>(
        CfgReadUInt64(cfg, "sound", "cache_size", setup.SoundCacheSize),
        SIZE_MAX / 1024);
    setup.SoundLoadAtOnceSize = std::min<uint64_t>(
        CfgReadUInt64(cfg, "sound", "stream_threshold", setup.SoundLoadAtOnceSize),
        SIZE_MAX / 1024);

    // Various system options
    setup.LoadLatestSave = CfgReadBoolInt(cfg, "misc", "load_latest_save", setup.LoadLatestSave);
    setup.CompressSaves = CfgReadBoolInt(cfg, "misc", "compress_saves", setup.CompressSaves);
    setup.RunInBackground = CfgReadInt(cfg, "misc", "background", 0) != 0;
    setup.ShowFps = CfgReadBoolInt(cfg, "misc", "show_fps");
    setup.ClearCacheOnRoomChange = CfgReadBoolInt(cfg, "misc", "clear_cache_on_room_change", setup.ClearCacheOnRoomChange);

    // Accessibility settings
    setup.Access.SpeechSkipStyle = parse_speechskip_style(CfgReadString(cfg, "access", "speechskip"));
    setup.Access.TextSkipStyle = parse_speechskip_style(CfgReadString(cfg, "access", "textskip"));
    setup.Access.TextReadSpeed = CfgReadInt(cfg, "access", "textreadspeed");
}

void apply_config(const ConfigTree &cfg, GameSetup &setup)
{
    load_common_config(cfg, setup);

    //
    // Additional config applied by the engine
    //

    read_legacy_meta_config(cfg, setup);

    // Mouse control options
    String mouse_str = CfgReadString(cfg, "mouse", "control_when", "fullscreen");
    setup.MouseCtrlWhen = StrUtil::ParseEnum<MouseControlWhen>(
        mouse_str, CstrArr<kNumMouseCtrlOptions>{ "never", "fullscreen", "always" },
            setup.MouseCtrlWhen);
    setup.MouseCtrlEnabled = CfgReadBoolInt(cfg, "mouse", "control_enabled", setup.MouseCtrlEnabled);
    mouse_str = CfgReadString(cfg, "mouse", "speed_def", "current_display");
    setup.MouseSpeedDef = StrUtil::ParseEnum<MouseSpeedDefinition>(
        mouse_str, CstrArr<kNumMouseSpeedDefs>{ "absolute", "current_display" }, setup.MouseSpeedDef);

    // User's overrides and hacks
    setup.Override.Multitasking = CfgReadInt(cfg, "override", "multitasking", -1);
    setup.Override.NoPlugins = CfgReadBoolInt(cfg, "override", "noplugins", false);
    String override_os = CfgReadString(cfg, "override", "os");
    setup.Override.ScriptOS = StrUtil::ParseEnum<eScriptSystemOSID>(override_os,
        CstrArr<eNumOS>{"", "dos", "win", "linux", "mac", "android", "ios", "psp", "web", "freebsd"}, eOS_Unknown);
    setup.Override.UpscaleResolution = CfgReadBoolInt(cfg, "override", "upscale", setup.Override.UpscaleResolution);
    setup.Override.KeySaveGame = CfgReadInt(cfg, "override", "save_game_key", 0);
    setup.Override.KeyRestoreGame = CfgReadInt(cfg, "override", "restore_game_key", 0);
    setup.Override.MaxSaveSlot = CfgReadInt(cfg, "override", "max_save", 0);

    // Apply logging configuration
    apply_debug_config(cfg, true);
}

void post_config(GameSetup &setup)
{
    if (setup.Display.DriverID.IsEmpty() || setup.Display.DriverID.CompareNoCase("DX5") == 0)
    {
        setup.Display.DriverID = "Software";
    }

    if (setup.Display.Filter.ID.IsEmpty() || setup.Display.Filter.ID.CompareNoCase("none") == 0)
    {
        setup.Display.Filter.ID = "StdScale";
    }
}

void save_common_config(const GameConfig &setup, ConfigTree &cfg)
{
    CfgWriteString(cfg, "misc", "user_data_dir", setup.UserSaveDir);
    CfgWriteString(cfg, "misc", "shared_data_dir", setup.AppDataDir);
    CfgWriteBoolInt(cfg, "misc", "compress_saves", setup.CompressSaves);

    CfgWriteString(cfg, "graphics", "driver", setup.Display.DriverID);
    CfgWriteInt(cfg, "graphics", "display", (setup.Display.UseDefaultDisplay) ?
        0 : setup.Display.DisplayIndex + 1);
    CfgWriteString(cfg, "graphics", "filter", setup.Display.Filter.ID);
    CfgWriteString(cfg, "graphics", "fullscreen", make_window_mode_option(setup.Display.FsSetup));
    CfgWriteString(cfg, "graphics", "window", make_window_mode_option(setup.Display.WinSetup));
    CfgWriteString(cfg, "graphics", "game_scale_fs", make_scaling_option(setup.Display.FsGameFrame));
    CfgWriteString(cfg, "graphics", "game_scale_win", make_scaling_option(setup.Display.WinGameFrame));
    CfgWriteInt(cfg, "graphics", "windowed", setup.Display.Windowed ? 1 : 0);
    CfgWriteInt(cfg, "graphics", "refresh", setup.Display.RefreshRate);
    CfgWriteInt(cfg, "graphics", "vsync", setup.Display.VSync ? 1 : 0);
    CfgWriteInt(cfg, "graphics", "render_at_screenres", setup.RenderAtScreenRes ? 1 : 0);
    CfgWriteInt(cfg, "graphics", "antialias", setup.AntialiasSprites ? 1 : 0);

    CfgWriteInt(cfg, "sound", "enabled", setup.AudioEnabled ? 1 : 0);
    CfgWriteString(cfg, "sound", "driver", setup.AudioDriverID);
    CfgWriteInt(cfg, "sound", "usespeech", setup.UseVoicePack ? 1 : 0);

    CfgWriteInt(cfg, "mouse", "auto_lock", setup.MouseAutoLock ? 1 : 0);
    CfgWriteFloat(cfg, "mouse", "speed", setup.MouseSpeed, 1);

    CfgWriteUInt(cfg, "graphics", "sprite_cache_size", setup.SpriteCacheSize);
    CfgWriteUInt(cfg, "graphics", "texture_cache_size", setup.TextureCacheSize);
    CfgWriteUInt(cfg, "sound", "cache_size", setup.SoundCacheSize);
    CfgWriteString(cfg, "language", "translation", setup.Translation);

    CfgWriteString(cfg, "access", "speechskip", make_speechskip_option(setup.Access.SpeechSkipStyle));
    CfgWriteString(cfg, "access", "textskip", make_speechskip_option(setup.Access.TextSkipStyle));
    CfgWriteInt(cfg, "access", "textreadspeed", setup.Access.TextReadSpeed);
}

void save_runtime_config_file()
{
    ConfigTree cfg;
    // Last display mode
    cfg["graphics"]["windowed"] = String::FromFormat("%d", System_GetWindowed() != 0 ? 1 : 0);
    // Other game options that could be changed at runtime
    if (game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_UserDefined)
        cfg["graphics"]["render_at_screenres"] = String::FromFormat("%d", usetup.RenderAtScreenRes ? 1 : 0);
    cfg["mouse"]["control_enabled"] = String::FromFormat("%d", usetup.MouseCtrlEnabled ? 1 : 0);
    cfg["mouse"]["speed"] = String::FromFormat("%f", Mouse::GetSpeed());
    cfg["language"]["translation"] = usetup.Translation;

    String cfg_file = PreparePathForWriting(GetGameUserConfigDir(), DefaultConfigFileName);
    if (!cfg_file.IsEmpty())
        IniUtil::Merge(cfg_file, cfg);
}
