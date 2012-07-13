
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
void    System_SetNumLock(int newValue);
int     System_GetVsync();
void    System_SetVsync(int newValue);
int     System_GetWindowed();
int     System_GetSupportsGammaControl();
int     System_GetGamma();
void    System_SetGamma(int newValue);
int     System_GetAudioChannelCount();
ScriptAudioChannel* System_GetAudioChannels(int index);
int     System_GetVolume();
void    System_SetVolume(int newvol);


#endif // __AGS_EE_AC_SYSTEMAUDIO_H
