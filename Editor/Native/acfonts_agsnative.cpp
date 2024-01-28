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


bool ShouldAntiAliasText()
{
    return (antiAliasFonts != 0);
}
