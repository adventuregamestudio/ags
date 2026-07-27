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
#include "data/game_utils.h"

using namespace AGS::Common;

namespace AGS
{
namespace DataUtil
{

// TODO: consider a generic String-to-String conversion based on a array of string pairs?

String MakeGameScalingConfig(const String &scaling)
{
    if (scaling == "MaxInteger" || scaling == "Integer")
        return "round";
    if (scaling == "StretchToFit")
        return "stretch";
    if (scaling == "ProportionalStretch")
        return "proportional";
    return "";
}

String MakeRotationConfig(const String &rotation)
{
    if (rotation == "Unlocked")
        return "unlocked";
    else if (rotation == "Portrait")
        return "portrait";
    else if (rotation == "Landscape")
        return "landscape";
    return "";
}

String MakeTouchMouseEmulModeConfig(const String &mode)
{
    if (mode == "OneFinger")
        return "one_finger";
    if (mode == "TwoFingers")
        return "two_fingers";
    return "off";
}

String CustomPathForConfig(bool use_custom_path, const String &custom_path)
{
    String path_value = ""; // no value
    if (use_custom_path)
    {
        if (custom_path.IsEmpty())
            path_value = "."; // same directory
        else
            path_value = custom_path;
    }
    return path_value;
}

void WriteConfig(const RuntimeSetup &setup, const GameSettings *settings, ConfigTree &cfg)
{
    cfg["graphics"]["driver"] = setup.GraphicsDriver;
    cfg["graphics"]["windowed"] = setup.Windowed ? "1" : "0";
    cfg["graphics"]["fullscreen"] = setup.FullscreenDesktop ? "full_window" : "desktop";
    if (setup.WindowGameScaling == "Integer")
        cfg["graphics"]["window"] = String::FromFormat("x%d", setup.GameScalingMultiplier);
    else
        cfg["graphics"]["window"] = "desktop";

    cfg["graphics"]["game_scale_fs"] = MakeGameScalingConfig(setup.FullscreenGameScaling);
    cfg["graphics"]["game_scale_win"] = MakeGameScalingConfig(setup.WindowGameScaling);

    cfg["graphics"]["filter"] = setup.GraphicsFilter;
    cfg["graphics"]["vsync"] = setup.VSync ? "1" : "0";
    cfg["graphics"]["antialias"] = setup.AAScaledSprites ? "1" : "0";
    bool render_at_screenres = setup.RenderAtScreenResolution;
    if (settings)
        render_at_screenres = render_at_screenres
            && (settings->RenderAtScreenResolution != ::kRenderAtScreenRes_Disabled)
            || (settings->RenderAtScreenResolution == ::kRenderAtScreenRes_Enabled);
    cfg["graphics"]["render_at_screenres"] = render_at_screenres ? "1" : "0";
    
    cfg["graphics"]["rotation"] = MakeRotationConfig(setup.Rotation);

    bool audio_enabled = setup.AudioDriver != "Disabled";
    cfg["sound"]["enabled"] = audio_enabled ? "1" : "0";
    cfg["sound"]["driver"] = ""; // always default
    cfg["sound"]["usespeech"] = setup.UseVoicePack ? "1" : "0";

    cfg["language"]["translation"] = setup.Translation;
    cfg["mouse"]["auto_lock"] = setup.AutoLockMouse ? "1" : "0";
    cfg["mouse"]["speed"] = String::FromFormat("%.f", setup.MouseSpeed);

    // Touch input
    cfg["touch"]["emul_mouse_mode"] = MakeTouchMouseEmulModeConfig(setup.TouchToMouseEmulation);
    cfg["touch"]["emul_mouse_relative"] = setup.TouchToMouseMotionMode == "Relative" ? "1" : "0";

    // Note: the cache sizes are written in KB (while we have it in MB on the editor pane)
    cfg["graphics"]["sprite_cache_size"] = String::FromFormat("%d", setup.SpriteCacheSize * 1024);
    cfg["graphics"]["texture_cache_size"] = String::FromFormat("%d", setup.TextureCacheSize * 1024);
    cfg["sound"]["cache_size"] = String::FromFormat("%d", setup.SoundCacheSize * 1024);
    cfg["misc"]["compress_saves"] = setup.CompressSaves ? "1" : "0";

    cfg["misc"]["user_data_dir"] = CustomPathForConfig(setup.UseCustomSavePath, setup.CustomSavePath);
    cfg["misc"]["shared_data_dir"] = CustomPathForConfig(setup.UseCustomAppDataPath, setup.CustomAppDataPath);
    cfg["misc"]["titletext"] = setup.TitleText;

    // Do not write show_fps in a release build, this is only intended for the developer
    if (!settings || !settings->DebugMode)
    {
        cfg["misc"]["show_fps"] = setup.ShowFPS ? "1" : "0";
    }
}

} // namespace DataUtil
} // namespace AGS
