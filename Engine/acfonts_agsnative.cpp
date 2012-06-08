//
// Implementation from acfonts.cpp specific to AGS.Native library
//

// Headers, as they are in acfonts.cpp
#pragma unmanaged
#define CROOM_NOFUNCTIONS
#ifndef USE_ALFONT
#define USE_ALFONT
#endif
#define USE_CLIB
#define WGT2ALLEGRO_NOFUNCTIONS
#include "wgt2allg.h"
#include "acroom.h"
#include "acruntim.h"

#ifdef USE_ALFONT
#include "alfont.h"
#endif

//=============================================================================
// Symbols, originally defined in acfonts.cpp for AGS.Native
//=============================================================================

extern GameSetupStruct thisgame;
void check_font(int *fontnum)
{
  // Stop roomedit crashing if they specify an invalid font number
  if (fontnum[0] >= thisgame.numfonts)
    fontnum[0] = 0;
}

//=============================================================================
// AGS.Native-specific implementations split out of acfonts.cpp
//=============================================================================

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
