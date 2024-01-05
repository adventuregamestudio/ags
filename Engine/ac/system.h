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
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SYSTEMAUDIO_H
#define __AGS_EE_AC__SYSTEMAUDIO_H

#include "ac/dynobj/scriptaudiochannel.h"

int     System_GetColorDepth();
int     System_GetOS();
int     System_GetScreenWidth();
int     System_GetScreenHeight();
int     System_GetViewportHeight();
int     System_GetViewportWidth();
const char *System_GetVersion();
int     System_GetHardwareAcceleration();
int     System_GetNumLock();
int     System_GetCapsLock();
int     System_GetScrollLock();
int     System_GetVsync();
void    System_SetVsync(int newValue);
void    System_SetVSyncInternal(bool vsync);
int     System_GetWindowed();
int     System_GetSupportsGammaControl();
int     System_GetGamma();
void    System_SetGamma(int newValue);
int     System_GetAudioChannelCount();
ScriptAudioChannel* System_GetAudioChannels(int index);
int     System_GetVolume();
void    System_SetVolume(int newvol);
const char *System_GetRuntimeInfo();


#endif // __AGS_EE_AC_SYSTEMAUDIO_H
