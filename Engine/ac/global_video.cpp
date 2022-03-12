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
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/global_game.h"
#include "ac/global_video.h"
#include "ac/path_helper.h"
#include "core/assetmanager.h"
#include "debug/debugger.h"
#include "debug/debug_log.h"
#include "media/video/video.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;

void pause_sound_if_necessary_and_play_video(const char *name, int flags, VideoSkipType skip);

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
    /*
    0  player can't skip animation
    1  player can press ESC to skip animation
    2  player can press any key or click mouse to skip animation
    +10 (i.e.10,11,12) do not stretch to full-screen, just play at flc size
    +100 do not clear the screen before starting playback
    */
    int flags = kVideo_EnableVideo;
    VideoSkipType skip = VideoSkipNone;
    switch (scr_flags % 10)
    {
    case 1: skip = VideoSkipEscape; break;
    case 2: skip = VideoSkipKeyOrMouse; break;
    default: break;
    }
    if ((scr_flags % 100) < 10)
        flags |= kVideo_Stretch;
    if (scr_flags < 100)
        flags |= kVideo_ClearScreen;

    play_flc_video(numb, flags, skip);
}

void PlayVideo(const char* name, int skip, int scr_flags) {
    EndSkippingUntilCharStops();
    if (play.fast_forward)
        return;
    if (debug_flags & DBG_NOVIDEO)
        return;

    // Convert PlayVideo flags to common video flags
    /*
    0: the video will be played at original size, with AVI audio
    1: the video will be stretched to full screen, with appropriate
       black borders to maintain its aspect ratio and AVI audio.
    10: original size, with game audio continuing (AVI audio muted)
    11: stretched to full screen, with game audio continuing (AVI audio muted)
    */
    int flags = kVideo_EnableVideo;
    if ((scr_flags % 10) == 1)
        flags |= kVideo_Stretch;
    if (scr_flags < 10)
        flags |= kVideo_EnableAudio;

    // if game audio is disabled, then don't play any sound on the video either
    if (!usetup.audio_enabled)
        flags &= ~kVideo_EnableAudio;

    if (loaded_game_file_version < kGameVersion_360_16)
        flags |= kVideo_LegacyFrameSize;

    pause_sound_if_necessary_and_play_video(name, flags, static_cast<VideoSkipType>(skip));
}


#ifndef AGS_NO_VIDEO_PLAYER

void pause_sound_if_necessary_and_play_video(const char *name, int flags, VideoSkipType skip)
{
    int musplaying = play.cur_music_number, i;
    int ambientWas[MAX_GAME_CHANNELS]{0};
    for (i = NUM_SPEECH_CHANS; i < game.numGameChannels; i++)
        ambientWas[i] = ambient[i].channel;

    if ((strlen(name) > 3) && (ags_stricmp(&name[strlen(name) - 3], "ogv") == 0))
    {
        play_theora_video(name, flags, skip);
    }
    else
    {
        debug_script_warn("PlayVideo: file '%s' is an unsupported format.", name);
        return;
    }

    if ((flags & kVideo_EnableAudio) != 0)
    {
        update_music_volume();
        // restart the music
        if (musplaying >= 0)
            newmusic (musplaying);
        for (i = NUM_SPEECH_CHANS; i < game.numGameChannels; i++) {
            if (ambientWas[i] > 0)
                PlayAmbientSound(ambientWas[i], ambient[i].num, ambient[i].vol, ambient[i].x, ambient[i].y);
        }
    }
}

#else

void pause_sound_if_necessary_and_play_video(const char *name, int flags, VideoSkipType skip) {}

#endif