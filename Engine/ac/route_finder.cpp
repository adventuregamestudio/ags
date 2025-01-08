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

void MaskRouteFinder::SetWalkableArea(const AGS::Common::Bitmap *walkablearea, int coord_scale)
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
        AGS::Common::Debug::Printf(AGS::Common::MessageType::kDbgMsg_Info, "Initialize path finder");
        return std::make_unique<JPSRouteFinder>();
    } 
    else 
    {
        AGS::Common::Debug::Printf(AGS::Common::MessageType::kDbgMsg_Info, "Initialize legacy path finder library");
        return std::make_unique<LegacyRouteFinder>();
    }
}

bool FindRoute(MoveList &mls, IRouteFinder *finder, int srcx, int srcy, int dstx, int dsty,
    int move_speed_x, int move_speed_y, bool exact_dest, bool ignore_walls)
{
    std::vector<Point> path;
    if (!finder->FindRoute(path, srcx, srcy, dstx, dsty, exact_dest, ignore_walls))
        return false;

    return CalculateMoveList(mls, path, move_speed_x, move_speed_y);
}

inline fixed input_speed_to_fixed(int speed_val)
{
    // negative move speeds like -2 get converted to 1/2
    if (speed_val < 0)
        return itofix(1) / (-speed_val);
    else
        return itofix(speed_val);
}

inline fixed calc_move_speed_at_angle(fixed speed_x, fixed speed_y, fixed xdist, fixed ydist)
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
void calculate_move_stage(MoveList &mls, uint32_t stage, fixed move_speed_x, fixed move_speed_y)
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

    mls.permove[stage].X = newxmove;
    mls.permove[stage].Y = newymove;

#ifdef DEBUG_PATHFINDER
  AGS::Common::Debug::Printf("stage %d from %d,%d to %d,%d Xpermove:%X Ypm:%X", stage, ourx, oury, destx, desty, newxmove, newymove);
#endif
}

bool CalculateMoveList(MoveList &mls, const std::vector<Point> path, int move_speed_x, int move_speed_y)
{
    MoveList mlist;
    mlist.pos = path;
    mlist.permove.resize(path.size());

    const fixed fix_speed_x = input_speed_to_fixed(move_speed_x);
    const fixed fix_speed_y = input_speed_to_fixed(move_speed_y);
    for (uint32_t i = 0; i < mlist.GetNumStages() - 1; i++)
        calculate_move_stage(mlist, i, fix_speed_x, fix_speed_y);

    mlist.from = mlist.pos[0];
    mls = mlist;
    return true;
}

bool AddWaypointDirect(MoveList &mls, int x, int y, int move_speed_x, int move_speed_y)
{
    const fixed fix_speed_x = input_speed_to_fixed(move_speed_x);
    const fixed fix_speed_y = input_speed_to_fixed(move_speed_y);
    const uint32_t last_stage = mls.GetNumStages() - 1;
    mls.pos.emplace_back( x, y );
    mls.permove.resize(mls.pos.size());
    calculate_move_stage(mls, last_stage, fix_speed_x, fix_speed_y);
    return true;
}

void RecalculateMoveSpeeds(MoveList &mls, int old_speed_x, int old_speed_y, int new_speed_x, int new_speed_y)
{
    const fixed old_movspeed_x = input_speed_to_fixed(old_speed_x);
    const fixed old_movspeed_y = input_speed_to_fixed(old_speed_y);
    const fixed new_movspeed_x = input_speed_to_fixed(new_speed_x);
    const fixed new_movspeed_y = input_speed_to_fixed(new_speed_y);
    // save current stage's step lengths, for later onpart's update
    const fixed old_stage_xpermove = mls.permove[mls.onstage].X;
    const fixed old_stage_ypermove = mls.permove[mls.onstage].Y;

    for (uint32_t i = 0; (i < mls.GetNumStages()) && ((mls.permove[i].X != 0.f) || (mls.permove[i].Y != 0.f)); ++i)
    {
        // First three cases where the speed is a plain factor, therefore
        // we may simply divide on old one and multiple on a new one
        if ((old_movspeed_x == old_movspeed_y) || // diagonal move at straight 45 degrees
            (mls.permove[i].X == 0) || // straight vertical move
            (mls.permove[i].Y == 0))   // straight horizontal move
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
            fixed old_speed_at_angle = calc_move_speed_at_angle(old_movspeed_x, old_movspeed_y, xdist, ydist);
            fixed new_speed_at_angle = calc_move_speed_at_angle(new_movspeed_x, new_movspeed_y, xdist, ydist);

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

} // namespace Pathfinding

} // namespace Engine
} // namespace AGS
