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
#include "media/audio/openalsource.h"
#include "util/geometry.h"
#include "util/string.h"
#include "util/error.h"

namespace AGS
{

namespace Common { class Bitmap; }

namespace Engine
{

enum VideoFlags
{
    kVideo_EnableVideo    = 0x0001,
    kVideo_Stretch        = 0x0002,
    kVideo_ClearScreen    = 0x0004,
    kVideo_LegacyFrameSize= 0x0008,
    kVideo_EnableAudio    = 0x0010,
    kVideo_KeepGameAudio  = 0x0020
};

enum VideoSkipType
{
    VideoSkipNone         = 0,
    VideoSkipEscape       = 1,
    VideoSkipAnyKey       = 2,
    VideoSkipKeyOrMouse   = 3
};

} // namespace Engine
} // namespace AGS


AGS::Common::HError play_theora_video(const char *name, int flags, AGS::Engine::VideoSkipType skip);
AGS::Common::HError play_flc_video(int numb, int flags, AGS::Engine::VideoSkipType skip);

// Pause the active video
void video_pause();
// Resume the active video
void video_resume();
// Stop current playback and dispose all video resource
void video_shutdown();

#endif // __AGS_EE_MEDIA__VIDEO_H
