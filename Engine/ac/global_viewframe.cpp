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
#include "ac/global_viewframe.h"
#include "ac/common.h"
#include "ac/game.h"
#include "ac/view.h"
#include "ac/gamesetupstruct.h"
#include "debug/debug_log.h"
#include "media/audio/audio_system.h"

extern GameSetupStruct game;
extern std::vector<ViewStruct> views;


void SetFrameSound (int vii, int loop, int frame, int sound) {
    vii--; // convert to 0-based
    AssertFrame("SetFrameSound", vii, loop, frame);

    if (sound < 1)
    {
        views[vii].loops[loop].frames[frame].sound = -1;
        views[vii].loops[loop].frames[frame].audioclip = -1;
    }
    else
    {
        ScriptAudioClip* clip = GetAudioClipForOldStyleNumber(game, false, sound);
        if (clip == nullptr)
            quitprintf("!SetFrameSound: audio clip aSound%d not found", sound);

        views[vii].loops[loop].frames[frame].sound = 
            game.IsLegacyAudioSystem() ? sound : clip->id;
        views[vii].loops[loop].frames[frame].audioclip = clip->id;
    }
}
