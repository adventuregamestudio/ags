
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__CHARACTEREXTRAS_H
#define __AGS_EE_AC__CHARACTEREXTRAS_H

#include "ac/runtime_defines.h"
#include "util/file.h"

struct CharacterExtras {
    // UGLY UGLY UGLY!! The CharacterInfo struct size is fixed because it's
    // used in the scripts, therefore overflowing stuff has to go here
    short invorder[MAX_INVORDER];
    short invorder_count;
    short width,height;
    short zoom;
    short xwas, ywas;
    short tint_r, tint_g;
    short tint_b, tint_level;
    short tint_light;
    char  process_idle_this_time;
    char  slow_move_counter;
    short animwait;

    void ReadFromFile(FILE *f);
    void WriteToFile(FILE *f);
};

#endif // __AGS_EE_AC__CHARACTEREXTRAS_H
