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

#include "ac/audiochannel.h"
#include "ac/roomstruct.h"
#include "debug/debug_log.h"
#include "ac/gamestate.h"
#include "media/audio/audio.h"
#include "media/audio/soundclip.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_audioclip.h"

extern GameState play;
extern roomstruct thisroom;
extern CCAudioClip ccDynamicAudioClip;

int AudioChannel_GetID(ScriptAudioChannel *channel)
{
    return channel->id;
}

int AudioChannel_GetIsPlaying(ScriptAudioChannel *channel)
{
    if (play.fast_forward)
    {
        return 0;
    }

    if ((channels[channel->id] != NULL) &&
        (channels[channel->id]->done == 0))
    {
        return 1;
    }
    return 0;
}

int AudioChannel_GetPanning(ScriptAudioChannel *channel)
{
    if ((channels[channel->id] != NULL) &&
        (channels[channel->id]->done == 0))
    {
        return channels[channel->id]->panningAsPercentage;
    }
    return 0;
}

void AudioChannel_SetPanning(ScriptAudioChannel *channel, int newPanning)
{
    if ((newPanning < -100) || (newPanning > 100))
        quitprintf("!AudioChannel.Panning: panning value must be between -100 and 100 (passed=%d)", newPanning);

    if ((channels[channel->id] != NULL) &&
        (channels[channel->id]->done == 0))
    {
        channels[channel->id]->set_panning(((newPanning + 100) * 255) / 200);
        channels[channel->id]->panningAsPercentage = newPanning;
    }
}

ScriptAudioClip* AudioChannel_GetPlayingClip(ScriptAudioChannel *channel)
{
    if ((channels[channel->id] != NULL) &&
        (channels[channel->id]->done == 0))
    {
        return (ScriptAudioClip*)channels[channel->id]->sourceClip;
    }
    return NULL;
}

int AudioChannel_GetPosition(ScriptAudioChannel *channel)
{
    if ((channels[channel->id] != NULL) &&
        (channels[channel->id]->done == 0))
    {
        if (play.fast_forward)
            return 999999999;

        return channels[channel->id]->get_pos();
    }
    return 0;
}

int AudioChannel_GetPositionMs(ScriptAudioChannel *channel)
{
    if ((channels[channel->id] != NULL) &&
        (channels[channel->id]->done == 0))
    {
        if (play.fast_forward)
            return 999999999;

        return channels[channel->id]->get_pos_ms();
    }
    return 0;
}

int AudioChannel_GetLengthMs(ScriptAudioChannel *channel)
{
    if ((channels[channel->id] != NULL) &&
        (channels[channel->id]->done == 0))
    {
        return channels[channel->id]->get_length_ms();
    }
    return 0;
}

int AudioChannel_GetVolume(ScriptAudioChannel *channel)
{
    if ((channels[channel->id] != NULL) &&
        (channels[channel->id]->done == 0))
    {
        return channels[channel->id]->volAsPercentage;
    }
    return 0;
}

int AudioChannel_SetVolume(ScriptAudioChannel *channel, int newVolume)
{
    if ((newVolume < 0) || (newVolume > 100))
        quitprintf("!AudioChannel.Volume: new value out of range (supplied: %d, range: 0..100)", newVolume);

    if ((channels[channel->id] != NULL) &&
        (channels[channel->id]->done == 0))
    {
        channels[channel->id]->set_volume((newVolume * 255) / 100);
        channels[channel->id]->volAsPercentage = newVolume;
    }
    return 0;
}


void AudioChannel_Stop(ScriptAudioChannel *channel)
{
    stop_or_fade_out_channel(channel->id, -1, NULL);
}

void AudioChannel_Seek(ScriptAudioChannel *channel, int newPosition)
{
    if (newPosition < 0)
        quitprintf("!AudioChannel.Seek: invalid seek position %d", newPosition);

    if ((channels[channel->id] != NULL) &&
        (channels[channel->id]->done == 0))
    {
        channels[channel->id]->seek(newPosition);
    }
}

void AudioChannel_SetRoomLocation(ScriptAudioChannel *channel, int xPos, int yPos)
{
    if ((channels[channel->id] != NULL) &&
        (channels[channel->id]->done == 0))
    {
        int maxDist = ((xPos > thisroom.width / 2) ? xPos : (thisroom.width - xPos)) - AMBIENCE_FULL_DIST;
        channels[channel->id]->xSource = (xPos > 0) ? xPos : -1;
        channels[channel->id]->ySource = yPos;
        channels[channel->id]->maximumPossibleDistanceAway = maxDist;
        if (xPos > 0)
        {
            update_directional_sound_vol();
        }
        else
        {
            channels[channel->id]->directionalVolModifier = 0;
            channels[channel->id]->set_volume(channels[channel->id]->vol);
        }
    }
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetID);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetIsPlaying(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetIsPlaying);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetPanning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetPanning);
}

// void | ScriptAudioChannel *channel, int newPanning
RuntimeScriptValue Sc_AudioChannel_SetPanning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioChannel, AudioChannel_SetPanning);
}

// ScriptAudioClip* | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetPlayingClip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptAudioChannel, ScriptAudioClip, ccDynamicAudioClip, AudioChannel_GetPlayingClip);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetPosition);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetPositionMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetPositionMs);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetLengthMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetLengthMs);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetVolume);
}

// int | ScriptAudioChannel *channel, int newVolume
RuntimeScriptValue Sc_AudioChannel_SetVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptAudioChannel, AudioChannel_SetVolume);
}

// void | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_Stop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioChannel, AudioChannel_Stop);
}

// void | ScriptAudioChannel *channel, int newPosition
RuntimeScriptValue Sc_AudioChannel_Seek(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioChannel, AudioChannel_Seek);
}

// void | ScriptAudioChannel *channel, int xPos, int yPos
RuntimeScriptValue Sc_AudioChannel_SetRoomLocation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptAudioChannel, AudioChannel_SetRoomLocation);
}

void RegisterAudioChannelAPI()
{
    ccAddExternalObjectFunction("AudioChannel::Seek^1",             Sc_AudioChannel_Seek);
    ccAddExternalObjectFunction("AudioChannel::SetRoomLocation^2",  Sc_AudioChannel_SetRoomLocation);
    ccAddExternalObjectFunction("AudioChannel::Stop^0",             Sc_AudioChannel_Stop);
    ccAddExternalObjectFunction("AudioChannel::get_ID",             Sc_AudioChannel_GetID);
    ccAddExternalObjectFunction("AudioChannel::get_IsPlaying",      Sc_AudioChannel_GetIsPlaying);
    ccAddExternalObjectFunction("AudioChannel::get_LengthMs",       Sc_AudioChannel_GetLengthMs);
    ccAddExternalObjectFunction("AudioChannel::get_Panning",        Sc_AudioChannel_GetPanning);
    ccAddExternalObjectFunction("AudioChannel::set_Panning",        Sc_AudioChannel_SetPanning);
    ccAddExternalObjectFunction("AudioChannel::get_PlayingClip",    Sc_AudioChannel_GetPlayingClip);
    ccAddExternalObjectFunction("AudioChannel::get_Position",       Sc_AudioChannel_GetPosition);
    ccAddExternalObjectFunction("AudioChannel::get_PositionMs",     Sc_AudioChannel_GetPositionMs);
    ccAddExternalObjectFunction("AudioChannel::get_Volume",         Sc_AudioChannel_GetVolume);
    ccAddExternalObjectFunction("AudioChannel::set_Volume",         Sc_AudioChannel_SetVolume);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("AudioChannel::Seek^1",             (void*)AudioChannel_Seek);
    ccAddExternalFunctionForPlugin("AudioChannel::SetRoomLocation^2",  (void*)AudioChannel_SetRoomLocation);
    ccAddExternalFunctionForPlugin("AudioChannel::Stop^0",             (void*)AudioChannel_Stop);
    ccAddExternalFunctionForPlugin("AudioChannel::get_ID",             (void*)AudioChannel_GetID);
    ccAddExternalFunctionForPlugin("AudioChannel::get_IsPlaying",      (void*)AudioChannel_GetIsPlaying);
    ccAddExternalFunctionForPlugin("AudioChannel::get_LengthMs",       (void*)AudioChannel_GetLengthMs);
    ccAddExternalFunctionForPlugin("AudioChannel::get_Panning",        (void*)AudioChannel_GetPanning);
    ccAddExternalFunctionForPlugin("AudioChannel::set_Panning",        (void*)AudioChannel_SetPanning);
    ccAddExternalFunctionForPlugin("AudioChannel::get_PlayingClip",    (void*)AudioChannel_GetPlayingClip);
    ccAddExternalFunctionForPlugin("AudioChannel::get_Position",       (void*)AudioChannel_GetPosition);
    ccAddExternalFunctionForPlugin("AudioChannel::get_PositionMs",     (void*)AudioChannel_GetPositionMs);
    ccAddExternalFunctionForPlugin("AudioChannel::get_Volume",         (void*)AudioChannel_GetVolume);
    ccAddExternalFunctionForPlugin("AudioChannel::set_Volume",         (void*)AudioChannel_SetVolume);
}
