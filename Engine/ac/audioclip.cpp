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

#include "util/wgt2allg.h"
#include "ac/audioclip.h"
#include "ac/audiochannel.h"
#include "ac/gamesetupstruct.h"
#include "media/audio/audio.h"


extern GameSetupStruct game;
extern ScriptAudioChannel scrAudioChannel[MAX_SOUND_CHANNELS + 1];

int AudioClip_GetFileType(ScriptAudioClip *clip)
{
    return game.audioClips[clip->id].fileType;
}

int AudioClip_GetType(ScriptAudioClip *clip)
{
    return game.audioClips[clip->id].type;
}
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
