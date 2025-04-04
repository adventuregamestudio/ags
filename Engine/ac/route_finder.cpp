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
#include <memory>
#include <allegro.h>
#include "ac/movelist.h"
#include "ac/route_finder_impl.h"
#include "ac/route_finder_impl_legacy.h"
#include "debug/out.h"
#include "gfx/bitmap.h"
#include "util/memory_compat.h"

using namespace AGS::Common;

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

void MaskRouteFinder::SetWalkableArea(const Bitmap *walkablearea, int coord_scale)
{
    _walkablearea = walkablearea;
    assert(coord_scale > 0);
    _coordScale = std::max(1, coord_scale);
    OnSetWalkableArea();
}


namespace Pathfinding
{

std::unique_ptr<MaskRouteFinder> CreateDefaultMaskPathfinder(GameDataVersion game_ver)
{
    if (game_ver >= kGameVersion_350) 
    {
        Debug::Printf(MessageType::kDbgMsg_Info, "Initialize path finder");
        return std::make_unique<JPSRouteFinder>();
    } 
    else 
    {
        Debug::Printf(MessageType::kDbgMsg_Info, "Initialize legacy path finder library");
        return std::make_unique<LegacyRouteFinder>();
    }
}

bool FindRoute(MoveList &mls, IRouteFinder *finder, int srcx, int srcy, int dstx, int dsty,
    int move_speed_x, int move_speed_y, bool exact_dest, bool ignore_walls)
{
    std::vector<Point> path;
    if (!finder->FindRoute(path, srcx, srcy, dstx, dsty, exact_dest, ignore_walls))
        return false;

    return CalculateMoveList(mls, path, move_speed_x, move_speed_y, ignore_walls ? kMoveStage_Direct : 0);
}

// Converts input moving speed to a fixed-point representation.
// Negative move speeds become fractional, e.g. -2 = 1/2 speed.
inline fixed InputSpeedToFixed(int speed_val)
{
    if (speed_val < 0)
        return itofix(1) / (-speed_val);
    else
        return itofix(speed_val);
}

// Calculates velocity vector length from x/y speed components
inline fixed CalcMoveSpeedAtAngle(fixed speed_x, fixed speed_y, fixed xdist, fixed ydist)
{
    fixed useMoveSpeed;
    if (speed_x == speed_y)
    {
        useMoveSpeed = speed_x;
    }
    else
    {
        // different X and Y move speeds
        // the X proportion of the movement is (x / (x + y))
        fixed xproportion = fixdiv(xdist, (xdist + ydist));

        if (speed_x > speed_y)
        {
            // speed = y + ((1 - xproportion) * (x - y))
            useMoveSpeed = speed_y + fixmul(xproportion, speed_x - speed_y);
        }
        else
        {
            // speed = x + (xproportion * (y - x))
            useMoveSpeed = speed_x + fixmul(itofix(1) - xproportion, speed_y - speed_x);
        }
    }
    return useMoveSpeed;
}

// Calculates the X and Y per game loop, for this stage of the movelist
void CalculateMoveStage(MoveList &mls, uint32_t stage, fixed move_speed_x, fixed move_speed_y)
{
    // work out the x & y per move. First, opp/adj=tan, so work out the angle
    if (mls.pos[stage] == mls.pos[stage + 1])
    {
        mls.permove[stage].X = 0;
        mls.permove[stage].Y = 0;
        return;
    } 

    int ourx = mls.pos[stage].X;
    int oury = mls.pos[stage].Y;
    int destx = mls.pos[stage + 1].X;
    int desty = mls.pos[stage + 1].Y;

    // Special case for vertical and horizontal movements
    if (ourx == destx)
    {
        mls.permove[stage].X = 0;
        mls.permove[stage].Y = move_speed_y;
        if (desty < oury)
            mls.permove[stage].Y = -mls.permove[stage].Y;
        return;
    }

    if (oury == desty)
    {
        mls.permove[stage].X = move_speed_x;
        mls.permove[stage].Y = 0;
        if (destx < ourx)
            mls.permove[stage].X = -mls.permove[stage].X;
        return;
    }

    fixed xdist = itofix(abs(ourx - destx));
    fixed ydist = itofix(abs(oury - desty));

    fixed useMoveSpeed = CalcMoveSpeedAtAngle(move_speed_x, move_speed_y, xdist, ydist);

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

    mls.permove[stage].X = newxmove;
    mls.permove[stage].Y = newymove;

#ifdef DEBUG_PATHFINDER
  Debug::Printf("stage %d from %d,%d to %d,%d Xpermove:%X Ypm:%X", stage, ourx, oury, destx, desty, newxmove, newymove);
#endif
}

bool CalculateMoveList(MoveList &mls, const std::vector<Point> path, int move_speed_x, int move_speed_y, uint8_t stage_flag)
{
    MoveList mlist;
    mlist.pos = path;
    mlist.permove.resize(path.size());
    mlist.stageflags.resize(path.size(), stage_flag);

    const fixed fix_speed_x = InputSpeedToFixed(move_speed_x);
    const fixed fix_speed_y = InputSpeedToFixed(move_speed_y);
    for (uint32_t i = 0; i < mlist.GetNumStages() - 1; i++)
        CalculateMoveStage(mlist, i, fix_speed_x, fix_speed_y);

    mlist.from = mlist.pos[0];
    mls = mlist;
    return true;
}

bool AddWaypointDirect(MoveList &mls, int x, int y, int move_speed_x, int move_speed_y, uint8_t stage_flag)
{
    // Safety fixup, because the MoveList logic requires at least 2 points
    if (mls.GetNumStages() == 0)
    {
        mls.pos.emplace_back(x, y);
    }

    const fixed fix_speed_x = InputSpeedToFixed(move_speed_x);
    const fixed fix_speed_y = InputSpeedToFixed(move_speed_y);
    const uint32_t last_stage = mls.GetNumStages() - 1;
    mls.pos.emplace_back( x, y );
    mls.permove.resize(mls.pos.size());
    mls.stageflags.resize(mls.pos.size(), stage_flag);
    mls.stageflags[last_stage] = stage_flag;
    CalculateMoveStage(mls, last_stage, fix_speed_x, fix_speed_y);
    return true;
}

void RecalculateMoveSpeeds(MoveList &mls, int old_speed_x, int old_speed_y, int new_speed_x, int new_speed_y)
{
    const fixed old_movspeed_x = InputSpeedToFixed(old_speed_x);
    const fixed old_movspeed_y = InputSpeedToFixed(old_speed_y);
    const fixed new_movspeed_x = InputSpeedToFixed(new_speed_x);
    const fixed new_movspeed_y = InputSpeedToFixed(new_speed_y);
    // save current stage's step lengths, for later onpart's update
    const fixed old_stage_xpermove = mls.permove[mls.onstage].X;
    const fixed old_stage_ypermove = mls.permove[mls.onstage].Y;

    for (uint32_t i = 0; (i < mls.GetNumStages()) && ((mls.permove[i].X != 0.f) || (mls.permove[i].Y != 0.f)); ++i)
    {
        // First three cases where the speed is a plain factor, therefore
        // we may simply divide on old one and multiple on a new one
        if ((old_movspeed_x == old_movspeed_y) && (new_movspeed_x == new_movspeed_y) // diagonal move at straight 45 degrees
            || (mls.permove[i].X == 0) // straight vertical move
            || (mls.permove[i].Y == 0)) // straight horizontal move
        {
            mls.permove[i].X = fixdiv(fixmul(mls.permove[i].X, new_movspeed_x), old_movspeed_x);
            mls.permove[i].Y = fixdiv(fixmul(mls.permove[i].Y, new_movspeed_y), old_movspeed_y);
        }
        else
        {
            // Move at angle has adjusted speed factor, which we must recalculate first
            int ourx = mls.pos[i].X;
            int oury = mls.pos[i].Y;
            int destx = mls.pos[i + 1].X;
            int desty = mls.pos[i + 1].Y;

            fixed xdist = itofix(abs(ourx - destx));
            fixed ydist = itofix(abs(oury - desty));
            fixed old_speed_at_angle = CalcMoveSpeedAtAngle(old_movspeed_x, old_movspeed_y, xdist, ydist);
            fixed new_speed_at_angle = CalcMoveSpeedAtAngle(new_movspeed_x, new_movspeed_y, xdist, ydist);

            mls.permove[i].X = fixdiv(fixmul(mls.permove[i].X, new_speed_at_angle), old_speed_at_angle);
            mls.permove[i].Y = fixdiv(fixmul(mls.permove[i].Y, new_speed_at_angle), old_speed_at_angle);
        }
    }

    // now adjust current passed stage fraction
    if (mls.onpart >= 0.f)
    {
        if (old_stage_xpermove != 0)
            mls.onpart = (mls.onpart * fixtof(old_stage_xpermove)) / fixtof(mls.permove[mls.onstage].X);
        else
            mls.onpart = (mls.onpart * fixtof(old_stage_ypermove)) / fixtof(mls.permove[mls.onstage].Y);
    }
}

bool FindNearestWalkablePoint(Bitmap *mask, const Point &from_pt, Point &dst_pt,
    const int range, const int step)
{
    return FindNearestWalkablePoint(mask, from_pt, dst_pt, RectWH(mask->GetSize()), range, step);
}

bool FindNearestWalkablePoint(Bitmap *mask, const Point &from_pt, Point &dst_pt,
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
