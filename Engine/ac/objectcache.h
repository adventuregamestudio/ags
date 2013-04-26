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
#ifndef __AGS_EE_AC__OBJECTCACHE_H
#define __AGS_EE_AC__OBJECTCACHE_H

#include "core/types.h"

namespace AGS { namespace Common { class Bitmap; } }
using namespace AGS; // REMOVE later

// stores cached object info
struct ObjectCache {
    Common::Bitmap *image;
    int   sppic;
    short tintredwas, tintgrnwas, tintbluwas, tintamntwas, tintlightwas;
    short lightlevwas, mirroredWas, zoomWas;
    // The following are used to determine if the object has moved
    int   xwas, ywas;

    ObjectCache()
        : image(NULL)
        , sppic(0)
        , tintredwas(0)
        , tintgrnwas(0)
        , tintbluwas(0)
        , tintamntwas(0)
        , tintlightwas(0)
        , lightlevwas(0)
        , mirroredWas(0)
        , zoomWas(0)
        , xwas(0)
        , ywas(0)
    {
    }
};

#endif // __AGS_EE_AC__OBJECTCACHE_H
