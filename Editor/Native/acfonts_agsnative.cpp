//
// Implementation from acfonts.cpp specific to AGS.Native library
//

#pragma unmanaged

#ifndef USE_ALFONT
#define USE_ALFONT
#endif
#include "util/wgt2allg.h"
#include "ac/gamesetupstruct.h"

//=============================================================================
// AGS.Native-specific implementation split out of acfonts.cpp
//=============================================================================

extern GameSetupStruct thisgame;

void check_font(int *fontnum)
{
    // Stop roomedit crashing if they specify an invalid font number
    if (fontnum[0] >= thisgame.numfonts)
        fontnum[0] = 0;
}

void set_our_eip(int eip)
{
    // do nothing
}

int get_our_eip()
{
    return 0;
}
