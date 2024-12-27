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
#include "ac/route_finder.h"
#include "ac/route_finder_impl.h"
#include "util/memory_compat.h"

namespace AGS
{
namespace Engine
{

bool MaskRouteFinder::CanSeeFrom(int srcx, int srcy, int dstx, int dsty, int *lastcx, int *lastcy)
{
    if (!_walkablearea)
        return false;

    // convert input to the mask coords
    srcx /= _coordScale;
    srcy /= _coordScale;
    dstx /= _coordScale;
    dsty /= _coordScale;

    int last_valid_x, last_valid_y;
    bool result = CanSeeFromImpl(srcx, srcy, dstx, dsty, &last_valid_x, &last_valid_y);

    // convert output from the mask coords
    if (lastcx)
        *lastcx = last_valid_x * _coordScale;
    if (lastcy)
        *lastcy = last_valid_y * _coordScale;
    return result;
}

bool MaskRouteFinder::FindRoute(std::vector<Point> &path, int srcx, int srcy, int dstx, int dsty,
    bool exact_dest, bool ignore_walls)
{
    if (!_walkablearea)
        return false;

    // convert input to the mask coords
    srcx /= _coordScale;
    srcy /= _coordScale;
    dstx /= _coordScale;
    dsty /= _coordScale;

    if (!FindRouteImpl(path, srcx, srcy, dstx, dsty, exact_dest, ignore_walls))
        return false;
    
    // convert output from the mask coords
    for (auto &pt : path)
        pt *= _coordScale;
    return true;
}

void MaskRouteFinder::SetWalkableArea(const AGS::Common::Bitmap *walkablearea, int coord_scale)
{
    _walkablearea = walkablearea;
    assert(coord_scale > 0);
    _coordScale = std::max(1, coord_scale);
    OnSetWalkableArea();
}


namespace Pathfinding
{

std::unique_ptr<MaskRouteFinder> CreateDefaultMaskPathfinder()
{
    return std::make_unique<JPSRouteFinder>();
}

// Find route using a provided IRouteFinder, and calculate the MoveList using move speeds
bool FindRoute(MoveList &mls, IRouteFinder *finder, int srcx, int srcy, int dstx, int dsty,
    int move_speed_x, int move_speed_y, bool exact_dest, bool ignore_walls,
    const RunPathParams &run_params)
{
    std::vector<Point> path;
    if (!finder->FindRoute(path, srcx, srcy, dstx, dsty, exact_dest, ignore_walls))
        return false;

    return CalculateMoveList(mls, path, move_speed_x, move_speed_y, run_params);
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
static void calculate_move_stage(MoveList &mls, uint32_t index, float move_speed_x, float move_speed_y)
{
    // work out the x & y per move. First, opp/adj=tan, so work out the angle
    if (mls.pos[index] == mls.pos[index + 1])
    {
        mls.permove[index].X = 0.f;
        mls.permove[index].Y = 0.f;
        return;
    }

    int ourx = mls.pos[index].X;
    int oury = mls.pos[index].Y;
    int destx = mls.pos[index + 1].X;
    int desty = mls.pos[index + 1].Y;

    // Special case for vertical and horizontal movements
    if (ourx == destx)
    {
        mls.permove[index].X = 0.f;
        mls.permove[index].Y = move_speed_y;
        if (desty < oury)
            mls.permove[index].Y = -mls.permove[index].Y;
        return;
    }

    if (oury == desty)
    {
        mls.permove[index].X = move_speed_x;
        mls.permove[index].Y = 0.f;
        if (destx < ourx)
            mls.permove[index].X = -mls.permove[index].X;
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

    mls.permove[index].X = newxmove;
    mls.permove[index].Y = newymove;
}

bool CalculateMoveList(MoveList &mls, const std::vector<Point> path, int move_speed_x, int move_speed_y,
    const RunPathParams &run_params)
{
    assert(!path.empty());
    assert(move_speed_x != 0 || move_speed_y != 0);
    if (path.empty())
        return false;

    MoveList mlist;
    mlist.pos = path;
    // Ensure there are at least 2 pos elements (required for the further algorithms)
    if (mlist.pos.size() == 1)
        mlist.pos.push_back(mlist.pos[0]);
    mlist.permove.resize(path.size());

    const float fspeed_x = input_speed_to_move(move_speed_x);
    const float fspeed_y = input_speed_to_move(move_speed_y);
    for (uint32_t i = 0; i < mlist.GetNumStages() - 1; i++)
        calculate_move_stage(mlist, i, fspeed_x, fspeed_y);

    mlist.run_params = run_params;
    mlist.ResetToBegin();
    mls = mlist;
    return true;
}

bool AddWaypointDirect(MoveList &mls, int x, int y, int move_speed_x, int move_speed_y)
{
    const float fspeed_x = input_speed_to_move(move_speed_x);
    const float fspeed_y = input_speed_to_move(move_speed_y);
    mls.pos.emplace_back( x, y );
    // Ensure there are at least 2 pos elements (required for the further algorithms)
    if (mls.pos.size() == 1)
        mls.pos.push_back(mls.pos[0]);
    // Calculate new stage starting from the one before last
    calculate_move_stage(mls, mls.GetNumStages() - 2, fspeed_x, fspeed_y);
    return true;
}

void RecalculateMoveSpeeds(MoveList &mls, int old_speed_x, int old_speed_y, int new_speed_x, int new_speed_y)
{
    const float old_movspeed_x = input_speed_to_move(old_speed_x);
    const float old_movspeed_y = input_speed_to_move(old_speed_y);
    const float new_movspeed_x = input_speed_to_move(new_speed_x);
    const float new_movspeed_y = input_speed_to_move(new_speed_y);
    // save current stage's step lengths, for later onpart's update
    const float old_stage_xpermove = mls.permove[mls.onstage].X;
    const float old_stage_ypermove = mls.permove[mls.onstage].Y;

    for (uint32_t i = 0; (i < mls.GetNumStages()) && ((mls.permove[i].X != 0.f) || (mls.permove[i].Y != 0.f)); ++i)
    {
        // First three cases where the speed is a plain factor, therefore
        // we may simply divide on old one and multiple on a new one
        if ((old_movspeed_x == old_movspeed_y) || // diagonal move at straight 45 degrees
            (mls.permove[i].X == 0) || // straight vertical move
            (mls.permove[i].Y == 0))   // straight horizontal move
        {
            mls.permove[i].X = (mls.permove[i].X * new_movspeed_x) / old_movspeed_x;
            mls.permove[i].Y = (mls.permove[i].Y * new_movspeed_y) / old_movspeed_y;
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

            mls.permove[i].X = (mls.permove[i].X * new_speed_at_angle) / old_speed_at_angle;
            mls.permove[i].Y = (mls.permove[i].Y * new_speed_at_angle) / old_speed_at_angle;
        }
    }

    // now adjust current passed stage fraction
    if (mls.onpart >= 0.f)
    {
        if (old_stage_xpermove != 0)
            mls.onpart = (mls.onpart * old_stage_xpermove) / mls.permove[mls.onstage].X;
        else
            mls.onpart = (mls.onpart * old_stage_ypermove) / mls.permove[mls.onstage].Y;
    }
}

} // namespace Pathfinding

} // namespace Engine
} // namespace AGS
