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
#include <SDL.h>
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/dynobj/cc_audiochannel.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_debug.h"
#include "ac/global_translation.h"
#include "ac/mouse.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/dynobj/scriptsystem.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "gfx/graphicsdriver.h"
#include "main/config.h"
#include "main/graphics_mode.h"
#include "main/engine.h"
#include "main/main.h"
#include "media/audio/audio_core.h"
#include "media/audio/audio_system.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern GameState play;
extern ScriptAudioChannel scrAudioChannel[MAX_GAME_CHANNELS];
extern ScriptSystem scsystem;
extern IGraphicsDriver *gfxDriver;
extern CCAudioChannel ccDynamicAudio;
extern volatile bool switched_away;

bool System_GetHasInputFocus()
{
    return !switched_away;
}

int System_GetColorDepth() {
    return scsystem.coldepth;
}

int System_GetOS() {
    return scsystem.os;
}

// [IKM] 2014-09-21
// IMPORTANT NOTE on System.ScreenWidth and System.ScreenHeight:
// It appears that in AGS these properties were not defining actual window size
// in pixels, but rather game frame size, which could include black borders,
// in 'native' (unscaled) pixels. This was due the specifics of how graphics
// modes were implemented in previous versions.
// 
// Quote from the old manual:
// "Returns the actual screen width that the game is running at. If a graphic
//  filter is in use, the resolution returned will be that before any
//  stretching by the filter has been applied. If widescreen side borders are
//  enabled, the screen width reported will include the size of these borders."
//
// The key words are "the resolution returned will be that BEFORE any
// stretching by the filter has been applied".
//
// Since now the letterbox and pillarbox borders are handled by graphics
// renderer and are not part of the game anymore, these properties should
// return strictly native game size. This is required for backwards
// compatibility.
//
int System_GetScreenWidth() {
    return game.GetGameRes().Width;
}

int System_GetScreenHeight() {
    return game.GetGameRes().Height;
}

int System_GetViewportHeight() {
    return game_to_data_coord(play.GetMainViewport().GetHeight());
}

int System_GetViewportWidth() {
    return game_to_data_coord(play.GetMainViewport().GetWidth());
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
    usetup.Screen.Params.VSync = vsync;
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
    return play.digital_master_volume;
}

void System_SetVolume(int newvol) 
{
    if ((newvol < 0) || (newvol > 100))
        quit("!System.Volume: invalid volume - must be from 0-100");

    if (newvol == play.digital_master_volume)
        return;

    play.digital_master_volume = newvol;
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

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/dynobj/scriptstring.h"

extern ScriptString myScriptStringImpl;

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
RuntimeScriptValue Sc_System_GetScreenHeight(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetScreenHeight);
}

// int ()
RuntimeScriptValue Sc_System_GetScreenWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetScreenWidth);
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
RuntimeScriptValue Sc_System_GetViewportHeight(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetViewportHeight);
}

// int ()
RuntimeScriptValue Sc_System_GetViewportWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetViewportWidth);
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
    API_SCALL_VOID(save_config_file);
}

RuntimeScriptValue Sc_System_Log(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF_PURE(Sc_System_Log, 2);
    Debug::Printf(kDbgGroup_Script, (MessageType)params[0].IValue, "%s", scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

void ScPl_System_Log(CharacterInfo *chaa, int message_type, const char *texx, ...)
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
        { "System::get_Gamma",                API_FN_PAIR(System_GetGamma) },
        { "System::set_Gamma",                API_FN_PAIR(System_SetGamma) },
        { "System::get_HardwareAcceleration", API_FN_PAIR(System_GetHardwareAcceleration) },
        { "System::get_HasInputFocus",        API_FN_PAIR(System_GetHasInputFocus) },
        { "System::get_NumLock",              API_FN_PAIR(System_GetNumLock) },
        { "System::get_OperatingSystem",      API_FN_PAIR(System_GetOS) },
        { "System::get_RenderAtScreenResolution", API_FN_PAIR(System_GetRenderAtScreenResolution) },
        { "System::set_RenderAtScreenResolution", API_FN_PAIR(System_SetRenderAtScreenResolution) },
        { "System::get_RuntimeInfo",          API_FN_PAIR(System_GetRuntimeInfo) },
        { "System::get_ScreenHeight",         API_FN_PAIR(System_GetScreenHeight) },
        { "System::get_ScreenWidth",          API_FN_PAIR(System_GetScreenWidth) },
        { "System::get_ScrollLock",           API_FN_PAIR(System_GetScrollLock) },
        { "System::get_SupportsGammaControl", API_FN_PAIR(System_GetSupportsGammaControl) },
        { "System::get_Version",              API_FN_PAIR(System_GetVersion) },
        { "SystemInfo::get_Version",          API_FN_PAIR(System_GetVersion) },
        { "System::get_ViewportHeight",       API_FN_PAIR(System_GetViewportHeight) },
        { "System::get_ViewportWidth",        API_FN_PAIR(System_GetViewportWidth) },
        { "System::get_Volume",               API_FN_PAIR(System_GetVolume) },
        { "System::set_Volume",               API_FN_PAIR(System_SetVolume) },
        { "System::get_VSync",                API_FN_PAIR(System_GetVsync) },
        { "System::set_VSync",                API_FN_PAIR(System_SetVsync) },
        { "System::get_Windowed",             API_FN_PAIR(System_GetWindowed) },
        { "System::set_Windowed",             API_FN_PAIR(System_SetWindowed) },
        
        { "System::SaveConfigToFile",         Sc_System_SaveConfigToFile, save_config_file },
        { "System::Log^102",                  Sc_System_Log, ScPl_System_Log },
    };

    ccAddExternalFunctions(system_api);
}
