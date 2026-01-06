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
//
// Game-blocking video interface.
//
//=============================================================================
#ifndef __AGS_EE_MEDIA__VIDEO_H
#define __AGS_EE_MEDIA__VIDEO_H

#include "media/video/videoplayer.h"
#include "util/geometry.h"
#include "util/string.h"
#include "util/error.h"

namespace AGS
{
namespace Engine
{

// Flags which define behavior of a blocking video "game state"
enum VideoStateFlags
{
    // Force-clear screen on begin and end (meant for software render)
    kVideoState_ClearScreen    = 0x0001,
    // Stretch video to the game screen
    kVideoState_Stretch        = 0x0002,
    // Stop game audio and restore after video finishes playing
    kVideoState_StopGameAudio  = 0x0004,
    // Increase game FPS to match video's
    kVideoState_SetGameFps     = 0x0008
};

// Flags which define a skipping method for the blocking video playback
enum VideoSkipType
{
    VideoSkipNone         = 0,
    VideoSkipEscape       = 1,
    VideoSkipAnyKey       = 2,
    VideoSkipKeyOrMouse   = 3
};

} // namespace Engine
} // namespace AGS


// Blocking video API
//
// Start a blocking OGV playback
AGS::Common::HError play_theora_video(const AGS::Common::String &name, int video_flags, int state_flags, AGS::Engine::VideoSkipType skip);
// Start a blocking FLIC playback
AGS::Common::HError play_flc_video(int numb, int video_flags, int state_flags, AGS::Engine::VideoSkipType skip);
// Pause the active blocking video
void video_single_pause();
// Resume the active blocking video
void video_single_resume();
// Stop current blocking video playback and dispose all video resource
void video_single_stop();

// Stop all videos and video thread
void video_shutdown();

#endif // __AGS_EE_MEDIA__VIDEO_H
