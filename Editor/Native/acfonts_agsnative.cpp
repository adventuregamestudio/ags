//
// Implementation from acfonts.cpp specific to AGS.Native library
//

#pragma unmanaged

#ifndef USE_ALFONT
#define USE_ALFONT
#endif
#include "util/wgt2allg.h"
#include "ac/gamesetupstruct.h"
#include "util/filestream.h"

using AGS::Common::CDataStream;

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

CDataStream *fopen_shared(char *filnamm, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
{
  return Common::File::OpenFile(filnamm, open_mode, work_mode);
}

int flength_shared(CDataStream *ffi)
{
  // in the editor, we don't read from clib, only from disk
  return ffi->GetLength();
}
