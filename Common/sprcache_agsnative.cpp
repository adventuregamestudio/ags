//
// Implementation from sprcache.cpp specific to AGS.Native library
//

// Headers, as they are in sprcache.cpp
#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#pragma warning (disable: 4996 4312)  // disable deprecation warnings
#endif

#define WGT2ALLEGRO_NOFUNCTIONS
#define CROOM_NOFUNCTIONS
#include "wgt2allg.h"
#include "acroom.h"
#include "sprcache.h"
#include "compress.h"
#include "bigend.h"
//

//=============================================================================
// Symbols, originally defined in sprcache.cpp for AGS.Native
//=============================================================================

void get_new_size_for_sprite(int ee, int ww, int hh, int &newwid, int &newhit) {
  newwid = ww;
  newhit = hh;
}
int spritewidth[MAX_SPRITES + 5], spriteheight[MAX_SPRITES + 5];
//


//=============================================================================
// AGS.Native-specific implementations split out of sprcache.cpp
//=============================================================================

void SpriteCache::initFile_adjustBuffers(short numspri)
{
  // do nothing
}

void SpriteCache::initFile_initNullSpriteParams(int vv)
{
  // no sprite ... blank it out
  spritewidth[vv] = 0;
  spriteheight[vv] = 0;
  offsets[vv] = 0;
}
