//
// Implementation from acfonts.cpp specific to AGS.Native library
//

#pragma unmanaged

#include "util/wgt2allg.h"
#include "ac/gamesetupstruct.h"

//=============================================================================
// AGS.Native-specific implementation split out of acfonts.cpp
//=============================================================================

extern GameSetupStruct thisgame;
extern int antiAliasFonts;

void set_our_eip(int eip)
{
  // do nothing
}

int get_our_eip()
{
  return 0;
}

bool ShouldAntiAliasText()
{
    return (antiAliasFonts != 0);
}
