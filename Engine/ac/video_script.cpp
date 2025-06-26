//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// VideoPlayer script API.
//
//=============================================================================
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/scriptvideoplayer.h"
#include "debug/debug_log.h"
#include "media/video/video.h"
#include "media/video/videoplayer.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

ScriptVideoPlayer *VideoPlayer_Open(const char *filename, bool auto_play, int repeat_style)
{
    int video_id;
    HError err = open_video(filename,
        kVideo_EnableVideo | kVideo_EnableAudio | kVideo_DropFrames, video_id);
    if (!err)
    {
        debug_script_warn("Failed to play video '%s': %s", filename, err->FullMessage().GetCStr());
        return nullptr;
    }

    VideoControl *video_ctrl = get_video_control(video_id);
    video_ctrl->SetLooping((repeat_style) ? true : false);
    if (auto_play)
    {
        video_ctrl->Play();
    }

    ScriptVideoPlayer *sc_video = new ScriptVideoPlayer(video_id);
    int handle = ccRegisterManagedObject(sc_video, sc_video);
    video_ctrl->SetScriptHandle(handle);
    return sc_video;
}

void VideoPlayer_Play(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    video_ctrl->Play();
}

void VideoPlayer_Pause(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    video_ctrl->Pause();
}

void VideoPlayer_NextFrame(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    video_ctrl->NextFrame();
}

int VideoPlayer_SeekFrame(ScriptVideoPlayer *sc_video, int frame)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->SeekFrame(frame);
}

int VideoPlayer_SeekMs(ScriptVideoPlayer *sc_video, int pos)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->SeekMs(pos);
}

void VideoPlayer_Stop(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return;
    video_stop(sc_video->GetID());
    sc_video->Invalidate();
}

int VideoPlayer_GetFrame(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetFrame();
}

int VideoPlayer_GetFrameCount(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetFrameCount();
}

int VideoPlayer_GetFrameHeight(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetFrameSize().Height;
}

float VideoPlayer_GetFrameRate(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0.f;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetFrameRate();
}

int VideoPlayer_GetFrameWidth(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetFrameSize().Width;
}

int VideoPlayer_GetGraphic(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetSpriteID();
}

int VideoPlayer_GetLengthMs(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetDurationMs();
}

int VideoPlayer_GetLooping(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return false;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetLooping();
}

void VideoPlayer_SetLooping(ScriptVideoPlayer *sc_video, bool loop)
{
    if (sc_video->GetID() < 0)
        return;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->SetLooping(loop);
}

int VideoPlayer_GetPositionMs(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetPositionMs();
}

float VideoPlayer_GetSpeed(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0.f;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetSpeed();
}

void VideoPlayer_SetSpeed(ScriptVideoPlayer *sc_video, float speed)
{
    if (sc_video->GetID() < 0)
        return;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    video_ctrl->SetSpeed(speed);
}

int VideoPlayer_GetState(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return PlaybackState::PlayStateInvalid;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetState();
}

int VideoPlayer_GetVolume(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetVolume();
}

void VideoPlayer_SetVolume(ScriptVideoPlayer *sc_video, int volume)
{
    if (sc_video->GetID() < 0)
        return;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    video_ctrl->SetVolume(volume);
}

//=============================================================================

RuntimeScriptValue Sc_VideoPlayer_Open(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ_PINT2(ScriptVideoPlayer, VideoPlayer_Open, const char);
}

RuntimeScriptValue Sc_VideoPlayer_Play(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptVideoPlayer, VideoPlayer_Play);
}

RuntimeScriptValue Sc_VideoPlayer_Pause(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptVideoPlayer, VideoPlayer_Pause);
}

RuntimeScriptValue Sc_VideoPlayer_NextFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptVideoPlayer, VideoPlayer_NextFrame);
}

RuntimeScriptValue Sc_VideoPlayer_SeekFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptVideoPlayer, VideoPlayer_SeekFrame);
}

RuntimeScriptValue Sc_VideoPlayer_SeekMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptVideoPlayer, VideoPlayer_SeekMs);
}

RuntimeScriptValue Sc_VideoPlayer_Stop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptVideoPlayer, VideoPlayer_Stop);
}

RuntimeScriptValue Sc_VideoPlayer_GetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetFrame);
}

RuntimeScriptValue Sc_VideoPlayer_GetFrameCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetFrameCount);
}

RuntimeScriptValue Sc_VideoPlayer_GetFrameHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetFrameHeight);
}

RuntimeScriptValue Sc_VideoPlayer_GetFrameRate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptVideoPlayer, VideoPlayer_GetFrameRate);
}

RuntimeScriptValue Sc_VideoPlayer_GetFrameWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetFrameWidth);
}

RuntimeScriptValue Sc_VideoPlayer_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetGraphic);
}

RuntimeScriptValue Sc_VideoPlayer_GetLengthMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetLengthMs);
}

RuntimeScriptValue Sc_VideoPlayer_GetLooping(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetLooping);
}

RuntimeScriptValue Sc_VideoPlayer_SetLooping(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptVideoPlayer, VideoPlayer_SetLooping);
}

RuntimeScriptValue Sc_VideoPlayer_GetPositionMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetPositionMs);
}

RuntimeScriptValue Sc_VideoPlayer_GetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptVideoPlayer, VideoPlayer_GetSpeed);
}

RuntimeScriptValue Sc_VideoPlayer_SetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptVideoPlayer, VideoPlayer_SetSpeed);
}

RuntimeScriptValue Sc_VideoPlayer_GetState(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetState);
}

RuntimeScriptValue Sc_VideoPlayer_GetVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetVolume);
}

RuntimeScriptValue Sc_VideoPlayer_SetVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptVideoPlayer, VideoPlayer_SetVolume);
}



void RegisterVideoAPI()
{
    ScFnRegister video_api[] = {
        { "VideoPlayer::Open",          API_FN_PAIR(VideoPlayer_Open) },
        { "VideoPlayer::Play",          API_FN_PAIR(VideoPlayer_Play) },
        { "VideoPlayer::Pause",         API_FN_PAIR(VideoPlayer_Pause) },
        { "VideoPlayer::NextFrame",     API_FN_PAIR(VideoPlayer_NextFrame) },
        { "VideoPlayer::SeekFrame",     API_FN_PAIR(VideoPlayer_SeekFrame) },
        { "VideoPlayer::SeekMs",        API_FN_PAIR(VideoPlayer_SeekMs) },
        { "VideoPlayer::Stop",          API_FN_PAIR(VideoPlayer_Stop) },

        { "VideoPlayer::get_Frame",     API_FN_PAIR(VideoPlayer_GetFrame) },
        { "VideoPlayer::get_FrameCount", API_FN_PAIR(VideoPlayer_GetFrameCount) },
        { "VideoPlayer::get_FrameHeight", API_FN_PAIR(VideoPlayer_GetFrameHeight) },
        { "VideoPlayer::get_FrameRate", API_FN_PAIR(VideoPlayer_GetFrameRate) },
        { "VideoPlayer::get_FrameWidth", API_FN_PAIR(VideoPlayer_GetFrameWidth) },
        { "VideoPlayer::get_Graphic",   API_FN_PAIR(VideoPlayer_GetGraphic) },
        { "VideoPlayer::get_LengthMs",  API_FN_PAIR(VideoPlayer_GetLengthMs) },
        { "VideoPlayer::get_Looping",   API_FN_PAIR(VideoPlayer_GetLooping) },
        { "VideoPlayer::set_Looping",   API_FN_PAIR(VideoPlayer_SetLooping) },
        { "VideoPlayer::get_PositionMs", API_FN_PAIR(VideoPlayer_GetPositionMs) },
        { "VideoPlayer::get_Speed",     API_FN_PAIR(VideoPlayer_GetSpeed) },
        { "VideoPlayer::set_Speed",     API_FN_PAIR(VideoPlayer_SetSpeed) },
        { "VideoPlayer::get_State",     API_FN_PAIR(VideoPlayer_GetState) },
        { "VideoPlayer::get_Volume",    API_FN_PAIR(VideoPlayer_GetVolume) },
        { "VideoPlayer::set_Volume",    API_FN_PAIR(VideoPlayer_SetVolume) }
    };

    ccAddExternalFunctions(video_api);
}
