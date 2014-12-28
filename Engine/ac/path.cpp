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
        ml->pos[i] = ((int)highPart << 16) | (lowPart & 0x0000ffff);

        ml->xpermove[i] *= current_screen_resolution_multiplier;
        ml->ypermove[i] *= current_screen_resolution_multiplier;
    }
}
