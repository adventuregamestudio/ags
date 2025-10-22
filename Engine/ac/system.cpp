//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <SDL.h>
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/dynobj/cc_audiochannel.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_debug.h"
#include "ac/global_translation.h"
#include "ac/mouse.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/dynobj/scriptsystem.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "gfx/graphicsdriver.h"
#include "gfx/gfxfilter.h"
#include "main/config.h"
#include "main/game_run.h"
#include "main/graphics_mode.h"
#include "main/engine.h"
#include "ac/global_translation.h"
#include "main/main.h"
#include "media/audio/audio_core.h"
#include "media/audio/audio_system.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern ScriptAudioChannel scrAudioChannel[MAX_GAME_CHANNELS];
extern ScriptSystem scsystem;
extern IGraphicsDriver *gfxDriver;
extern CCAudioChannel ccDynamicAudio;
extern volatile bool switched_away;
extern FPSDisplayMode display_fps;

bool System_GetHasInputFocus()
{
    return !switched_away;
}

int System_GetColorDepth() {
    return scsystem.coldepth;
}

int System_GetFPS()
{
    float fps = get_real_fps();
    return std::isnan(fps) ? -1 : static_cast<int>(std::round(fps));
}

int System_GetOS() {
    return scsystem.os;
}

const char *System_GetVersion() {
    return CreateNewScriptString(EngineVersion.LongString);
}

int System_GetHardwareAcceleration() 
{
    return gfxDriver->HasAcceleratedTransform() ? 1 : 0;
}

int System_GetNumLock()
{
    SDL_Keymod mod_state = SDL_GetModState();
    return (mod_state & KMOD_NUM) ? 1 : 0;
}

int System_GetCapsLock()
{
    SDL_Keymod mod_state = SDL_GetModState();
    return (mod_state & KMOD_CAPS) ? 1 : 0;
}

int System_GetScrollLock()
{
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    return (state[SDL_SCANCODE_SCROLLLOCK]) ? 1 : 0;
}

int System_GetVsync() {
    return scsystem.vsync;
}

void System_SetVsync(int newValue) {
    if (gfxDriver->DoesSupportVsyncToggle()) {
        System_SetVSyncInternal(newValue != 0);
    }
}

void System_SetVSyncInternal(bool vsync) {
    scsystem.vsync = vsync;
    usetup.Display.VSync = vsync;
}

int System_GetWindowed() {
    return scsystem.windowed;
}

void System_SetWindowed(int windowed)
{
    if (windowed != scsystem.windowed)
        engine_try_switch_windowed_gfxmode();
}

int System_GetSupportsGammaControl() {
    return gfxDriver->SupportsGammaControl();
}

int System_GetGamma() {
    return play.gamma_adjustment;
}

void System_SetGamma(int newValue) {
    if ((newValue < 0) || (newValue > 200))
        quitprintf("!System.Gamma: value must be between 0-200 (not %d)", newValue);

    if (play.gamma_adjustment != newValue) {
        debug_script_log("Gamma control set to %d", newValue);
        play.gamma_adjustment = newValue;

        if (gfxDriver->SupportsGammaControl())
            gfxDriver->SetGamma(newValue);
    }
}

int System_GetAudioChannelCount()
{
    return game.numGameChannels;
}

ScriptAudioChannel* System_GetAudioChannels(int index)
{
    if ((index < 0) || (index >= game.numGameChannels))
        quitprintf("!System.AudioChannels: invalid sound channel index %d, supported %d - %d", index, 0, game.numGameChannels - 1);

    return &scrAudioChannel[index];
}

int System_GetVolume() 
{
    return play.audio_master_volume;
}

void System_SetVolume(int newvol) 
{
    if ((newvol < 0) || (newvol > 100))
        quit("!System.Volume: invalid volume - must be from 0-100");

    if (newvol == play.audio_master_volume)
        return;

    play.audio_master_volume = newvol;
    auto newvol_f = static_cast<float>(newvol) / 100.0;
    audio_core_set_master_volume(newvol_f);
}

const char* System_GetRuntimeInfo()
{
    String runtimeInfo = GetRuntimeInfo();

    return CreateNewScriptString(runtimeInfo.GetCStr());
}

int System_GetRenderAtScreenResolution()
{
    return usetup.RenderAtScreenRes;
}

void System_SetRenderAtScreenResolution(int enable)
{
    usetup.RenderAtScreenRes = enable != 0;
}

int System_GetEngineInteger(int value_id, int index)
{
    int value = 0;
    if (!GetEngineInteger(value, static_cast<EngineValueID>(value_id), index))
        debug_script_warn("System.GetEngineInteger: undefined engine value %d (#%d), or not an integer value", value_id, index);
    return value;
}

const char* System_GetEngineString(int value_id, int index)
{
    String value;
    if (!GetEngineString(value, static_cast<EngineValueID>(value_id), index))
        debug_script_warn("System.GetEngineString: undefined engine value %d (#%d)", value_id, index);
    return CreateNewScriptString(value.GetCStr());
}

bool GetEngineInteger(int &value, EngineValueID value_id, int index)
{
    switch (value_id)
    {
    case ENGINE_VALUE_I_SPRCACHE_MAXNORMAL:
        value = static_cast<int>(spriteset.GetMaxCacheSize() / 1024u); return true;
    case ENGINE_VALUE_I_SPRCACHE_NORMAL:
        value = static_cast<int>(spriteset.GetCacheSize() / 1024u); return true;
    case ENGINE_VALUE_I_SPRCACHE_LOCKED:
        value = static_cast<int>(spriteset.GetLockedSize() / 1024u); return true;
    case ENGINE_VALUE_I_SPRCACHE_EXTERNAL:
        value = static_cast<int>(spriteset.GetExternalSize() / 1024u); return true;
    case ENGINE_VALUE_I_TEXCACHE_MAXNORMAL: /* fall-through */
    case ENGINE_VALUE_I_TEXCACHE_NORMAL:
    {
        size_t max_txcached, total_txcached, total_txlocked, total_txext;
        texturecache_get_state(max_txcached, total_txcached, total_txlocked, total_txext);
        switch (value_id)
        {
        case ENGINE_VALUE_I_TEXCACHE_MAXNORMAL: value = static_cast<int>(max_txcached / 1024u); return true;
        case ENGINE_VALUE_I_TEXCACHE_NORMAL: value = static_cast<int>(total_txcached / 1024u); return true;
        default: return false; // should not happen...
        }
    }
    case ENGINE_VALUE_I_FPS_MAX:
        value = isTimerFpsMaxed() ? -1 : frames_per_second; return true;
    case ENGINE_VALUE_I_FPS:
    {
        float fps = get_real_fps();
        value = std::isnan(fps) ? -1 : static_cast<int>(std::round(fps));
        return true;
    }
    default: return false;
    }
}

bool GetEngineString(AGS::Common::String &value, EngineValueID value_id, int index)
{
    switch (value_id)
    {
    case ENGINE_VALUE_SI_VALUENAME: value = GetEngineValueName(static_cast<EngineValueID>(index)); return true;
    case ENGINE_VALUE_S_ENGINE_NAME: value = get_engine_name(); return true;
    case ENGINE_VALUE_S_ENGINE_VERSION: value = EngineVersion.LongString; return true;
    case ENGINE_VALUE_S_ENGINE_VERSION_FULL: value = get_engine_version_and_build(); return true;
    case ENGINE_VALUE_S_DISPLAY_MODE_STR:
        value = "";
        if (gfxDriver)
        {
            DisplayMode mode = gfxDriver->GetDisplayMode();
            value = String::FromFormat("%d x %d %d-bit %s", mode.Width, mode.Height, mode.ColorDepth,
                (mode.IsWindowed() ? " W" : (mode.IsRealFullscreen() ? " F" : " FD")));
        }
        return true;
    case ENGINE_VALUE_S_GFXRENDERER: value = (gfxDriver ? gfxDriver->GetDriverName() : ""); return true;
    case ENGINE_VALUE_S_GFXFILTER:
        value = "";
        if (gfxDriver)
        {
            auto filter = gfxDriver->GetGraphicsFilter();
            if (filter)
                value = filter->GetInfo().Name;
        }
        return true;
    default:
        // Attempt to retrieve an integer value and print that into string
        {
            int ival = 0;
            if (GetEngineInteger(ival, value_id, index))
            {
                value = String::FromFormat("%d", ival);
                return true;
            }
        }
        return false;
    }
}

String GetEngineValueName(EngineValueID value_id)
{
    switch (value_id)
    {
    case ENGINE_VALUE_SI_VALUENAME: return "Engine Value Name";
    case ENGINE_VALUE_S_ENGINE_NAME: return "Engine Name";
    case ENGINE_VALUE_S_ENGINE_VERSION: return "Engine Version String";
    case ENGINE_VALUE_S_ENGINE_VERSION_FULL: return "Engine Version String (Full)";
    case ENGINE_VALUE_S_DISPLAY_MODE_STR: return "Display Mode String";
    case ENGINE_VALUE_S_GFXRENDERER: return "Graphics Renderer Name";
    case ENGINE_VALUE_S_GFXFILTER: return "Graphics Filter Name";
    case ENGINE_VALUE_I_SPRCACHE_MAXNORMAL: return "Sprite cache: normal capacity (KB)";
    case ENGINE_VALUE_I_SPRCACHE_NORMAL: return "Sprite cache: normal size (KB)";
    case ENGINE_VALUE_I_SPRCACHE_LOCKED: return "Sprite cache: locked size (KB)";
    case ENGINE_VALUE_I_SPRCACHE_EXTERNAL: return "Sprite cache: ext sprites size (KB)";
    case ENGINE_VALUE_I_TEXCACHE_MAXNORMAL: return "Texture cache: normal capacity (KB)";
    case ENGINE_VALUE_I_TEXCACHE_NORMAL: return "Texture cache: normal size (KB)";
    case ENGINE_VALUE_I_FPS_MAX: return "FPS cap";
    case ENGINE_VALUE_I_FPS: return "FPS real";
    default: return "";
    }
}

bool System_GetDisplayFPS() {
    return display_fps != kFPS_Hide;
}

void System_SetDisplayFPS(bool show_fps)
{
    if (display_fps != kFPS_Forced)
        display_fps = show_fps ? kFPS_Display : kFPS_Hide;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/dynobj/scriptstring.h"

// int ()
RuntimeScriptValue Sc_System_GetAudioChannelCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetAudioChannelCount);
}

// ScriptAudioChannel* (int index)
RuntimeScriptValue Sc_System_GetAudioChannels(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(ScriptAudioChannel, ccDynamicAudio, System_GetAudioChannels);
}

// int ()
RuntimeScriptValue Sc_System_GetCapsLock(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetCapsLock);
}

// int ()
RuntimeScriptValue Sc_System_GetColorDepth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetColorDepth);
}

RuntimeScriptValue Sc_System_GetFPS(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetFPS);
}

// int ()
RuntimeScriptValue Sc_System_GetGamma(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetGamma);
}

// void (int newValue)
RuntimeScriptValue Sc_System_SetGamma(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(System_SetGamma);
}

// int () 
RuntimeScriptValue Sc_System_GetHardwareAcceleration(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetHardwareAcceleration);
}

RuntimeScriptValue Sc_System_GetHasInputFocus(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(System_GetHasInputFocus);
}

// int ()
RuntimeScriptValue Sc_System_GetNumLock(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetNumLock);
}

// int ()
RuntimeScriptValue Sc_System_GetOS(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetOS);
}

// int ()
RuntimeScriptValue Sc_System_GetScrollLock(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetScrollLock);
}

// int ()
RuntimeScriptValue Sc_System_GetSupportsGammaControl(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetSupportsGammaControl);
}

// const char *()
RuntimeScriptValue Sc_System_GetVersion(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, System_GetVersion);
}

// int ()
RuntimeScriptValue Sc_System_GetVolume(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetVolume);
}

// void (int newvol)
RuntimeScriptValue Sc_System_SetVolume(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(System_SetVolume);
}

// int ()
RuntimeScriptValue Sc_System_GetVsync(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetVsync);
}

// void (int newValue)
RuntimeScriptValue Sc_System_SetVsync(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(System_SetVsync);
}

RuntimeScriptValue Sc_System_GetWindowed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetWindowed);
}

RuntimeScriptValue Sc_System_SetWindowed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(System_SetWindowed);
}

// const char *()
RuntimeScriptValue Sc_System_GetRuntimeInfo(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, System_GetRuntimeInfo);
}

RuntimeScriptValue Sc_System_GetRenderAtScreenResolution(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetRenderAtScreenResolution);
}

RuntimeScriptValue Sc_System_SetRenderAtScreenResolution(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(System_SetRenderAtScreenResolution);
}

RuntimeScriptValue Sc_System_SaveConfigToFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(save_runtime_config_file);
}

RuntimeScriptValue Sc_System_Log(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF_PURE(Sc_System_Log, 2);
    Debug::Printf(kDbgGroup_Script, (MessageType)params[0].IValue, "%s", scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_System_GetEngineInteger(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(System_GetEngineInteger);
}

RuntimeScriptValue Sc_System_GetEngineString(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(const char, myScriptStringImpl, System_GetEngineString);
}

RuntimeScriptValue Sc_System_GetDisplayFPS(const RuntimeScriptValue* params, int32_t param_count)
{
    API_SCALL_BOOL(System_GetDisplayFPS);
}

RuntimeScriptValue Sc_System_SetDisplayFPS(const RuntimeScriptValue* params, int32_t param_count)
{
    API_SCALL_VOID_PBOOL(System_SetDisplayFPS);
}

//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

void ScPl_System_Log(int message_type, const char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF_PURE(texx);
    Debug::Printf(kDbgGroup_Script, (MessageType)message_type, scsf_buffer);
}


void RegisterSystemAPI()
{
    ScFnRegister system_api[] = {
        { "System::get_AudioChannelCount",    API_FN_PAIR(System_GetAudioChannelCount) },
        { "System::geti_AudioChannels",       API_FN_PAIR(System_GetAudioChannels) },
        { "System::get_CapsLock",             API_FN_PAIR(System_GetCapsLock) },
        { "System::get_ColorDepth",           API_FN_PAIR(System_GetColorDepth) },
        { "System::get_FPS",                  API_FN_PAIR(System_GetFPS) },
        { "System::get_Gamma",                API_FN_PAIR(System_GetGamma) },
        { "System::set_Gamma",                API_FN_PAIR(System_SetGamma) },
        { "System::get_HardwareAcceleration", API_FN_PAIR(System_GetHardwareAcceleration) },
        { "System::get_HasInputFocus",        API_FN_PAIR(System_GetHasInputFocus) },
        { "System::get_NumLock",              API_FN_PAIR(System_GetNumLock) },
        { "System::get_OperatingSystem",      API_FN_PAIR(System_GetOS) },
        { "System::get_RenderAtScreenResolution", API_FN_PAIR(System_GetRenderAtScreenResolution) },
        { "System::set_RenderAtScreenResolution", API_FN_PAIR(System_SetRenderAtScreenResolution) },
        { "System::get_RuntimeInfo",          API_FN_PAIR(System_GetRuntimeInfo) },
        { "System::get_ScrollLock",           API_FN_PAIR(System_GetScrollLock) },
        { "System::get_SupportsGammaControl", API_FN_PAIR(System_GetSupportsGammaControl) },
        { "System::get_Version",              API_FN_PAIR(System_GetVersion) },
        { "SystemInfo::get_Version",          API_FN_PAIR(System_GetVersion) },
        { "System::get_Volume",               API_FN_PAIR(System_GetVolume) },
        { "System::set_Volume",               API_FN_PAIR(System_SetVolume) },
        { "System::get_VSync",                API_FN_PAIR(System_GetVsync) },
        { "System::set_VSync",                API_FN_PAIR(System_SetVsync) },
        { "System::get_Windowed",             API_FN_PAIR(System_GetWindowed) },
        { "System::set_Windowed",             API_FN_PAIR(System_SetWindowed) },
        
        { "System::SaveConfigToFile",         Sc_System_SaveConfigToFile, save_runtime_config_file },
        { "System::Log^102",                  Sc_System_Log, ScPl_System_Log },
        { "System::GetEngineInteger^2",       API_FN_PAIR(System_GetEngineInteger) },
        { "System::GetEngineString^2",        API_FN_PAIR(System_GetEngineString) },
        { "System::get_DisplayFPS",           API_FN_PAIR(System_GetDisplayFPS) },
        { "System::set_DisplayFPS",           API_FN_PAIR(System_SetDisplayFPS) },
    };

    ccAddExternalFunctions(system_api);
}
