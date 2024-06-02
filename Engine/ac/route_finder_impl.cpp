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
// Jump point search (JPS) A* pathfinder implementation by Martin Sedlak.
//
//=============================================================================
#include "ac/route_finder_impl.h"
#include "ac/route_finder_jps.inl"
#include "gfx/bitmap.h"

using namespace AGS::Common;

// #define DEBUG_PATHFINDER

namespace AGS
{
namespace Engine
{

JPSRouteFinder::JPSRouteFinder()
    : nav(*new Navigation())
{
}

JPSRouteFinder::~JPSRouteFinder()
{
    delete &nav;
}

void JPSRouteFinder::SetWalkableArea(const Bitmap *walkablearea) 
{
    this->walkablearea = walkablearea;
}

void JPSRouteFinder::SyncNavWalkablearea()
{
    // FIXME: this is dumb, but...
    nav.Resize(walkablearea->GetWidth(), walkablearea->GetHeight());

    for (int y=0; y<walkablearea->GetHeight(); y++)
        nav.SetMapRow(y, walkablearea->GetScanLine(y));
}

bool JPSRouteFinder::CanSeeFrom(int srcx, int srcy, int dstx, int dsty, int *lastcx, int *lastcy)
{
    int last_valid_x = srcx, last_valid_y = srcy;
    bool result = true;
    if ((srcx != dstx) || (srcy != dsty))
    {
        SyncNavWalkablearea();
        result = !nav.TraceLine(srcx, srcy, dstx, dsty, last_valid_x, last_valid_y);
    }

    if (lastcx)
        *lastcx = last_valid_x;
    if (lastcy)
        *lastcy = last_valid_y;
    return result;
}

bool JPSRouteFinder::FindRouteJPS(int fromx, int fromy, int destx, int desty)
{
    SyncNavWalkablearea();

    path.clear();
    cpath.clear();

    if (nav.NavigateRefined(fromx, fromy, destx, desty, path, cpath) == Navigation::NAV_UNREACHABLE)
        return false;

    num_navpoints = 0;

    // new behavior: cut path if too complex rather than abort with error message
    int count = std::min<int>((int)cpath.size(), MAXNAVPOINTS);

    for (int i = 0; i<count; i++)
    {
        int x, y;
        nav.UnpackSquare(cpath[i], x, y);
        navpoints[num_navpoints++] = { x, y };
    }

    return true;
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

inline float calc_move_speed_at_angle(float speed_x, float speed_y, float xdist, float ydist)
{
    float useMoveSpeed;
    // short circuit degenerate "simple" cases
    if (xdist == 0.f || speed_x == 0.f)
    {
        useMoveSpeed = speed_y;
    }
    else if (ydist == 0.f || speed_y == 0.f)
    {
        useMoveSpeed = speed_x;
    }
    else if (speed_x == speed_y)
    {
        useMoveSpeed = speed_x;
    }
    else
    {
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

// Calculates the X and Y per game loop, for this stage of the movelist
static void calculate_move_stage(MoveList &mls, int index, float move_speed_x, float move_speed_y)
{
    // work out the x & y per move. First, opp/adj=tan, so work out the angle
    if (mls.pos[index] == mls.pos[index + 1])
    {
        mls.xpermove[index] = 0;
        mls.ypermove[index] = 0;
        return;
    }

    int ourx = mls.pos[index].X;
    int oury = mls.pos[index].Y;
    int destx = mls.pos[index + 1].X;
    int desty = mls.pos[index + 1].Y;

    // Special case for vertical and horizontal movements
    if (ourx == destx)
    {
        mls.xpermove[index] = 0;
        mls.ypermove[index] = move_speed_y;
        if (desty < oury)
            mls.ypermove[index] = -mls.ypermove[index];
        return;
    }

    if (oury == desty)
    {
        mls.xpermove[index] = move_speed_x;
        mls.ypermove[index] = 0;
        if (destx < ourx)
            mls.xpermove[index] = -mls.xpermove[index];
        return;
    }

    float xdist = abs(ourx - destx);
    float ydist = abs(oury - desty);

    float useMoveSpeed = calc_move_speed_at_angle(move_speed_x, move_speed_y, xdist, ydist);
    float angl = atan(ydist / xdist);

    // now, since new opp=hyp*sin, work out the Y step size
    float newymove = useMoveSpeed * sin(angl);

    // since adj=hyp*cos, work out X step size
    float newxmove = useMoveSpeed * cos(angl);

    // validate that the computed movement isn't larger than the set maxima
    assert(newxmove <= move_speed_x && newymove <= move_speed_y);

    if (destx < ourx)
        newxmove = -newxmove;
    if (desty < oury)
        newymove = -newymove;

    mls.xpermove[index] = newxmove;
    mls.ypermove[index] = newymove;
}

void JPSRouteFinder::RecalculateMoveSpeeds(MoveList &mls, int old_speed_x, int old_speed_y, int new_speed_x, int new_speed_y)
{
    const float old_movspeed_x = input_speed_to_move(old_speed_x);
    const float old_movspeed_y = input_speed_to_move(old_speed_y);
    const float new_movspeed_x = input_speed_to_move(new_speed_x);
    const float new_movspeed_y = input_speed_to_move(new_speed_y);
    // save current stage's step lengths, for later onpart's update
    const float old_stage_xpermove = mls.xpermove[mls.onstage];
    const float old_stage_ypermove = mls.ypermove[mls.onstage];

    for (int i = 0; (i < mls.numstage) && ((mls.xpermove[i] != 0) || (mls.ypermove[i] != 0)); ++i)
    {
        // First three cases where the speed is a plain factor, therefore
        // we may simply divide on old one and multiple on a new one
        if ((old_movspeed_x == old_movspeed_y) || // diagonal move at straight 45 degrees
            (mls.xpermove[i] == 0) || // straight vertical move
            (mls.ypermove[i] == 0))   // straight horizontal move
        {
            mls.xpermove[i] = (mls.xpermove[i] * new_movspeed_x) / old_movspeed_x;
            mls.ypermove[i] = (mls.ypermove[i] * new_movspeed_y) / old_movspeed_y;
        }
        else
        {
            // Move at angle has adjusted speed factor, which we must recalculate first
            int ourx = mls.pos[i].X;
            int oury = mls.pos[i].Y;
            int destx = mls.pos[i + 1].X;
            int desty = mls.pos[i + 1].Y;

            float xdist = abs(ourx - destx);
            float ydist = abs(oury - desty);
            float old_speed_at_angle = calc_move_speed_at_angle(old_movspeed_x, old_movspeed_y, xdist, ydist);
            float new_speed_at_angle = calc_move_speed_at_angle(new_movspeed_x, new_movspeed_y, xdist, ydist);

            mls.xpermove[i] = (mls.xpermove[i] * new_speed_at_angle) / old_speed_at_angle;
            mls.ypermove[i] = (mls.ypermove[i] * new_speed_at_angle) / old_speed_at_angle;
        }
    }

    // now adjust current passed stage fraction
    if (mls.onpart >= 0.f)
    {
        if (old_stage_xpermove != 0)
            mls.onpart = (mls.onpart * old_stage_xpermove) / mls.xpermove[mls.onstage];
        else
            mls.onpart = (mls.onpart * old_stage_ypermove) / mls.ypermove[mls.onstage];
    }
}

bool JPSRouteFinder::FindRoute(MoveList &mls, int srcx, int srcy, int dstx, int dsty,
    int move_speed_x, int move_speed_y, bool exact_dest, bool ignore_walls)
{
    num_navpoints = 0;

    if (ignore_walls || CanSeeFrom(srcx, srcy, dstx, dsty))
    {
        num_navpoints = 2;
        navpoints[0] = { srcx, srcy };
        navpoints[1] = { dstx, dsty };
    }
    else
    {
        if ((exact_dest) && (walkablearea->GetPixel(dstx, dsty) == 0))
            return false; // clicked on a wall

        FindRouteJPS(srcx, srcy, dstx, dsty);
    }

    if (!num_navpoints)
        return false;

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

    const float fspeed_x = input_speed_to_move(move_speed_x);
    const float fspeed_y = input_speed_to_move(move_speed_y);
    for (int i = 0; i < num_navpoints - 1; i++)
        calculate_move_stage(mlist, i, fspeed_x, fspeed_y);

    mlist.from = { srcx, srcy };
    mls = mlist;
    return true;
}

bool JPSRouteFinder::AddWaypointDirect(MoveList &mls, int x, int y, int move_speed_x, int move_speed_y)
{
    if (mls.numstage >= MAXNEEDSTAGES)
        return false;

    const float fspeed_x = input_speed_to_move(move_speed_x);
    const float fspeed_y = input_speed_to_move(move_speed_y);
    mls.pos[mls.numstage] = { x, y };
    calculate_move_stage(mls, mls.numstage - 1, fspeed_x, fspeed_y);
    mls.numstage++;
    return true;
}


} // namespace Engine
} // namespace AGS
