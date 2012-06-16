
#include <stdio.h>
#include "wgt2allg.h"
#include "acaudio/ac_audioclip.h"
#include "acaudio/ac_audio.h"
#include "ac/ac_gamesetupstruct.h"

extern SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1];
extern ScriptAudioChannel scrAudioChannel[MAX_SOUND_CHANNELS + 1];
extern GameSetupStruct game;


int AudioClip_GetIsAvailable(ScriptAudioClip *clip)
{
    if (get_audio_clip_file_name(clip) != NULL)
        return 1;

    return 0;
}


void AudioClip_Stop(ScriptAudioClip *clip)
{
    for (int i = 0; i < MAX_SOUND_CHANNELS; i++)
    {
        if ((channels[i] != NULL) && (!channels[i]->done) && (channels[i]->sourceClip == clip))
        {
            AudioChannel_Stop(&scrAudioChannel[i]);
        }
    }
}

ScriptAudioChannel* AudioClip_Play(ScriptAudioClip *clip, int priority, int repeat)
{
    return play_audio_clip(clip, priority, repeat, 0, false);
}

ScriptAudioChannel* AudioClip_PlayFrom(ScriptAudioClip *clip, int position, int priority, int repeat)
{
    return play_audio_clip(clip, priority, repeat, position, false);
}

ScriptAudioChannel* AudioClip_PlayQueued(ScriptAudioClip *clip, int priority, int repeat)
{
    return play_audio_clip(clip, priority, repeat, 0, true);
}


const char *CCAudioClip::GetType() {
    return "AudioClip";
}

int CCAudioClip::Serialize(const char *address, char *buffer, int bufsize) {
    ScriptAudioClip *ach = (ScriptAudioClip*)address;
    StartSerialize(buffer);
    SerializeInt(ach->id);
    return EndSerialize();
}

void CCAudioClip::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int id = UnserializeInt();
    ccRegisterUnserializedObject(index, &game.audioClips[id], this);
}
