//
// Implementation from acfonts.cpp specific to Engine runtime
//

// Headers, as they are in acfonts.cpp
#pragma unmanaged
#ifndef USE_ALFONT
#define USE_ALFONT
#endif
#include "util/wgt2allg.h"
#include "ac/gamesetupstruct.h"

#ifdef USE_ALFONT
#include "alfont.h"
#endif

// For engine these are defined in ac.cpp
extern int our_eip;
extern GameSetupStruct game;

// For engine these are defined in clib32.cpp
extern "C"
{
  extern FILE *clibfopen(char *, char *);
  extern long last_opened_size;
}
//

//=============================================================================
// Engine-specific implementation split out of acfonts.cpp
//=============================================================================

void set_our_eip(int eip)
{
  our_eip = eip;
}

int get_our_eip()
{
  return our_eip;
}

FILE *fopen_shared(char *filnamm, char *fmt)
{
  return clibfopen(filnamm, fmt);
}

int flength_shared(FILE *ffi)
{
  // clibfopen will have set last_opened_size
  return last_opened_size;
}

void set_font_outline(int font_number, int outline_type)
{
    game.fontoutline[font_number] = FONT_OUTLINE_AUTO;
}
