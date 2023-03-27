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
#ifndef __AC_MOVE_H
#define __AC_MOVE_H
#include "game/savegame.h"
#include "util/geometry.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#define MAXNEEDSTAGES 256

enum MoveListDoneFlags
{
    kMoveListDone_X = 0x01,
    kMoveListDone_Y = 0x02,
    kMoveListDone_XY = kMoveListDone_X | kMoveListDone_Y
};

struct MoveList {
    Point pos[MAXNEEDSTAGES]{};
    int   numstage = 0;
    float xpermove[MAXNEEDSTAGES]{}, ypermove[MAXNEEDSTAGES]{};
    int   fromx = 0, fromy = 0;
    int   onstage = 0, onpart = 0;
    int   lastx = 0, lasty = 0;
    char  doneflag = 0;
    char  direct = 0;  // MoveCharDirect was used or not

    AGS::Engine::HSaveError ReadFromFile(Common::Stream *in, int32_t cmp_ver);
    void WriteToFile(Common::Stream *out) const;
};

#endif // __AC_MOVE_H
