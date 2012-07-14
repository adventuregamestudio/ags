
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__AUDIOCLIP_H
#define __AGS_EE_AC__AUDIOCLIP_H

#include "ac/dynobj/scriptaudioclip.h"
#include "ac/dynobj/scriptaudiochannel.h"

int     AudioClip_GetFileType(ScriptAudioClip *clip);
int     AudioClip_GetType(ScriptAudioClip *clip);
int     AudioClip_GetIsAvailable(ScriptAudioClip *clip);
void    AudioClip_Stop(ScriptAudioClip *clip);
ScriptAudioChannel* AudioClip_Play(ScriptAudioClip *clip, int priority, int repeat);
ScriptAudioChannel* AudioClip_PlayFrom(ScriptAudioClip *clip, int position, int priority, int repeat);
ScriptAudioChannel* AudioClip_PlayQueued(ScriptAudioClip *clip, int priority, int repeat);

#endif // __AGS_EE_AC__AUDIOCLIP_H
