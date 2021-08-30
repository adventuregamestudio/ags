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
#include "media/audio/audio_system.h"
#include "ac/asset_helper.h"
#include "ac/audioclip.h"
#include "ac/audiochannel.h"
#include "ac/common.h" // quitprintf
#include "ac/gamesetupstruct.h"
#include "ac/dynobj/cc_audiochannel.h"
#include "script/runtimescriptvalue.h"

extern GameSetupStruct game;
extern ScriptAudioChannel scrAudioChannel[MAX_GAME_CHANNELS];
extern CCAudioChannel ccDynamicAudio;

int AudioClip_GetID(ScriptAudioClip *clip)
{
    return clip->id;
}

int AudioClip_GetFileType(ScriptAudioClip *clip)
{
    return clip->fileType;
}

int AudioClip_GetType(ScriptAudioClip *clip)
{
    return clip->type;
}
int AudioClip_GetIsAvailable(ScriptAudioClip *clip)
{
    return DoesAssetExistInLib(get_audio_clip_assetpath(clip->bundlingType, clip->fileName)) ? 1 : 0;
}

void AudioClip_Stop(ScriptAudioClip *clip)
{
    AudioChannelsLock lock;
    for (int i = NUM_SPEECH_CHANS; i < MAX_GAME_CHANNELS; i++)
    {
        auto* ch = lock.GetChannelIfPlaying(i);
        if ((ch != nullptr) && (ch->sourceClip == clip))
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

ScriptAudioChannel* AudioClip_PlayOnChannel(ScriptAudioClip *clip, int chan, int priority, int repeat)
{
    if (chan < NUM_SPEECH_CHANS || chan >= MAX_GAME_CHANNELS)
        quitprintf("!AudioClip.PlayOnChannel: invalid channel %d, the range is %d - %d",
            chan, NUM_SPEECH_CHANS, MAX_GAME_CHANNELS - 1);
    if (priority == SCR_NO_VALUE)
        priority = clip->defaultPriority;
    if (repeat == SCR_NO_VALUE)
        repeat = clip->defaultRepeat;
    return play_audio_clip_on_channel(chan, clip, priority, repeat, 0);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

RuntimeScriptValue Sc_AudioClip_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetID);
}

// int | ScriptAudioClip *clip
RuntimeScriptValue Sc_AudioClip_GetFileType(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetFileType);
}

// int | ScriptAudioClip *clip
RuntimeScriptValue Sc_AudioClip_GetType(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetType);
}

// int | ScriptAudioClip *clip
RuntimeScriptValue Sc_AudioClip_GetIsAvailable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetIsAvailable);
}

// void | ScriptAudioClip *clip
RuntimeScriptValue Sc_AudioClip_Stop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioClip, AudioClip_Stop);
}

// ScriptAudioChannel* | ScriptAudioClip *clip, int priority, int repeat
RuntimeScriptValue Sc_AudioClip_Play(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT2(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_Play);
}

// ScriptAudioChannel* | ScriptAudioClip *clip, int position, int priority, int repeat
RuntimeScriptValue Sc_AudioClip_PlayFrom(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT3(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_PlayFrom);
}

// ScriptAudioChannel* | ScriptAudioClip *clip, int priority, int repeat
RuntimeScriptValue Sc_AudioClip_PlayQueued(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT2(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_PlayQueued);
}

RuntimeScriptValue Sc_AudioClip_PlayOnChannel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT3(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_PlayOnChannel);
}

void RegisterAudioClipAPI()
{
    ccAddExternalObjectFunction("AudioClip::Play^2",            Sc_AudioClip_Play);
    ccAddExternalObjectFunction("AudioClip::PlayFrom^3",        Sc_AudioClip_PlayFrom);
    ccAddExternalObjectFunction("AudioClip::PlayQueued^2",      Sc_AudioClip_PlayQueued);
    ccAddExternalObjectFunction("AudioClip::PlayOnChannel^3",   Sc_AudioClip_PlayOnChannel);
    ccAddExternalObjectFunction("AudioClip::Stop^0",            Sc_AudioClip_Stop);
    ccAddExternalObjectFunction("AudioClip::get_ID",            Sc_AudioClip_GetID);
    ccAddExternalObjectFunction("AudioClip::get_FileType",      Sc_AudioClip_GetFileType);
    ccAddExternalObjectFunction("AudioClip::get_IsAvailable",   Sc_AudioClip_GetIsAvailable);
    ccAddExternalObjectFunction("AudioClip::get_Type",          Sc_AudioClip_GetType);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("AudioClip::Play^2",            (void*)AudioClip_Play);
    ccAddExternalFunctionForPlugin("AudioClip::PlayFrom^3",        (void*)AudioClip_PlayFrom);
    ccAddExternalFunctionForPlugin("AudioClip::PlayQueued^2",      (void*)AudioClip_PlayQueued);
    ccAddExternalFunctionForPlugin("AudioClip::Stop^0",            (void*)AudioClip_Stop);
    ccAddExternalFunctionForPlugin("AudioClip::get_FileType",      (void*)AudioClip_GetFileType);
    ccAddExternalFunctionForPlugin("AudioClip::get_IsAvailable",   (void*)AudioClip_GetIsAvailable);
    ccAddExternalFunctionForPlugin("AudioClip::get_Type",          (void*)AudioClip_GetType);
}
