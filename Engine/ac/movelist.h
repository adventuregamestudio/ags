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

struct MoveList
{
    int     numstage = 0;
    Point   pos[MAXNEEDSTAGES];
    // xpermove and ypermove contain number of pixels done per a single step
    // along x and y axes; i.e. this is a movement vector, per path stage
    fixed   xpermove[MAXNEEDSTAGES]{};
    fixed   ypermove[MAXNEEDSTAGES]{};
    Point   from;
    int     onstage = 0; // current path stage
    int     onpart = 0; // total number of steps done on this stage
    uint8_t doneflag = 0u;
    uint8_t direct = 0;  // MoveCharDirect was used or not

    // Gets a movelist's step length, in coordinate units
    // (normally the coord unit is a game pixel)
    float GetStepLength() const;

    void ReadFromFile_Legacy(Common::Stream *in);
    AGS::Engine::HSaveError ReadFromFile(Common::Stream *in, int32_t cmp_ver);
    void WriteToFile(Common::Stream *out);
};

#endif // __AGS_EN_AC__MOVELIST_H
