//
// Implementation from sprcache.cpp specific to Engine runtime
//

// Headers, as they are in sprcache.cpp
#define WGT2ALLEGRO_NOFUNCTIONS
#define CROOM_NOFUNCTIONS
#include "wgt2allg.h"
#include "acroom.h"
#include "sprcache.h"
#include "compress.h"
#include "bigend.h"
//

// For engine these are originally defined in ac.cpp
extern int spritewidth[], spriteheight[];
//

//=============================================================================
// Engine-specific implementations split out of sprcache.cpp
//=============================================================================

void SpriteCache::initFile_adjustBuffers(short numspri)
{
  // adjust the buffers to the sprite file size
  changeMaxSize(numspri + 1);
}

void SpriteCache::initFile_initNullSpriteParams(int vv)
{
  // make it a blue cup, to avoid crashes
  spritewidth[vv] = spritewidth[0];
  spriteheight[vv] = spriteheight[0];
  offsets[vv] = offsets[0];
  flags[vv] = SPRCACHEFLAG_DOESNOTEXIST;
}
