//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/properties.h"
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
    return play_audio_clip_on_channel(clip, chan, priority, repeat);
}

ScriptAudioChannel *AudioClip_PlayAsType(ScriptAudioClip *clip, int type, int chan, int priority, int repeat)
{
    if (type <= AUDIOTYPE_SPEECH || static_cast<uint32_t>(type) >= game.audioClipTypes.size())
        quitprintf("!AudioClip.PlayAsType: invalid audio type %d, the range is %u - %u",
            type, AUDIOTYPE_SPEECH + 1, static_cast<uint32_t>(game.audioClipTypes.size() - 1));
    if (chan != SCR_NO_VALUE && (chan < NUM_SPEECH_CHANS || chan >= game.numGameChannels))
        quitprintf("!AudioClip.PlayAsType: invalid channel %d, the range is %d - %d",
            chan, NUM_SPEECH_CHANS, game.numGameChannels - 1);
    if (chan == SCR_NO_VALUE)
        chan = -1;
    if (priority == SCR_NO_VALUE)
        priority = clip->defaultPriority;
    if (repeat == SCR_NO_VALUE)
        repeat = clip->defaultRepeat;
    return play_audio_clip(AudioPlayback(clip, type), chan, priority, repeat);
}

int AudioClip_GetProperty(ScriptAudioClip *clip, const char *property)
{
    return get_int_property(game.audioclipProps[clip->id], play.audioclipProps[clip->id], property);
}

const char* AudioClip_GetTextProperty(ScriptAudioClip *clip, const char *property)
{
    return get_text_property_dynamic_string(game.audioclipProps[clip->id], play.audioclipProps[clip->id], property);
}

bool AudioClip_SetProperty(ScriptAudioClip *clip, const char *property, int value)
{
    return set_int_property(play.audioclipProps[clip->id], property, value);
}

bool AudioClip_SetTextProperty(ScriptAudioClip *clip, const char *property, const char *value)
{
    return set_text_property(play.audioclipProps[clip->id], property, value);
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

RuntimeScriptValue Sc_AudioClip_PlayAsType(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT4(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_PlayAsType);
}

RuntimeScriptValue Sc_AudioClip_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptAudioClip, AudioClip_GetProperty, const char);
}

RuntimeScriptValue Sc_AudioClip_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptAudioClip, const char, myScriptStringImpl, AudioClip_GetTextProperty, const char);
}

RuntimeScriptValue Sc_AudioClip_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptAudioClip, AudioClip_SetProperty, const char);
}

RuntimeScriptValue Sc_AudioClip_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptAudioClip, AudioClip_SetTextProperty, const char, const char);
}

void RegisterAudioClipAPI()
{
    ScFnRegister audioclip_api[] = {
        { "AudioClip::GetByName",         API_FN_PAIR(AudioClip_GetByName) },
        { "AudioClip::Play^2",            API_FN_PAIR(AudioClip_Play) },
        { "AudioClip::PlayFrom^3",        API_FN_PAIR(AudioClip_PlayFrom) },
        { "AudioClip::PlayQueued^2",      API_FN_PAIR(AudioClip_PlayQueued) },
        { "AudioClip::PlayOnChannel^3",   API_FN_PAIR(AudioClip_PlayOnChannel) },
        { "AudioClip::PlayAsType^4",      API_FN_PAIR(AudioClip_PlayAsType) },
        { "AudioClip::Stop^0",            API_FN_PAIR(AudioClip_Stop) },
        { "AudioClip::GetProperty^1",     API_FN_PAIR(AudioClip_GetProperty) },
        { "AudioClip::GetTextProperty^1", API_FN_PAIR(AudioClip_GetTextProperty) },
        { "AudioClip::SetProperty^2",     API_FN_PAIR(AudioClip_SetProperty) },
        { "AudioClip::SetTextProperty^2", API_FN_PAIR(AudioClip_SetTextProperty) },
        { "AudioClip::get_ID",            API_FN_PAIR(AudioClip_GetID) },
        { "AudioClip::get_FileType",      API_FN_PAIR(AudioClip_GetFileType) },
        { "AudioClip::get_IsAvailable",   API_FN_PAIR(AudioClip_GetIsAvailable) },
        { "AudioClip::get_ScriptName",    API_FN_PAIR(AudioClip_GetScriptName) },
        { "AudioClip::get_Type",          API_FN_PAIR(AudioClip_GetType) },
    };

    ccAddExternalFunctions(audioclip_api);
}
