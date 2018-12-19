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
// Deprecated room stuff. Removed from room class and load routine because this
// data is no longer supported in the engine. Perhaps move to some legacy
// knowledge base; or restore when it's possible to support ancient games.
//
//=============================================================================

#if defined (OBSOLETE)

#include "ac/common.h"
#include "util/stream.h"

using namespace AGS::Common;

#define AE_WAITFLAG   0x80000000
#define MAXANIMSTAGES 10
struct AnimationStruct
{
    int   x, y;
    int   data;
    int   object;
    int   speed;
    char  action;
    char  wait;
    AnimationStruct() { action = 0; object = 0; wait = 1; speed = 5; }
};

struct FullAnimation
{
    AnimationStruct stage[MAXANIMSTAGES];
    int             numstages;
    FullAnimation() { numstages = 0; }
};

#define MAXPOINTS 30
struct PolyPoints
{
    int x[MAXPOINTS];
    int y[MAXPOINTS];
    int numpoints;
    void add_point(int x, int y);
    PolyPoints() { numpoints = 0; }

    void Read(AGS::Common::Stream *in);
};


#define MAXANIMS 10
// Just a list of cut out data
struct DeprecatedRoomStruct
{
    // Full-room animations
    int16_t       numanims;
    FullAnimation anims[MAXANIMS];
    // Polygonal walkable areas (unknown version)
    int32_t       numwalkareas;
    PolyPoints    wallpoints[MAX_WALK_AREAS];
    // Unknown flags
    int16_t       flagstates[MAX_FLAGS];
};



void PolyPoints::add_point(int x, int y)
{
    x[numpoints] = x;
    y[numpoints] = y;
    numpoints++;

    if (numpoints >= MAXPOINTS)
        quit("too many poly points added");
}

void PolyPoints::Read(Stream *in)
{
    in->ReadArrayOfInt32(x, MAXPOINTS);
    in->ReadArrayOfInt32(y, MAXPOINTS);
    numpoints = in->ReadInt32();
}

#endif // OBSOLETE
