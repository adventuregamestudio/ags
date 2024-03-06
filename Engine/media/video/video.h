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
    // Keep game audio running while video is playing
    kVideoState_KeepGameAudio  = 0x0004
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


AGS::Common::HError play_theora_video(const char *name, int video_flags, int state_flags, AGS::Engine::VideoSkipType skip);
AGS::Common::HError play_flc_video(int numb, int video_flags, int state_flags, AGS::Engine::VideoSkipType skip);

// Pause the active video
void video_pause();
// Resume the active video
void video_resume();
// Stop current playback and dispose all video resource
void video_shutdown();

#endif // __AGS_EE_MEDIA__VIDEO_H
