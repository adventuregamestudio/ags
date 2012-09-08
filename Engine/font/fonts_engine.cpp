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

#include "util/clib32.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

// For engine these are defined in ac.cpp
extern int our_eip;
extern GameSetupStruct game;

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

CDataStream *fopen_shared(char *filnamm,
                          Common::FileOpenMode open_mode,
                          Common::FileWorkMode work_mode)
{
  return clibfopen(filnamm, open_mode, work_mode);
}

int flength_shared(CDataStream *ffi)
{
  // clibfopen will have set last_opened_size
  return last_opened_size;
}

void set_font_outline(int font_number, int outline_type)
{
    game.fontoutline[font_number] = FONT_OUTLINE_AUTO;
}
