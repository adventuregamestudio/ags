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

#include "ac/common.h"   // quit()
#include "ac/movelist.h"     // MoveList
#include "ac/common_defines.h"
#include "gfx/bitmap.h"
#include "debug/out.h"

#include "route_finder_jps.inl"

extern std::vector<MoveList> mls;

using AGS::Common::Bitmap;

// #define DEBUG_PATHFINDER

namespace AGS {
namespace Engine {
namespace RouteFinder {

static const int MAXNAVPOINTS = MAXNEEDSTAGES;
static Point navpoints[MAXNAVPOINTS];
static int num_navpoints;
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

  num_navpoints = 0;

  // new behavior: cut path if too complex rather than abort with error message
  int count = std::min<int>((int)cpath.size(), MAXNAVPOINTS);

  for (int i = 0; i<count; i++)
  {
    int x, y;
    nav.UnpackSquare(cpath[i], x, y);

    navpoints[num_navpoints++] = { x, y };
  }

  return 1;
}

inline fixed input_speed_to_fixed(int speed_val)
{
  // negative move speeds like -2 get converted to 1/2
  if (speed_val < 0) {
    return itofix(1) / (-speed_val);
  }
  else {
    return itofix(speed_val);
  }
}

inline fixed calc_move_speed_at_angle(fixed speed_x, fixed speed_y, fixed xdist, fixed ydist)
{
  fixed useMoveSpeed;
  if (speed_x == speed_y) {
    useMoveSpeed = speed_x;
  }
  else {
    // different X and Y move speeds
    // the X proportion of the movement is (x / (x + y))
    fixed xproportion = fixdiv(xdist, (xdist + ydist));

    if (speed_x > speed_y) {
      // speed = y + ((1 - xproportion) * (x - y))
      useMoveSpeed = speed_y + fixmul(xproportion, speed_x - speed_y);
    }
    else {
      // speed = x + (xproportion * (y - x))
      useMoveSpeed = speed_x + fixmul(itofix(1) - xproportion, speed_y - speed_x);
    }
  }
  return useMoveSpeed;
}

// Calculates the X and Y per game loop, for this stage of the movelist
void calculate_move_stage(MoveList *mlsp, int aaa, fixed move_speed_x, fixed move_speed_y)
{
  // work out the x & y per move. First, opp/adj=tan, so work out the angle
  if (mlsp->pos[aaa] == mlsp->pos[aaa + 1]) {
    mlsp->xpermove[aaa] = 0;
    mlsp->ypermove[aaa] = 0;
    return;
  }

  short ourx = mlsp->pos[aaa].X;
  short oury = mlsp->pos[aaa].Y;
  short destx = mlsp->pos[aaa + 1].X;
  short desty = mlsp->pos[aaa + 1].Y;

  // Special case for vertical and horizontal movements
  if (ourx == destx) {
    mlsp->xpermove[aaa] = 0;
    mlsp->ypermove[aaa] = move_speed_y;
    if (desty < oury)
      mlsp->ypermove[aaa] = -mlsp->ypermove[aaa];

    return;
  }

  if (oury == desty) {
    mlsp->xpermove[aaa] = move_speed_x;
    mlsp->ypermove[aaa] = 0;
    if (destx < ourx)
      mlsp->xpermove[aaa] = -mlsp->xpermove[aaa];

    return;
  }

  fixed xdist = itofix(abs(ourx - destx));
  fixed ydist = itofix(abs(oury - desty));

  fixed useMoveSpeed = calc_move_speed_at_angle(move_speed_x, move_speed_y, xdist, ydist);

  fixed angl = fixatan(fixdiv(ydist, xdist));

  // now, since new opp=hyp*sin, work out the Y step size
  //fixed newymove = useMoveSpeed * fsin(angl);
  fixed newymove = fixmul(useMoveSpeed, fixsin(angl));

  // since adj=hyp*cos, work out X step size
  //fixed newxmove = useMoveSpeed * fcos(angl);
  fixed newxmove = fixmul(useMoveSpeed, fixcos(angl));

  if (destx < ourx)
    newxmove = -newxmove;
  if (desty < oury)
    newymove = -newymove;

  mlsp->xpermove[aaa] = newxmove;
  mlsp->ypermove[aaa] = newymove;
}

void recalculate_move_speeds(MoveList *mlsp, int old_speed_x, int old_speed_y, int new_speed_x, int new_speed_y)
{
  const fixed old_movspeed_x = input_speed_to_fixed(old_speed_x);
  const fixed old_movspeed_y = input_speed_to_fixed(old_speed_y);
  const fixed new_movspeed_x = input_speed_to_fixed(new_speed_x);
  const fixed new_movspeed_y = input_speed_to_fixed(new_speed_y);
  // save current stage's step lengths, for later onpart's update
  const fixed old_stage_xpermove = mlsp->xpermove[mlsp->onstage];
  const fixed old_stage_ypermove = mlsp->ypermove[mlsp->onstage];

  for (int i = 0; (i < mlsp->numstage) && ((mlsp->xpermove[i] != 0) || (mlsp->ypermove[i] != 0)); ++i)
  {
    // First three cases where the speed is a plain factor, therefore
    // we may simply divide on old one and multiple on a new one
    if ((old_movspeed_x == old_movspeed_y) || // diagonal move at straight 45 degrees
        (mlsp->xpermove[i] == 0) || // straight vertical move
        (mlsp->ypermove[i] == 0))   // straight horizontal move
    {
      mlsp->xpermove[i] = fixdiv(fixmul(mlsp->xpermove[i], new_movspeed_x), old_movspeed_x);
      mlsp->ypermove[i] = fixdiv(fixmul(mlsp->ypermove[i], new_movspeed_y), old_movspeed_y);
    }
    else
    {
      // Move at angle has adjusted speed factor, which we must recalculate first
      short ourx = mlsp->pos[i].X;
      short oury = mlsp->pos[i].Y;
      short destx = mlsp->pos[i + 1].X;
      short desty = mlsp->pos[i + 1].Y;

      fixed xdist = itofix(abs(ourx - destx));
      fixed ydist = itofix(abs(oury - desty));
      fixed old_speed_at_angle = calc_move_speed_at_angle(old_movspeed_x, old_movspeed_y, xdist, ydist);
      fixed new_speed_at_angle = calc_move_speed_at_angle(new_movspeed_x, new_movspeed_y, xdist, ydist);

      mlsp->xpermove[i] = fixdiv(fixmul(mlsp->xpermove[i], new_speed_at_angle), old_speed_at_angle);
      mlsp->ypermove[i] = fixdiv(fixmul(mlsp->ypermove[i], new_speed_at_angle), old_speed_at_angle);
    }
  }

  // now adjust current passed stage fraction
  if (mlsp->onpart >= 0.f)
  {
    if (old_stage_xpermove != 0)
      mlsp->onpart = (mlsp->onpart * fixtof(old_stage_xpermove)) / fixtof(mlsp->xpermove[mlsp->onstage]);
    else
      mlsp->onpart = (mlsp->onpart * fixtof(old_stage_ypermove)) / fixtof(mlsp->ypermove[mlsp->onstage]);
  }
}


int find_route(short srcx, short srcy, short xx, short yy, int move_speed_x, int move_speed_y,
    Bitmap *onscreen, int move_id, int nocross, int ignore_walls)
{
  wallscreen = onscreen;

  num_navpoints = 0;

  if (ignore_walls || can_see_from(srcx, srcy, xx, yy))
  {
    num_navpoints = 2;
    navpoints[0] = { srcx, srcy };
    navpoints[1] = { xx, yy };
  } else {
    if ((nocross == 0) && (wallscreen->GetPixel(xx, yy) == 0))
      return 0; // clicked on a wall

    find_route_jps(srcx, srcy, xx, yy);
  }

  if (!num_navpoints)
    return 0;

  // FIXME: really necessary?
  if (num_navpoints == 1)
    navpoints[num_navpoints++] = navpoints[0];

  assert(num_navpoints <= MAXNAVPOINTS);

#ifdef DEBUG_PATHFINDER
  AGS::Common::Debug::Printf("Route from %d,%d to %d,%d - %d stages", srcx,srcy,xx,yy,num_navpoints);
#endif

  MoveList mlist;
  mlist.numstage = num_navpoints;
  memcpy(&mlist.pos[0], &navpoints[0], sizeof(Point) * num_navpoints);
#ifdef DEBUG_PATHFINDER
  AGS::Common::Debug::Printf("stages: %d\n",num_navpoints);
#endif

  const fixed fix_speed_x = input_speed_to_fixed(move_speed_x);
  const fixed fix_speed_y = input_speed_to_fixed(move_speed_y);
  for (int i=0; i<num_navpoints-1; i++)
    calculate_move_stage(&mlist, i, fix_speed_x, fix_speed_y);

  mlist.from = { srcx, srcy };
  mls[move_id] = mlist;
  return move_id;
}

bool add_waypoint_direct(MoveList * mlsp, short x, short y, int move_speed_x, int move_speed_y)
{
  if (mlsp->numstage >= MAXNEEDSTAGES)
    return false;

  const fixed fix_speed_x = input_speed_to_fixed(move_speed_x);
  const fixed fix_speed_y = input_speed_to_fixed(move_speed_y);
  mlsp->pos[mlsp->numstage] = { x, y };
  calculate_move_stage(mlsp, mlsp->numstage - 1, fix_speed_x, fix_speed_y);
  mlsp->numstage++;
  return true;
}


} // namespace RouteFinder
} // namespace Engine
} // namespace AGS
