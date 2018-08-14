//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// Implementation from sprcache.cpp specific to Engine runtime
//
//=============================================================================

// Headers, as they are in sprcache.cpp
#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#pragma warning (disable: 4996 4312)  // disable deprecation warnings
#endif

#include "ac/spritecache.h"
#include "util/compress.h"

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
  _sprInfos[vv].Width = _sprInfos[0].Width;
  _sprInfos[vv].Height = _sprInfos[0].Height;
  offsets[vv] = offsets[0];
  flags[vv] = SPRCACHEFLAG_DOESNOTEXIST;
}
