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

#include "ac/common.h"
#include "util/wgt2allg.h"
#include "gfx/ali3d.h"
#include "media/audio/audiodefines.h"
#include "ac/draw.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "ac/mouse.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/dynobj/scriptsystem.h"
#include "debug/debug_log.h"
#include "main/main.h"
#include "gfx/graphicsdriver.h"
#include "ac/dynobj/cc_audiochannel.h"

extern GameSetup usetup;
extern GameState play;
extern SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1];
extern ScriptAudioChannel scrAudioChannel[MAX_SOUND_CHANNELS + 1];
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern ScriptSystem scsystem;
extern int scrnwid,scrnhit;
extern IGraphicsDriver *gfxDriver;
extern CCAudioChannel ccDynamicAudio;


int System_GetColorDepth() {
    return final_col_dep;
}

int System_GetOS() {
    return scsystem.os;
}

int System_GetScreenWidth() {
    return final_scrn_wid;
}

int System_GetScreenHeight() {
    return final_scrn_hit;
}

int System_GetViewportHeight() {
    return divide_down_coordinate(scrnhit);
}

int System_GetViewportWidth() {
    return divide_down_coordinate(scrnwid);
}

const char *System_GetVersion() {
    return CreateNewScriptString(EngineVersion.LongString);
}

int System_GetHardwareAcceleration() 
{
    return gfxDriver->HasAcceleratedStretchAndFlip() ? 1 : 0;
}

int System_GetNumLock()
{
    return (key_shifts & KB_NUMLOCK_FLAG) ? 1 : 0;
}

int System_GetCapsLock()
{
    return (key_shifts & KB_CAPSLOCK_FLAG) ? 1 : 0;
}

int System_GetScrollLock()
{
    return (key_shifts & KB_SCROLOCK_FLAG) ? 1 : 0;
}

void System_SetNumLock(int newValue)
{
    // doesn't work ... maybe allegro doesn't implement this on windows
    int ledState = key_shifts & (KB_SCROLOCK_FLAG | KB_CAPSLOCK_FLAG);
    if (newValue)
    {
        ledState |= KB_NUMLOCK_FLAG;
    }
    set_leds(ledState);
}

int System_GetVsync() {
    return scsystem.vsync;
}

void System_SetVsync(int newValue) {
    scsystem.vsync = newValue;
}

int System_GetWindowed() {
    if (usetup.windowed)
        return 1;
    return 0;
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
        DEBUG_CONSOLE("Gamma control set to %d", newValue);
        play.gamma_adjustment = newValue;

        if (gfxDriver->SupportsGammaControl())
            gfxDriver->SetGamma(newValue);
    }
}

int System_GetAudioChannelCount()
{
    return MAX_SOUND_CHANNELS;
}

ScriptAudioChannel* System_GetAudioChannels(int index)
{
    if ((index < 0) || (index >= MAX_SOUND_CHANNELS))
        quit("!System.AudioChannels: invalid sound channel index");

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
    set_volume((newvol * 255) / 100, (newvol * 255) / 100);

    // allegro's set_volume can lose the volumes of all the channels
    // if it was previously set low; so restore them
    for (int i = 0; i <= MAX_SOUND_CHANNELS; i++) 
    {
        if ((channels[i] != NULL) && (channels[i]->done == 0)) 
        {
            channels[i]->set_volume(channels[i]->vol);
        }
    }
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

// int ()
RuntimeScriptValue Sc_System_GetNumLock(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetNumLock);
}

// void (int newValue)
RuntimeScriptValue Sc_System_SetNumLock(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(System_SetNumLock);
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

// int ()
RuntimeScriptValue Sc_System_GetWindowed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetWindowed);
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
    ccAddExternalStaticFunction("System::get_NumLock",              Sc_System_GetNumLock);
    ccAddExternalStaticFunction("System::set_NumLock",              Sc_System_SetNumLock);
    ccAddExternalStaticFunction("System::get_OperatingSystem",      Sc_System_GetOS);
    ccAddExternalStaticFunction("System::get_ScreenHeight",         Sc_System_GetScreenHeight);
    ccAddExternalStaticFunction("System::get_ScreenWidth",          Sc_System_GetScreenWidth);
    ccAddExternalStaticFunction("System::get_ScrollLock",           Sc_System_GetScrollLock);
    ccAddExternalStaticFunction("System::get_SupportsGammaControl", Sc_System_GetSupportsGammaControl);
    ccAddExternalStaticFunction("System::get_Version",              Sc_System_GetVersion);
    ccAddExternalStaticFunction("SystemInfo::get_Version",          Sc_System_GetVersion);
    ccAddExternalStaticFunction("System::get_ViewportHeight",       Sc_System_GetViewportHeight);
    ccAddExternalStaticFunction("System::get_ViewportWidth",        Sc_System_GetViewportWidth);
    ccAddExternalStaticFunction("System::get_Volume",               Sc_System_GetVolume);
    ccAddExternalStaticFunction("System::set_Volume",               Sc_System_SetVolume);
    ccAddExternalStaticFunction("System::get_VSync",                Sc_System_GetVsync);
    ccAddExternalStaticFunction("System::set_VSync",                Sc_System_SetVsync);
    ccAddExternalStaticFunction("System::get_Windowed",             Sc_System_GetWindowed);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("System::get_AudioChannelCount",    (void*)System_GetAudioChannelCount);
    ccAddExternalFunctionForPlugin("System::geti_AudioChannels",       (void*)System_GetAudioChannels);
    ccAddExternalFunctionForPlugin("System::get_CapsLock",             (void*)System_GetCapsLock);
    ccAddExternalFunctionForPlugin("System::get_ColorDepth",           (void*)System_GetColorDepth);
    ccAddExternalFunctionForPlugin("System::get_Gamma",                (void*)System_GetGamma);
    ccAddExternalFunctionForPlugin("System::set_Gamma",                (void*)System_SetGamma);
    ccAddExternalFunctionForPlugin("System::get_HardwareAcceleration", (void*)System_GetHardwareAcceleration);
    ccAddExternalFunctionForPlugin("System::get_NumLock",              (void*)System_GetNumLock);
    ccAddExternalFunctionForPlugin("System::set_NumLock",              (void*)System_SetNumLock);
    ccAddExternalFunctionForPlugin("System::get_OperatingSystem",      (void*)System_GetOS);
    ccAddExternalFunctionForPlugin("System::get_ScreenHeight",         (void*)System_GetScreenHeight);
    ccAddExternalFunctionForPlugin("System::get_ScreenWidth",          (void*)System_GetScreenWidth);
    ccAddExternalFunctionForPlugin("System::get_ScrollLock",           (void*)System_GetScrollLock);
    ccAddExternalFunctionForPlugin("System::get_SupportsGammaControl", (void*)System_GetSupportsGammaControl);
    ccAddExternalFunctionForPlugin("System::get_Version",              (void*)System_GetVersion);
    ccAddExternalFunctionForPlugin("SystemInfo::get_Version",          (void*)System_GetVersion);
    ccAddExternalFunctionForPlugin("System::get_ViewportHeight",       (void*)System_GetViewportHeight);
    ccAddExternalFunctionForPlugin("System::get_ViewportWidth",        (void*)System_GetViewportWidth);
    ccAddExternalFunctionForPlugin("System::get_Volume",               (void*)System_GetVolume);
    ccAddExternalFunctionForPlugin("System::set_Volume",               (void*)System_SetVolume);
    ccAddExternalFunctionForPlugin("System::get_VSync",                (void*)System_GetVsync);
    ccAddExternalFunctionForPlugin("System::set_VSync",                (void*)System_SetVsync);
    ccAddExternalFunctionForPlugin("System::get_Windowed",             (void*)System_GetWindowed);
}
