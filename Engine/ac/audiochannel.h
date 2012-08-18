
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__AUDIOCHANNEL_H
#define __AGS_EE_AC__AUDIOCHANNEL_H

#include "ac/dynobj/scriptaudioclip.h"
#include "ac/dynobj/scriptaudiochannel.h"

int     AudioChannel_GetID(ScriptAudioChannel *channel);
int     AudioChannel_GetIsPlaying(ScriptAudioChannel *channel);
int     AudioChannel_GetPanning(ScriptAudioChannel *channel);
void    AudioChannel_SetPanning(ScriptAudioChannel *channel, int newPanning);
ScriptAudioClip* AudioChannel_GetPlayingClip(ScriptAudioChannel *channel);
int     AudioChannel_GetPosition(ScriptAudioChannel *channel);
int     AudioChannel_GetPositionMs(ScriptAudioChannel *channel);
int     AudioChannel_GetLengthMs(ScriptAudioChannel *channel);
int     AudioChannel_GetVolume(ScriptAudioChannel *channel);
int     AudioChannel_SetVolume(ScriptAudioChannel *channel, int newVolume);
void    AudioChannel_Stop(ScriptAudioChannel *channel);
void    AudioChannel_Seek(ScriptAudioChannel *channel, int newPosition);
void    AudioChannel_SetRoomLocation(ScriptAudioChannel *channel, int xPos, int yPos);

#endif // __AGS_EE_AC__AUDIOCHANNEL_H
