//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
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

#include "ac/gamestructdefines.h"
#include "ac/spritecache.h"
#include "util/compress.h"

//=============================================================================
// Engine-specific implementation split out of sprcache.cpp
//=============================================================================

void AGS::Common::SpriteCache::InitNullSpriteParams(sprkey_t index)
{
    // make it a blue cup, to avoid crashes
    _sprInfos[index].Width = _sprInfos[0].Width;
    _sprInfos[index].Height = _sprInfos[0].Height;
    _spriteData[index].Image = nullptr;
    _spriteData[index].Size = _spriteData[0].Size;
    _spriteData[index].Flags = SPRCACHEFLAG_REMAPPED;
}
