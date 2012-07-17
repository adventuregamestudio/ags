//
// Implementation from sprcache.cpp specific to AGS.Native library
//

// Headers, as they are in sprcache.cpp
#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#pragma warning (disable: 4996 4312)  // disable deprecation warnings
#endif

#include "util/wgt2allg.h"
#include "ac/common_defines.h"
#include "ac/spritecache.h"
#include "util/compress.h"
//

//=============================================================================
// AGS.Native-specific implementation split out of sprcache.cpp
//=============================================================================

void get_new_size_for_sprite(int ee, int ww, int hh, int &newwid, int &newhit) {
  newwid = ww;
  newhit = hh;
}
int spritewidth[MAX_SPRITES + 5], spriteheight[MAX_SPRITES + 5];

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
