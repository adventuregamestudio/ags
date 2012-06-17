#ifndef __AC_AUDIOCLIP2_H
#define __AC_AUDIOCLIP2_H

#include "acrun/ac_ccdynamicobject.h"
#include "ac/ac_audioclip.h"

struct ScriptAudioChannel;

struct CCAudioClip : AGSCCDynamicObject {
    virtual const char *GetType();
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    virtual void Unserialize(int index, const char *serializedData, int dataSize);
};

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