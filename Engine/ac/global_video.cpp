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
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/global_video.h"
#include "debug/debugger.h"
#include "debug/debug_log.h"
#include "media/video/video.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void PlayFlic(int numb, int scr_flags)
{
    EndSkippingUntilCharStops();
    if (play.fast_forward)
        return;
    if (debug_flags & DBG_NOVIDEO)
        return;

    // AGS 2.x: If the screen is faded out, fade in again when playing a movie.
    if (loaded_game_file_version <= kGameVersion_272)
        play.screen_is_faded_out = 0;

    // Convert PlayFlic flags to common video flags
    /* NOTE: historically using decimal "flags"
    default (0): the video will be played stretched to screen;
        player cannot skip animation; screen will be cleared
    1: player can press ESC to skip animation
    2: player can press any key or click mouse to skip animation
    10: play the video at original size
    100: do not clear the screen before starting playback
    */
    // NOTE: don't enable frame drop with FLIC, or it will play too fast
    // (also see a note about kVideoState_SetGameFps below)
    int video_flags = kVideo_EnableVideo;
    int state_flags = 0;
    VideoSkipType skip = VideoSkipNone;
    // skip type
    switch (scr_flags % 10)
    {
    case 1: skip = VideoSkipEscape; break;
    case 2: skip = VideoSkipKeyOrMouse; break;
    default: skip = VideoSkipNone; break;
    }
    // video size
    switch ((scr_flags % 100) / 10)
    {
    case 1: /* play original size, no flag */ break;
    default: state_flags |= kVideoState_Stretch;
    }
    // clear screen
    switch ((scr_flags % 1000) / 100)
    {
    case 1: /* don't clear screen, no flag */ break;
    default: state_flags |= kVideoState_ClearScreen;
    }

    // NOTE: do not set kVideoState_SetGameFps for FLIC,
    // it seems like existing games featured FLICs with over-top FPS,
    // but limited it to ~60 FPS using vsync.

    HError err = play_flc_video(numb, video_flags, state_flags, skip);
    if (!err)
        debug_script_warn("Failed to play FLIC %d: %s", numb, err->FullMessage().GetCStr());
}

void PlayVideo(const char* name, int skip, int scr_flags)
{
    EndSkippingUntilCharStops();
    if (play.fast_forward)
        return;
    if (debug_flags & DBG_NOVIDEO)
        return;

    // Convert PlayVideo flags to common video flags
    /* NOTE: historically using decimal "flags"
    default (0): the video will be played at original size,
        video's own audio is playing, game sounds muted;
    1: the video will be stretched to full screen;
    10: keep game audio only, video's own audio muted;
    -- since 3.6.0:
    20: play both game audio and video's own audio
    */
    int video_flags = kVideo_EnableVideo | kVideo_DropFrames | kVideo_SyncAudioVideo;
    int state_flags = 0;
    // video size
    switch (scr_flags % 10)
    {
    case 1: state_flags |= kVideoState_Stretch; break;
    default: break;
    }
    // audio option
    switch ((scr_flags % 100) / 10)
    {
    case 1: break; // keep game audio, and no video audio
    case 2: video_flags |= kVideo_EnableAudio; break; // have both game and video audio
    default: // play video audio but stop game audio
        video_flags |= kVideo_EnableAudio;
        state_flags |= kVideoState_StopGameAudio;
        break;
    }

    // if audio is disabled, then don't play any sound on the video either
    if (!usetup.AudioEnabled)
        video_flags &= ~kVideo_EnableAudio;

    // for old versions: allow slightly offset video frames
    if (loaded_game_file_version < kGameVersion_360_16)
        video_flags |= kVideo_LegacyFrameSize;

    // original engine's behavior was to adjust FPS to match video's,
    // but we may rethink this later (or add an explicit setting)
    state_flags |= kVideoState_SetGameFps;

    // TODO: use extension as a format hint
    HError err = play_theora_video(name, video_flags, state_flags, static_cast<VideoSkipType>(skip));
    if (!err)
        debug_script_warn("Failed to play video '%s': %s", name, err->FullMessage().GetCStr());
}
