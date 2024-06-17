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
//
// New jump point search (JPS) A* pathfinder by Martin Sedlak.
//
//=============================================================================
#include "ac/route_finder_impl.h"
#include <string.h>
#include <math.h>
#include "ac/movelist.h"
#include "ac/common_defines.h"
#include "ac/route_finder.h"
#include "ac/route_finder_jps.inl"
#include "gfx/bitmap.h"
#include "debug/out.h"

extern std::vector<MoveList> mls;

using AGS::Common::Bitmap;

// #define DEBUG_PATHFINDER

namespace AGS {
namespace Engine {
namespace RouteFinder {

static const int MAXNAVPOINTS = MAXNEEDSTAGES;
static std::vector<Point> navpoints;
static Navigation nav;
static Bitmap *wallscreen;
static int lastcx, lastcy;

void init_pathfinder()
{
}

void shutdown_pathfinder()
{
}

void set_wallscreen(Bitmap *wallscreen_) 
{
  wallscreen = wallscreen_;
}

static void sync_nav_wallscreen()
{
  // FIXME: this is dumb, but...
  nav.Resize(wallscreen->GetWidth(), wallscreen->GetHeight());

  for (int y=0; y<wallscreen->GetHeight(); y++)
    nav.SetMapRow(y, wallscreen->GetScanLine(y));
}

int can_see_from(int x1, int y1, int x2, int y2)
{
  lastcx = x1;
  lastcy = y1;

  if ((x1 == x2) && (y1 == y2))
    return 1;

  sync_nav_wallscreen();

  return !nav.TraceLine(x1, y1, x2, y2, lastcx, lastcy);
}

void get_lastcpos(int &lastcx_, int &lastcy_) 
{
  lastcx_ = lastcx;
  lastcy_ = lastcy;
}

// new routing using JPS
static int find_route_jps(int fromx, int fromy, int destx, int desty)
{
  sync_nav_wallscreen();

  static std::vector<int> path, cpath;
  path.clear();
  cpath.clear();

  if (nav.NavigateRefined(fromx, fromy, destx, desty, path, cpath) == Navigation::NAV_UNREACHABLE)
    return 0;

  navpoints.clear();

  // new behavior: cut path if too complex rather than abort with error message
  int count = std::min<int>((int)cpath.size(), MAXNAVPOINTS);

  for (int i = 0; i<count; i++)
  {
    int x, y;
    nav.UnpackSquare(cpath[i], x, y);
    navpoints.emplace_back( x, y );
  }

  return 1;
}

int find_route(short srcx, short srcy, short xx, short yy, int move_speed_x, int move_speed_y,
    Bitmap *onscreen, int move_id, int nocross, int ignore_walls)
{
  wallscreen = onscreen;

  navpoints.clear();

  if (ignore_walls || can_see_from(srcx, srcy, xx, yy))
  {
    navpoints.emplace_back( srcx, srcy );
    navpoints.emplace_back( xx, yy );
  } else {
    if ((nocross == 0) && (wallscreen->GetPixel(xx, yy) == 0))
      return 0; // clicked on a wall

    find_route_jps(srcx, srcy, xx, yy);
  }

  if (navpoints.empty())
    return 0;

  // FIXME: really necessary?
  if (navpoints.size() == 1)
    navpoints.push_back(navpoints[0]);

  assert(navpoints.size() <= MAXNAVPOINTS);

#ifdef DEBUG_PATHFINDER
  AGS::Common::Debug::Printf("Route from %d,%d to %d,%d - %zu stages", srcx,srcy,xx,yy,navpoints.size());
#endif

  MoveList mlist;
  Pathfinding::CalculateMoveList(mlist, navpoints, move_speed_x, move_speed_y);
  mls[move_id] = mlist;
  return move_id;
}

} // namespace RouteFinder
} // namespace Engine
} // namespace AGS
