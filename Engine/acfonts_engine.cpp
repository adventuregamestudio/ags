//
// Implementation from acfonts.cpp specific to Engine runtime
//

// Headers, as they are in acfonts.cpp
#pragma unmanaged
#ifndef USE_ALFONT
#define USE_ALFONT
#endif
#include "wgt2allg.h"
#include "acruntim.h"

#ifdef USE_ALFONT
#include "alfont.h"
#endif

// For engine these are defined in ac.cpp
extern int our_eip;

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
