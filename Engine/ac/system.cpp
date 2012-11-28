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
extern RuntimeScriptValue GlobalReturnValue;
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
    return CreateNewScriptStringAsRetVal(ACI_VERSION_TEXT);
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

    GlobalReturnValue.SetDynamicObject(&scrAudioChannel[index], &ccDynamicAudio);
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
