//
// Implementation from acfonts.cpp specific to AGS.Native library
//

// Headers, as they are in acfonts.cpp
#pragma unmanaged
#ifndef USE_ALFONT
#define USE_ALFONT
#endif
//#include "wgt2allg_nofunc.h"
#include "wgt2allg.h"
//#include "acroom_nofunc.h"
#include "acruntim.h"

#ifdef USE_ALFONT
#include "alfont.h"
#endif

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

FILE *fopen_shared(char *filnamm, char *fmt)
{
  return fopen(filnamm, fmt);
}

int flength_shared(FILE *ffi)
{
  // in the editor, we don't read from clib, only from disk
  return filelength(fileno(ffi));
}
