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
static Bitmap *walkablearea;
static int lastcx, lastcy;

void init_pathfinder()
{
}

void shutdown_pathfinder()
{
}

void set_walkablearea(Bitmap *walkablearea_) 
{
  walkablearea = walkablearea_;
}

static void sync_nav_walkablearea()
{
  // FIXME: this is dumb, but...
  nav.Resize(walkablearea->GetWidth(), walkablearea->GetHeight());

  for (int y=0; y<walkablearea->GetHeight(); y++)
    nav.SetMapRow(y, walkablearea->GetScanLine(y));
}

int can_see_from(int x1, int y1, int x2, int y2)
{
  lastcx = x1;
  lastcy = y1;

  if ((x1 == x2) && (y1 == y2))
    return 1;

  sync_nav_walkablearea();

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
  sync_nav_walkablearea();

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

inline float input_speed_to_move(int speed_val)
{
  // negative move speeds like -2 get converted to 1/2
  if (speed_val < 0) {
    return 1.f / (-speed_val);
  }
  else
    return speed_val;
}

void set_route_move_speed(int speed_x, int speed_y)
{
  move_speed_x = input_speed_to_move(speed_x);
  move_speed_y = input_speed_to_move(speed_y);
}

inline float calc_move_speed_at_angle(float speed_x, float speed_y, float xdist, float ydist)
{
  float useMoveSpeed;
  // short circuit degenerate "simple" cases
  if (xdist == 0.f || speed_x == 0.f) {
      useMoveSpeed = speed_y;
  }
  else if (ydist == 0.f || speed_y == 0.f) {
      useMoveSpeed = speed_x;
  }
  else if (speed_x == speed_y) {
    useMoveSpeed = speed_x;
  }
  else {
    // different X and Y move speeds
    // speed_x and speed_y are the axis of an ellipse, whose border represent the "valid"
    // movement speeds at each angle. The equation for that is
    // (x/a)^2 + (y/b)^2 = 1
    // where
    // a == speed_x
    // b == speed_y
    // ydist and xdist give a straight line for the movement at this stage. Its equation is
    // y = mx
    // The slope m is ydist/xdist.
    // The velocity we want to compute is the length of the segment of that line from the
    // origin to its intersection with the ellipse. The coordinates of that intersection
    // can be found by substituting y = mx into the equation of the ellipse to solve for
    // x, and then solving back for y. The velocity is then computed by Pithagora's theorem.
    float a_squared = speed_x * speed_x;
    float b_squared = speed_y * speed_y;
    float m_squared = (ydist * ydist) / (xdist * xdist);
    float v_squared = (a_squared * b_squared * (1.f + m_squared)) / (b_squared + a_squared * m_squared);
    useMoveSpeed = sqrtf(v_squared);
  }
  // validate that the computed speed is in a valid range
  assert(useMoveSpeed >= std::min(speed_x, speed_y) && useMoveSpeed <= std::max(speed_x, speed_y));
  return useMoveSpeed;
}

// Calculates the X and Y per game loop, for this stage of the
// movelist
void calculate_move_stage(MoveList * mlsp, int index)
{
  // work out the x & y per move. First, opp/adj=tan, so work out the angle
  if (mlsp->pos[index] == mlsp->pos[index + 1]) {
    mlsp->xpermove[index] = 0;
    mlsp->ypermove[index] = 0;
    return;
  }

  int ourx = mlsp->pos[index].X;
  int oury = mlsp->pos[index].Y;
  int destx = mlsp->pos[index + 1].X;
  int desty = mlsp->pos[index + 1].Y;

  // Special case for vertical and horizontal movements
  if (ourx == destx) {
    mlsp->xpermove[index] = 0;
    mlsp->ypermove[index] = move_speed_y;
    if (desty < oury)
      mlsp->ypermove[index] = -mlsp->ypermove[index];

    return;
  }

  if (oury == desty) {
    mlsp->xpermove[index] = move_speed_x;
    mlsp->ypermove[index] = 0;
    if (destx < ourx)
      mlsp->xpermove[index] = -mlsp->xpermove[index];

    return;
  }

  float xdist = abs(ourx - destx);
  float ydist = abs(oury - desty);

  float useMoveSpeed = calc_move_speed_at_angle(move_speed_x, move_speed_y, xdist, ydist);
  float angl = atan(ydist / xdist);

  // now, since new opp=hyp*sin, work out the Y step size
  //fixed newymove = useMoveSpeed * fsin(angl);
  float newymove = useMoveSpeed * sin(angl);

  // since adj=hyp*cos, work out X step size
  //fixed newxmove = useMoveSpeed * fcos(angl);
  float newxmove = useMoveSpeed * cos(angl);

  // validate that the computed movement isn't larger than the set maxima
  assert(newxmove <= move_speed_x && newymove <= move_speed_y);

  if (destx < ourx)
    newxmove = -newxmove;
  if (desty < oury)
    newymove = -newymove;

  mlsp->xpermove[index] = newxmove;
  mlsp->ypermove[index] = newymove;
}

void recalculate_move_speeds(MoveList *mlsp, int old_speed_x, int old_speed_y, int new_speed_x, int new_speed_y)
{
  const float old_movspeed_x = input_speed_to_move(old_speed_x);
  const float old_movspeed_y = input_speed_to_move(old_speed_y);
  const float new_movspeed_x = input_speed_to_move(new_speed_x);
  const float new_movspeed_y = input_speed_to_move(new_speed_y);
  // save current stage's step lengths, for later onpart's update
  const float old_stage_xpermove = mlsp->xpermove[mlsp->onstage];
  const float old_stage_ypermove = mlsp->ypermove[mlsp->onstage];

  for (int i = 0; (i < mlsp->numstage) && ((mlsp->xpermove[i] != 0) || (mlsp->ypermove[i] != 0)); ++i)
  {
    // First three cases where the speed is a plain factor, therefore
    // we may simply divide on old one and multiple on a new one
    if ((old_movspeed_x == old_movspeed_y) || // diagonal move at straight 45 degrees
        (mlsp->xpermove[i] == 0) || // straight vertical move
        (mlsp->ypermove[i] == 0))   // straight horizontal move
    {
      mlsp->xpermove[i] = (mlsp->xpermove[i] * new_movspeed_x) / old_movspeed_x;
      mlsp->ypermove[i] = (mlsp->ypermove[i] * new_movspeed_y) / old_movspeed_y;
    }
    else
    {
      // Move at angle has adjusted speed factor, which we must recalculate first
      int ourx = mlsp->pos[i].X;
      int oury = mlsp->pos[i].Y;
      int destx = mlsp->pos[i + 1].X;
      int desty = mlsp->pos[i + 1].Y;

      float xdist = itofix(abs(ourx - destx));
      float ydist = itofix(abs(oury - desty));
      float old_speed_at_angle = calc_move_speed_at_angle(old_movspeed_x, old_movspeed_y, xdist, ydist);
      float new_speed_at_angle = calc_move_speed_at_angle(new_movspeed_x, new_movspeed_y, xdist, ydist);

      mlsp->xpermove[i] = (mlsp->xpermove[i] * new_speed_at_angle) / old_speed_at_angle;
      mlsp->ypermove[i] = (mlsp->ypermove[i] * new_speed_at_angle) / old_speed_at_angle;
    }
  }

  // now adjust current passed stage fraction
  if (mlsp->onpart >= 0.f)
  {
    if (old_stage_xpermove != 0)
      mlsp->onpart = (mlsp->onpart * old_stage_xpermove) / mlsp->xpermove[mlsp->onstage];
    else
      mlsp->onpart = (mlsp->onpart * old_stage_ypermove) / mlsp->ypermove[mlsp->onstage];
  }
}


int find_route(short srcx, short srcy, short xx, short yy, Bitmap *onscreen, int movlst, int nocross, int ignore_walls)
{
  int i;

  walkablearea = onscreen;

  num_navpoints = 0;

  if (ignore_walls || can_see_from(srcx, srcy, xx, yy))
  {
    num_navpoints = 2;
    navpoints[0] = { srcx, srcy };
    navpoints[1] = { xx, yy };
  } else {
    if ((nocross == 0) && (walkablearea->GetPixel(xx, yy) == 0))
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

  mls[mlist].from = { srcx, srcy };
  mls[mlist].onstage = 0;
  mls[mlist].onpart = 0.f;
  mls[mlist].doneflag = 0;
  return mlist;
}


} // namespace RouteFinder
} // namespace Engine
} // namespace AGS
