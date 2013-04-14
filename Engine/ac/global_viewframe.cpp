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

#include "ac/global_viewframe.h"
#include "ac/common.h"
#include "ac/view.h"
#include "debug/debug_log.h"
#include "game/game_objects.h"
#include "media/audio/audio.h"

extern ViewStruct*views;
extern int psp_is_old_datafile;


void SetFrameSound (int vii, int loop, int frame, int sound) {
    if ((vii < 1) || (vii > game.ViewCount))
        quit("!SetFrameSound: invalid view number");
    vii--;

    if (loop >= views[vii].numLoops)
        quit("!SetFrameSound: invalid loop number");

    if (frame >= views[vii].loops[loop].numFrames)
        quit("!SetFrameSound: invalid frame number");

    if (sound < 1)
    {
        views[vii].loops[loop].frames[frame].sound = -1;
    }
    else
    {
        ScriptAudioClip* clip = get_audio_clip_for_old_style_number(false, sound);
        if (clip == NULL)
            quitprintf("!SetFrameSound: audio clip aSound%d not found", sound);

        views[vii].loops[loop].frames[frame].sound = clip->id + (psp_is_old_datafile ? 0x10000000 : 0);
    }
}
