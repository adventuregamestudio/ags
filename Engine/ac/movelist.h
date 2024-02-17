//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EN_AC__MOVELIST_H
#define __AGS_EN_AC__MOVELIST_H
#include <allegro.h> // fixed math
#include "game/savegame.h"
#include "util/geometry.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#define MAXNEEDSTAGES 256
#define MAXNEEDSTAGES_LEGACY 40

enum MoveListDoneFlags
{
    kMoveListDone_X = 0x01,
    kMoveListDone_Y = 0x02,
    kMoveListDone_XY = kMoveListDone_X | kMoveListDone_Y
};

enum MoveListSvgVersion
{
    kMoveSvgVersion_Initial = 0, // [UNSUPPORTED] from 3.5.0 pre-alpha
    kMoveSvgVersion_350,   // new pathfinder, arbitrary number of stages
    kMoveSvgVersion_36109, // skip empty lists, progress as float
};

struct MoveList
{
    int     numstage = 0;
    // Waypoints, per stage
    Point   pos[MAXNEEDSTAGES];
    // xpermove and ypermove contain number of pixels done per a single step
    // along x and y axes; i.e. this is a movement vector, per path stage
    fixed   xpermove[MAXNEEDSTAGES]{};
    fixed   ypermove[MAXNEEDSTAGES]{};
    int     onstage = 0; // current path stage
    Point   from; // current stage's starting position
    // Steps made during current stage;
    // distance passed is calculated as xpermove[onstage] * onpart;
    // made a fractional value to let recalculate movelist dynamically
    float   onpart = 0.f;
    uint8_t doneflag = 0u;
    uint8_t direct = 0;  // MoveCharDirect was used or not

    // Dynamic fixups, not serialized
    // Final section move speed and steps, used when an object
    // finishes one of the axes sooner than the other
    fixed   fin_move = 0;
    float   fin_from_part = 0.f;

    const Point &GetLastPos() const { return numstage > 0 ? pos[numstage - 1] : pos[0]; }

    // Gets a movelist's step length, in coordinate units
    // (normally the coord unit is a game pixel)
    float GetStepLength() const;
    // Gets a fraction of a coordinate unit that is in progress of stepping over;
    // (normally the coord unit is a game pixel)
    float GetPixelUnitFraction() const;
    // Sets a step progress to this fraction of a coordinate unit
    void  SetPixelUnitFraction(float frac);

    AGS::Engine::HSaveError ReadFromSavegame(Common::Stream *in, int32_t cmp_ver);
    void WriteToSavegame(Common::Stream *out) const;
};

#endif // __AGS_EN_AC__MOVELIST_H
