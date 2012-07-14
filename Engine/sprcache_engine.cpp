//
// Implementation from sprcache.cpp specific to Engine runtime
//

// Headers, as they are in sprcache.cpp
#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#pragma warning (disable: 4996 4312)  // disable deprecation warnings
#endif

#include "wgt2allg.h"
#include "sprcache.h"
#include "util/compress.h"
#include "bigend.h"
//

// For engine these are defined in ac.cpp
extern int spritewidth[], spriteheight[];
//

//=============================================================================
// Engine-specific implementation split out of sprcache.cpp
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
