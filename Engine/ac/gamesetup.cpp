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
#include "ac/gamesetup.h"

GameSetup::GameSetup()
{
    audio_enabled = true;
    no_speech_pack = false;
    textheight = 0;
    enable_antialiasing = false;
    disable_exception_handling = false;
    mouse_auto_lock = false;
    override_script_os = -1;
    override_multitasking = -1;
    override_upscale = false;
    mouse_speed = 1.f;
    mouse_ctrl_when = kMouseCtrl_Fullscreen;
    mouse_ctrl_enabled = true;
    mouse_speed_def = kMouseSpeed_CurrentDisplay;
    touch_emulate_mouse = kTouchMouse_OneFingerDrag;
    touch_motion_relative = false;
    RenderAtScreenRes = false;
    Supersampling = 1;
    clear_cache_on_room_change = false;
    load_latest_save = false;
    rotation = kScreenRotation_Unlocked;
    show_fps = false;

    Screen.Params.RefreshRate = 0;
    Screen.Params.VSync = false;
    Screen.Windowed = false;
    Screen.FsGameFrame = kFrame_Proportional;
    Screen.WinGameFrame = kFrame_Round;
}
