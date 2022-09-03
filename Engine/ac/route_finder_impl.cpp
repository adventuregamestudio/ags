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
static float move_speed_x, move_speed_y;
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

void set_route_move_speed(int speed_x, int speed_y)
{
  // negative move speeds like -2 get converted to 1/2
  if (speed_x < 0) {
    move_speed_x = 1.0 / (-speed_x);
  }
  else {
    move_speed_x = speed_x;
  }

  if (speed_y < 0) {
    move_speed_y = 1.0 / (-speed_y);
  }
  else {
    move_speed_y = speed_y;
  }
}

// Calculates the X and Y per game loop, for this stage of the
// movelist
void calculate_move_stage(MoveList * mlsp, int aaa)
{
  // work out the x & y per move. First, opp/adj=tan, so work out the angle
  if (mlsp->pos[aaa] == mlsp->pos[aaa + 1]) {
    mlsp->xpermove[aaa] = 0;
    mlsp->ypermove[aaa] = 0;
    return;
  }

  int ourx = mlsp->pos[aaa].X;
  int oury = mlsp->pos[aaa].Y;
  int destx = mlsp->pos[aaa + 1].X;
  int desty = mlsp->pos[aaa + 1].Y;

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

  float xdist = abs(ourx - destx);
  float ydist = abs(oury - desty);

  float useMoveSpeed;

  if (move_speed_x == move_speed_y) {
    useMoveSpeed = move_speed_x;
  }
  else {
    // different X and Y move speeds
    // the X proportion of the movement is (x / (x + y))
    float xproportion = xdist / (xdist + ydist);

    // TODO: Investigate why the following comments are the opposite of what's being done
    if (move_speed_x > move_speed_y) {
      // speed = y + ((1 - xproportion) * (x - y))
      useMoveSpeed = move_speed_y + (xproportion * (move_speed_x - move_speed_y));
    }
    else {
      // speed = x + (xproportion * (y - x))
      useMoveSpeed = move_speed_x + ((1 - xproportion) * (move_speed_y - move_speed_x));
    }
  }

  float angl = atan(ydist / xdist);

  // now, since new opp=hyp*sin, work out the Y step size
  //fixed newymove = useMoveSpeed * fsin(angl);
  float newymove = useMoveSpeed * sin(angl);

  // since adj=hyp*cos, work out X step size
  //fixed newxmove = useMoveSpeed * fcos(angl);
  float newxmove = useMoveSpeed * cos(angl);

  if (destx < ourx)
    newxmove = -newxmove;
  if (desty < oury)
    newymove = -newymove;

  mlsp->xpermove[aaa] = newxmove;
  mlsp->ypermove[aaa] = newymove;
}


int find_route(short srcx, short srcy, short xx, short yy, Bitmap *onscreen, int movlst, int nocross, int ignore_walls)
{
  int i;

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

  int mlist = movlst;
  mls[mlist].numstage = num_navpoints;
  memcpy(&mls[mlist].pos[0], &navpoints[0], sizeof(Point) * num_navpoints);
#ifdef DEBUG_PATHFINDER
  AGS::Common::Debug::Printf("stages: %d\n",num_navpoints);
#endif

  for (i=0; i<num_navpoints-1; i++)
    calculate_move_stage(&mls[mlist], i);

  mls[mlist].fromx = srcx;
  mls[mlist].fromy = srcy;
  mls[mlist].onstage = 0;
  mls[mlist].onpart = 0;
  mls[mlist].doneflag = 0;
  mls[mlist].lastx = -1;
  mls[mlist].lasty = -1;
  return mlist;
}


} // namespace RouteFinder
} // namespace Engine
} // namespace AGS
