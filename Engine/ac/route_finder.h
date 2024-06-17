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
#ifndef __AC_ROUTEFND_H
#define __AC_ROUTEFND_H

#include <vector>
#include "ac/game_version.h"
#include "util/geometry.h"

// Forward declaration
namespace AGS { namespace Common { class Bitmap; }}
struct MoveList;

void init_pathfinder(GameDataVersion game_file_version);
void shutdown_pathfinder();

void set_wallscreen(AGS::Common::Bitmap *wallscreen);

int can_see_from(int x1, int y1, int x2, int y2);
void get_lastcpos(int &lastcx, int &lastcy);

int find_route(short srcx, short srcy, short xx, short yy, int move_speed_x, int move_speed_y,
    AGS::Common::Bitmap *onscreen, int movlst, int nocross = 0, int ignore_walls = 0);

//
// Various additional pathfinding functions and helpers.
// Manages converting navigation paths into MoveLists.
namespace Pathfinding
{
    // Calculate the MoveList from the given navigation path and move speeds.
    bool CalculateMoveList(MoveList &mls, const std::vector<Point> path, int move_speed_x, int move_speed_y);
    // Append a waypoint to the move list, skip pathfinding
    bool AddWaypointDirect(MoveList &mls, int x, int y, int move_speed_x, int move_speed_y);
    // Recalculates MoveList's step speeds
    void RecalculateMoveSpeeds(MoveList &mls, int old_speed_x, int old_speed_y, int new_speed_x, int new_speed_y);
}

#endif // __AC_ROUTEFND_H
