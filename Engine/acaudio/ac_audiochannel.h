#ifndef __AC_AUDIOCHANNEL_H
#define __AC_AUDIOCHANNEL_H

#include "acrun/ac_ccdynamicobject.h"
#include "acaudio/ac_audioclip.h"

#define MAX_SOUND_CHANNELS 8
#define SPECIAL_CROSSFADE_CHANNEL 8

struct ScriptAudioChannel
{
    int id;
    int reserved;
};

struct CCAudioChannel : AGSCCDynamicObject {
    virtual const char *GetType();
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    virtual void Unserialize(int index, const char *serializedData, int dataSize);
};

int AudioChannel_GetID(ScriptAudioChannel *channel);
int AudioChannel_GetIsPlaying(ScriptAudioChannel *channel);
int AudioChannel_GetPanning(ScriptAudioChannel *channel);
void AudioChannel_SetPanning(ScriptAudioChannel *channel, int newPanning);
ScriptAudioClip* AudioChannel_GetPlayingClip(ScriptAudioChannel *channel);
int AudioChannel_GetPosition(ScriptAudioChannel *channel);
int AudioChannel_GetPositionMs(ScriptAudioChannel *channel);
int AudioChannel_GetLengthMs(ScriptAudioChannel *channel);
int AudioChannel_GetVolume(ScriptAudioChannel *channel);
int AudioChannel_SetVolume(ScriptAudioChannel *channel, int newVolume);
void AudioChannel_Stop(ScriptAudioChannel *channel);
void AudioChannel_Seek(ScriptAudioChannel *channel, int newPosition);
void AudioChannel_SetRoomLocation(ScriptAudioChannel *channel, int xPos, int yPos);
int AudioClip_GetFileType(ScriptAudioClip *clip);
int AudioClip_GetType(ScriptAudioClip *clip);
//const char *CCAudioChannel::GetType();
//int CCAudioChannel::Serialize(const char *address, char *buffer, int bufsize);
//void CCAudioChannel::Unserialize(int index, const char *serializedData, int dataSize);

#endif // __AC_AUDIOCHANNEL_H