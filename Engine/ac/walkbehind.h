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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__WALKBEHIND_H
#define __AGS_EE_AC__WALKBEHIND_H

#include "util/array.h"

namespace AGS { namespace Engine { class IDriverDependantBitmap; } }

enum WalkBehindMethodEnum
{
    DrawOverCharSprite,
    DrawAsSeparateSprite,
    DrawAsSeparateCharSprite
};

struct WalkBehindPlacement
{
    int Left;
    int Top;
    int Right;
    int Bottom;
    AGS::Engine::IDriverDependantBitmap *Ddb;

    WalkBehindPlacement();
    WalkBehindPlacement(const WalkBehindPlacement &wbplace);
    ~WalkBehindPlacement();
};

void update_walk_behind_images();
void recache_walk_behinds ();

extern AGS::Common::ObjectArray<WalkBehindPlacement> WalkBehindPlacements;

#endif // __AGS_EE_AC__WALKBEHIND_H
