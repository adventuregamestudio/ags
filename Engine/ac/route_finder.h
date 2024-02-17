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

#include "ac/game_version.h"

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
// Append a waypoint to the move list, skip pathfinding
bool add_waypoint_direct(MoveList * mlsp, short x, short y, int move_speed_x, int move_speed_y);
void recalculate_move_speeds(MoveList *mlsp, int old_speed_x, int old_speed_y, int new_speed_x, int new_speed_y);

#endif // __AC_ROUTEFND_H
