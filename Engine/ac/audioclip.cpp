//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "media/audio/audio_system.h"
#include "ac/audioclip.h"
#include "ac/audiochannel.h"
#include "ac/common.h" // quitprintf
#include "ac/gamesetupstruct.h"
#include "ac/string.h"
#include "ac/dynobj/cc_audioclip.h"
#include "ac/dynobj/cc_audiochannel.h"
#include "core/assetmanager.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern ScriptAudioChannel scrAudioChannel[MAX_GAME_CHANNELS];
extern CCAudioClip ccDynamicAudioClip;
extern CCAudioChannel ccDynamicAudio;

int AudioClip_GetID(ScriptAudioClip *clip)
{
    return clip->id;
}

const char *AudioClip_GetScriptName(ScriptAudioClip *clip)
{
    return CreateNewScriptString(clip->scriptName);
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
    return AssetMgr->DoesAssetExist(get_audio_clip_assetpath(clip->bundlingType, clip->fileName)) ? 1 : 0;
}

void AudioClip_Stop(ScriptAudioClip *clip)
{
    for (int i = NUM_SPEECH_CHANS; i < game.numGameChannels; i++)
    {
        auto* ch = AudioChans::GetChannelIfPlaying(i);
        if ((ch != nullptr) && (ch->sourceClipID == clip->id))
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
    if (chan < NUM_SPEECH_CHANS || chan >= game.numGameChannels)
        quitprintf("!AudioClip.PlayOnChannel: invalid channel %d, the range is %d - %d",
            chan, NUM_SPEECH_CHANS, game.numGameChannels - 1);
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

#include "ac/dynobj/scriptstring.h"
#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

ScriptAudioClip *AudioClip_GetByName(const char *name)
{
    return static_cast<ScriptAudioClip*>(ccGetScriptObjectAddress(name, ccDynamicAudioClip.GetType()));
}


RuntimeScriptValue Sc_AudioClip_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptAudioClip, ccDynamicAudioClip, AudioClip_GetByName, const char);
}

RuntimeScriptValue Sc_AudioClip_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetID);
}

RuntimeScriptValue Sc_AudioClip_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptAudioClip, const char, myScriptStringImpl, AudioClip_GetScriptName);
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
    ScFnRegister audioclip_api[] = {
        { "AudioClip::GetByName",         API_FN_PAIR(AudioClip_GetByName) },
        { "AudioClip::Play^2",            API_FN_PAIR(AudioClip_Play) },
        { "AudioClip::PlayFrom^3",        API_FN_PAIR(AudioClip_PlayFrom) },
        { "AudioClip::PlayQueued^2",      API_FN_PAIR(AudioClip_PlayQueued) },
        { "AudioClip::PlayOnChannel^3",   API_FN_PAIR(AudioClip_PlayOnChannel) },
        { "AudioClip::Stop^0",            API_FN_PAIR(AudioClip_Stop) },
        { "AudioClip::get_ID",            API_FN_PAIR(AudioClip_GetID) },
        { "AudioClip::get_FileType",      API_FN_PAIR(AudioClip_GetFileType) },
        { "AudioClip::get_IsAvailable",   API_FN_PAIR(AudioClip_GetIsAvailable) },
        { "AudioClip::get_ScriptName",    API_FN_PAIR(AudioClip_GetScriptName) },
        { "AudioClip::get_Type",          API_FN_PAIR(AudioClip_GetType) },
    };

    ccAddExternalFunctions(audioclip_api);
}
