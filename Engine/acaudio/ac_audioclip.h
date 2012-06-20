#ifndef __AC_AUDIOCLIP2_H
#define __AC_AUDIOCLIP2_H

#include "ac/dynobj/cc_agsdynamicobject.h"
#include "ac/ac_audioclip.h"

struct ScriptAudioChannel;



int AudioClip_GetIsAvailable(ScriptAudioClip *clip);
void AudioClip_Stop(ScriptAudioClip *clip);
ScriptAudioChannel* AudioClip_Play(ScriptAudioClip *clip, int priority, int repeat);
ScriptAudioChannel* AudioClip_PlayFrom(ScriptAudioClip *clip, int position, int priority, int repeat);
ScriptAudioChannel* AudioClip_PlayQueued(ScriptAudioClip *clip, int priority, int repeat);

void BuildAudioClipArray();


//const char *CCAudioClip::GetType();
//int CCAudioClip::Serialize(const char *address, char *buffer, int bufsize);
//void CCAudioClip::Unserialize(int index, const char *serializedData, int dataSize);

#endif // __AC_AUDIOCLIP2_H