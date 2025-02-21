//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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
#include "gfx/bitmap.h"
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

bool MaskRouteFinder::IsWalkableAt(int x, int y)
{
    if (!_walkablearea)
        return false;

    return _walkablearea->GetPixel(x / _coordScale, y / _coordScale) > 0;
}

bool MaskRouteFinder::FindNearestWalkablePoint(const Point &from_pt, Point &dst_pt)
{
    if (!_walkablearea)
        return false;

    bool result = Pathfinding::FindNearestWalkablePoint(_walkablearea, from_pt / _coordScale, dst_pt);
    dst_pt *= _coordScale;
    return result;
}

void MaskRouteFinder::SetWalkableArea(const Bitmap *walkablearea, int coord_scale)
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

// Negative move speeds become fractional, e.g. -2 = 1/2 speed.
inline float InputSpeedToVelocity(int speed_val)
{
    if (speed_val < 0) {
        return 1.f / (-speed_val);
    }
    else
        return speed_val;
}

// Calculates velocity vector length from x/y speed components
inline float CalcMoveSpeedAtAngle(float speed_x, float speed_y, float xdist, float ydist)
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
static void CalculateMoveStage(MoveList &mls, uint32_t index, float move_speed_x, float move_speed_y)
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

    float useMoveSpeed = CalcMoveSpeedAtAngle(move_speed_x, move_speed_y, xdist, ydist);
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

    const float fspeed_x = InputSpeedToVelocity(move_speed_x);
    const float fspeed_y = InputSpeedToVelocity(move_speed_y);
    for (uint32_t i = 0; i < mlist.GetNumStages() - 1; i++)
        CalculateMoveStage(mlist, i, fspeed_x, fspeed_y);

    mlist.run_params = run_params;
    mlist.ResetToBegin();
    mls = mlist;
    return true;
}

bool AddWaypointDirect(MoveList &mls, int x, int y, int move_speed_x, int move_speed_y)
{
    const float fspeed_x = InputSpeedToVelocity(move_speed_x);
    const float fspeed_y = InputSpeedToVelocity(move_speed_y);
    mls.pos.emplace_back( x, y );
    // Ensure there are at least 2 pos elements (required for the further algorithms)
    if (mls.pos.size() == 1)
        mls.pos.push_back(mls.pos[0]);
    // Calculate new stage starting from the one before last
    CalculateMoveStage(mls, mls.GetNumStages() - 2, fspeed_x, fspeed_y);
    return true;
}

void RecalculateMoveSpeeds(MoveList &mls, int old_speed_x, int old_speed_y, int new_speed_x, int new_speed_y)
{
    const float old_movspeed_x = InputSpeedToVelocity(old_speed_x);
    const float old_movspeed_y = InputSpeedToVelocity(old_speed_y);
    const float new_movspeed_x = InputSpeedToVelocity(new_speed_x);
    const float new_movspeed_y = InputSpeedToVelocity(new_speed_y);
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
            float old_speed_at_angle = CalcMoveSpeedAtAngle(old_movspeed_x, old_movspeed_y, xdist, ydist);
            float new_speed_at_angle = CalcMoveSpeedAtAngle(new_movspeed_x, new_movspeed_y, xdist, ydist);

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

bool FindNearestWalkablePoint(const Bitmap *mask, const Point &from_pt, Point &dst_pt,
    const int range, const int step)
{
    return FindNearestWalkablePoint(mask, from_pt, dst_pt, RectWH(mask->GetSize()), range, step);
}

bool FindNearestWalkablePoint(const Bitmap *mask, const Point &from_pt, Point &dst_pt,
    const Rect &limits, const int range, const int step)
{
    assert(mask->GetColorDepth() == 8);
    const Rect mask_limits = IntersectRects(limits, RectWH(mask->GetSize()));
    const Rect use_limits = (range <= 0) ? mask_limits :
        IntersectRects(mask_limits, RectWH(from_pt.X - range / 2, from_pt.Y - range / 2, range * 2, range * 2));

    if (use_limits.IsEmpty())
        return false;

    // Quickly test if starting point is already on a walkable pixel
    if (use_limits.IsInside(from_pt) && mask->GetPixel(from_pt.X, from_pt.Y) > 0)
    {
        dst_pt = from_pt;
        return true;
    }

    // Scan outwards from the starting point, rectangle by rectangle,
    // and measure distance between start and found point. The one with the smallest
    // distance will be accepted as a result.
    // As soon as we find the first walkable point, we limit the max scan range
    // to its distance, because we won't need anything beyond that.
    // We store distances as "squared distance" in order to avoid sqrt() call in a loop
    uint64_t nearest_sqdist = UINT64_MAX;
    Point nearest_pt(-1, -1);
    const int mask_line_len = mask->GetLineLength();
    const uint8_t *mask_ptr = mask->GetData();

    int max_range = std::max(
        std::max(from_pt.X - use_limits.Left, use_limits.Right - from_pt.X),
        std::max(from_pt.Y - use_limits.Top, use_limits.Bottom - from_pt.Y));
    for (int cur_range = 1; cur_range <= max_range; cur_range += step)
    {
        const int scan_fromx = std::max(use_limits.Left,   from_pt.X - cur_range);
        const int scan_tox   = std::min(use_limits.Right,  from_pt.X + cur_range);
        const int scan_fromy = std::max(use_limits.Top,    from_pt.Y - cur_range);
        const int scan_toy   = std::min(use_limits.Bottom, from_pt.Y + cur_range);
        // X outer and Y internal loop was a historical order of search,
        // kept for backwards compatibility (this is not too important, but just in case)
        for (int x = scan_fromx; x <= scan_tox; x += step)
        {
            const int y_step = (x == scan_fromx || x == scan_tox) ? step : cur_range * 2;
            for (int y = scan_fromy; y <= scan_toy; y += y_step)
            {
                if (mask_ptr[y * mask_line_len + x] == 0)
                    continue;

                uint64_t sqdist = (x - from_pt.X) * (x - from_pt.X) + (y - from_pt.Y) * (y - from_pt.Y);
                if (sqdist < nearest_sqdist)
                {
                    max_range = std::sqrt(sqdist);
                    nearest_sqdist = sqdist;
                    nearest_pt = Point(x, y);
                }
            }
        }
    }

    if (nearest_sqdist < UINT64_MAX)
    {
        dst_pt = nearest_pt;
        return true;
    }
    return false;
}

} // namespace Pathfinding

} // namespace Engine
} // namespace AGS
