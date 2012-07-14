
#include "ac/global_viewframe.h"
#include "wgt2allg.h"
#include "ac/common.h"
#include "ac/view.h"
#include "ac/gamesetupstruct.h"
#include "debug/debug.h"
#include "media/audio/audio.h"

extern GameSetupStruct game;
extern ViewStruct*views;
extern int psp_is_old_datafile;


void SetFrameSound (int vii, int loop, int frame, int sound) {
    if ((vii < 1) || (vii > game.numviews))
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
