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
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_audiochannel.h"

extern GameSetupStruct game;
extern ScriptAudioChannel scrAudioChannel[MAX_SOUND_CHANNELS + 1];
extern RuntimeScriptValue GlobalReturnValue;
extern CCAudioChannel ccDynamicAudio;

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
    ScriptAudioChannel *sc_ch = play_audio_clip(clip, priority, repeat, 0, false);
    GlobalReturnValue.SetDynamicObject(sc_ch, &ccDynamicAudio);
    return sc_ch;
}

ScriptAudioChannel* AudioClip_PlayFrom(ScriptAudioClip *clip, int position, int priority, int repeat)
{
    ScriptAudioChannel *sc_ch = play_audio_clip(clip, priority, repeat, position, false);
    GlobalReturnValue.SetDynamicObject(sc_ch, &ccDynamicAudio);
    return sc_ch;
}

ScriptAudioChannel* AudioClip_PlayQueued(ScriptAudioClip *clip, int priority, int repeat)
{
    ScriptAudioChannel *sc_ch = play_audio_clip(clip, priority, repeat, 0, true);
    GlobalReturnValue.SetDynamicObject(sc_ch, &ccDynamicAudio);
    return sc_ch;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// int | ScriptAudioClip *clip
RuntimeScriptValue Sc_AudioClip_GetFileType(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetFileType)
}

// int | ScriptAudioClip *clip
RuntimeScriptValue Sc_AudioClip_GetType(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetType)
}

// int | ScriptAudioClip *clip
RuntimeScriptValue Sc_AudioClip_GetIsAvailable(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetIsAvailable)
}

// void | ScriptAudioClip *clip
RuntimeScriptValue Sc_AudioClip_Stop(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioClip, AudioClip_Stop)
}

// ScriptAudioChannel* | ScriptAudioClip *clip, int priority, int repeat
RuntimeScriptValue Sc_AudioClip_Play(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT2(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_Play)
}

// ScriptAudioChannel* | ScriptAudioClip *clip, int position, int priority, int repeat
RuntimeScriptValue Sc_AudioClip_PlayFrom(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT3(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_PlayFrom)
}

// ScriptAudioChannel* | ScriptAudioClip *clip, int priority, int repeat
RuntimeScriptValue Sc_AudioClip_PlayQueued(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT2(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_PlayQueued)
}

void RegisterAudioClipAPI()
{
    ccAddExternalObjectFunction("AudioClip::Play^2",            Sc_AudioClip_Play);
    ccAddExternalObjectFunction("AudioClip::PlayFrom^3",        Sc_AudioClip_PlayFrom);
    ccAddExternalObjectFunction("AudioClip::PlayQueued^2",      Sc_AudioClip_PlayQueued);
    ccAddExternalObjectFunction("AudioClip::Stop^0",            Sc_AudioClip_Stop);
    ccAddExternalObjectFunction("AudioClip::get_FileType",      Sc_AudioClip_GetFileType);
    ccAddExternalObjectFunction("AudioClip::get_IsAvailable",   Sc_AudioClip_GetIsAvailable);
    ccAddExternalObjectFunction("AudioClip::get_Type",          Sc_AudioClip_GetType);
}
