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
#include "ac/global_translation.h"
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

bool System_HasInputFocus()
{
    return !switched_away;
}

int System_GetColorDepth() {
    return scsystem.coldepth;
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
        quitprintf("!System.AudioChannels: invalid sound channel index %d, supported %d - %d", index, 0, game.numGameChannels);

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
    API_SCALL_BOOL(System_HasInputFocus);
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
    API_SCALL_VOID(save_config_file);
}

RuntimeScriptValue Sc_System_Log(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF_PURE(Sc_System_Log, 2);
    Debug::Printf(kDbgGroup_Script, (MessageType)params[0].IValue, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}



void RegisterSystemAPI()
{
    ccAddExternalStaticFunction("System::get_AudioChannelCount",    Sc_System_GetAudioChannelCount);
    ccAddExternalStaticFunction("System::geti_AudioChannels",       Sc_System_GetAudioChannels);
    ccAddExternalStaticFunction("System::get_CapsLock",             Sc_System_GetCapsLock);
    ccAddExternalStaticFunction("System::get_ColorDepth",           Sc_System_GetColorDepth);
    ccAddExternalStaticFunction("System::get_Gamma",                Sc_System_GetGamma);
    ccAddExternalStaticFunction("System::set_Gamma",                Sc_System_SetGamma);
    ccAddExternalStaticFunction("System::get_HardwareAcceleration", Sc_System_GetHardwareAcceleration);
    ccAddExternalStaticFunction("System::get_HasInputFocus",        Sc_System_GetHasInputFocus);
    ccAddExternalStaticFunction("System::get_NumLock",              Sc_System_GetNumLock);
    ccAddExternalStaticFunction("System::get_OperatingSystem",      Sc_System_GetOS);
    ccAddExternalStaticFunction("System::get_RenderAtScreenResolution", Sc_System_GetRenderAtScreenResolution);
    ccAddExternalStaticFunction("System::set_RenderAtScreenResolution", Sc_System_SetRenderAtScreenResolution);
    ccAddExternalStaticFunction("System::get_RuntimeInfo",          Sc_System_GetRuntimeInfo);
    ccAddExternalStaticFunction("System::get_ScrollLock",           Sc_System_GetScrollLock);
    ccAddExternalStaticFunction("System::get_SupportsGammaControl", Sc_System_GetSupportsGammaControl);
    ccAddExternalStaticFunction("System::get_Version",              Sc_System_GetVersion);
    ccAddExternalStaticFunction("SystemInfo::get_Version",          Sc_System_GetVersion);
    ccAddExternalStaticFunction("System::get_Volume",               Sc_System_GetVolume);
    ccAddExternalStaticFunction("System::set_Volume",               Sc_System_SetVolume);
    ccAddExternalStaticFunction("System::get_VSync",                Sc_System_GetVsync);
    ccAddExternalStaticFunction("System::set_VSync",                Sc_System_SetVsync);
    ccAddExternalStaticFunction("System::get_Windowed",             Sc_System_GetWindowed);
    ccAddExternalStaticFunction("System::set_Windowed",             Sc_System_SetWindowed);

    ccAddExternalStaticFunction("System::SaveConfigToFile",         Sc_System_SaveConfigToFile);
    ccAddExternalStaticFunction("System::Log^102",                  Sc_System_Log);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("System::get_AudioChannelCount",    (void*)System_GetAudioChannelCount);
    ccAddExternalFunctionForPlugin("System::geti_AudioChannels",       (void*)System_GetAudioChannels);
    ccAddExternalFunctionForPlugin("System::get_CapsLock",             (void*)System_GetCapsLock);
    ccAddExternalFunctionForPlugin("System::get_ColorDepth",           (void*)System_GetColorDepth);
    ccAddExternalFunctionForPlugin("System::get_Gamma",                (void*)System_GetGamma);
    ccAddExternalFunctionForPlugin("System::set_Gamma",                (void*)System_SetGamma);
    ccAddExternalFunctionForPlugin("System::get_HardwareAcceleration", (void*)System_GetHardwareAcceleration);
    ccAddExternalFunctionForPlugin("System::get_NumLock",              (void*)System_GetNumLock);
    ccAddExternalFunctionForPlugin("System::get_OperatingSystem",      (void*)System_GetOS);
    ccAddExternalFunctionForPlugin("System::get_RuntimeInfo",          (void*)System_GetRuntimeInfo);
    ccAddExternalFunctionForPlugin("System::get_ScrollLock",           (void*)System_GetScrollLock);
    ccAddExternalFunctionForPlugin("System::get_SupportsGammaControl", (void*)System_GetSupportsGammaControl);
    ccAddExternalFunctionForPlugin("System::get_Version",              (void*)System_GetVersion);
    ccAddExternalFunctionForPlugin("SystemInfo::get_Version",          (void*)System_GetVersion);
    ccAddExternalFunctionForPlugin("System::get_Volume",               (void*)System_GetVolume);
    ccAddExternalFunctionForPlugin("System::set_Volume",               (void*)System_SetVolume);
    ccAddExternalFunctionForPlugin("System::get_VSync",                (void*)System_GetVsync);
    ccAddExternalFunctionForPlugin("System::set_VSync",                (void*)System_SetVsync);
    ccAddExternalFunctionForPlugin("System::get_Windowed",             (void*)System_GetWindowed);
}
