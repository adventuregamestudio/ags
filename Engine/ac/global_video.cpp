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
    /* NOTE: historically using decimal "flags"
    default (0): the video will be played stretched to screen;
        player cannot skip animation; screen will be cleared
    1: player can press ESC to skip animation
    2: player can press any key or click mouse to skip animation
    10: play the video at original size
    100: do not clear the screen before starting playback
    */
    int flags = kVideo_EnableVideo;
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
    default: flags |= kVideo_Stretch;
    }
    // clear screen
    switch ((scr_flags % 1000) / 100)
    {
    case 1: /* don't clear screen, no flag */ break;
    default: flags |= kVideo_ClearScreen;
    }

    play_flc_video(numb, flags, skip);
}

void PlayVideo(const char* name, int skip, int scr_flags) {
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
    int flags = kVideo_EnableVideo;
    // video size
    switch (scr_flags % 10)
    {
    case 1: flags |= kVideo_Stretch; break;
    default: break;
    }
    // audio option
    switch ((scr_flags % 100) / 10)
    {
    case 1: flags |= kVideo_KeepGameAudio; break;
    case 2: flags |= kVideo_EnableAudio | kVideo_KeepGameAudio; break;
    default: flags |= kVideo_EnableAudio; break;
    }

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
    // Save the game audio parameters, in case we stop these
    int musplaying = play.cur_music_number, i;
    int ambientWas[MAX_GAME_CHANNELS]{0};
    for (i = NUM_SPEECH_CHANS; i < game.numGameChannels; i++)
        ambientWas[i] = ambient[i].channel;

    // Optionally stop the game audio
    if ((flags & kVideo_KeepGameAudio) == 0)
    {
        stop_all_sound_and_music();
    }

    if ((strlen(name) > 3) && (ags_stricmp(&name[strlen(name) - 3], "ogv") == 0))
    {
        play_theora_video(name, flags, skip);
    }
    else
    {
        debug_script_warn("PlayVideo: file '%s' is an unsupported format.", name);
        return;
    }

    // Restore the game audio if we stopped them before the video playback
    if ((flags & kVideo_KeepGameAudio) == 0)
    {
        update_music_volume();
        if (musplaying >= 0)
            newmusic(musplaying);
        for (i = NUM_SPEECH_CHANS; i < game.numGameChannels; i++) {
            if (ambientWas[i] > 0)
                PlayAmbientSound(ambientWas[i], ambient[i].num, ambient[i].vol, ambient[i].x, ambient[i].y);
        }
    }
}

#else

void pause_sound_if_necessary_and_play_video(const char *name, int flags, VideoSkipType skip) {}

#endif