
#include "util/wgt2allg.h"
#include "path.h"

extern int current_screen_resolution_multiplier;

void convert_move_path_to_high_res(MoveList *ml)
{
    ml->fromx *= current_screen_resolution_multiplier;
    ml->fromy *= current_screen_resolution_multiplier;
    ml->lastx *= current_screen_resolution_multiplier;
    ml->lasty *= current_screen_resolution_multiplier;

    for (int i = 0; i < ml->numstage; i++)
    {
        short lowPart = (ml->pos[i] & 0x0000ffff) * current_screen_resolution_multiplier;
        short highPart = ((ml->pos[i] >> 16) & 0x0000ffff) * current_screen_resolution_multiplier;
        ml->pos[i] = (highPart << 16) | lowPart;

        ml->xpermove[i] *= current_screen_resolution_multiplier;
        ml->ypermove[i] *= current_screen_resolution_multiplier;
    }
}
