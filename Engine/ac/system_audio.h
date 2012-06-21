
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SYSTEMAUDIO_H
#define __AGS_EE_AC__SYSTEMAUDIO_H

#include "ac/dynobj/scriptaudiochannel.h"

int                 System_GetAudioChannelCount();
ScriptAudioChannel* System_GetAudioChannels(int index);

int System_GetVolume();
void System_SetVolume(int newvol);


#endif // __AGS_EE_AC_SYSTEMAUDIO_H
