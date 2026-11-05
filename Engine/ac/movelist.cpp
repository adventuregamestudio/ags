//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/movelist.h"
#include <cmath>
#include "ac/common.h"
#include "util/bbop.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

float MoveList::GetStepLength() const
{
    assert(GetNumStages() > 0);
    float permove_x = fixtof(permove[onstage].X);
    float permove_y = fixtof(permove[onstage].Y);
    return std::sqrt(permove_x * permove_x + permove_y * permove_y);
}

float MoveList::GetPixelUnitFraction() const
{
    assert(GetNumStages() > 0);
    const float distance = GetStepLength() * onpart;
    return distance - std::floor(distance);
}

void MoveList::SetPixelUnitFraction(float frac)
{
    assert(GetNumStages() > 0);
    const float permove_dist = GetStepLength();
    onpart = permove_dist > 0.f ? (1.f / permove_dist) * frac : 0.f;
}

HSaveError MoveList::ReadFromSavegame(Stream *in, int32_t cmp_ver)
{
    if (cmp_ver < kMoveSvgVersion_350)
    {
        return new SavegameError(kSvgErr_UnsupportedComponentVersion,
            String::FromFormat("Movelist format %d is no longer supported", cmp_ver));
    }

    *this = MoveList(); // reset struct
    const uint32_t numstage = in->ReadInt32();
    pos.resize(numstage);
    permove.resize(numstage);
    stageflags.resize(numstage);
    if ((numstage == 0) && cmp_ver >= kMoveSvgVersion_36109)
    {
        return HSaveError::None();
    }

    from.X = in->ReadInt32();
    from.Y = in->ReadInt32();
    onstage = in->ReadInt32();
    BBOp::IntFloatSwap onpart_u(in->ReadInt32());
    in->ReadInt32(); // UNUSED
    in->ReadInt32(); // UNUSED
    doneflag = in->ReadInt8();
    const uint8_t old_direct_flag = in->ReadInt8();

    for (uint32_t i = 0; i < numstage; ++i)
    { // X & Y was packed as high/low shorts, and hence reversed in lo-end
        pos[i].Y = in->ReadInt16();
        pos[i].X = in->ReadInt16();
    }
    for (uint32_t i = 0; i < numstage; ++i)
    {
        permove[i].X = in->ReadInt32();
    }
    for (uint32_t i = 0; i < numstage; ++i)
    {
        permove[i].Y = in->ReadInt32();
    }

    // Some variables require conversion depending on a save version
    if (cmp_ver < kMoveSvgVersion_36109)
        onpart = static_cast<float>(onpart_u.val.i32);
    else
        onpart = onpart_u.val.f;

    if (cmp_ver >= kMoveSvgVersion_36208)
    {
        for (uint32_t i = 0; i < numstage; ++i)
        {
            stageflags[i] = in->ReadInt8();
        }
    }
    else
    {
        std::fill(stageflags.begin(), stageflags.end(), old_direct_flag);
    }

    return HSaveError::None();
}

void MoveList::WriteToSavegame(Stream *out) const
{
    const uint32_t numstage = GetNumStages();
    out->WriteInt32(numstage);
    if (numstage == 0)
        return;

    out->WriteInt32(from.X);
    out->WriteInt32(from.Y);
    out->WriteInt32(onstage);
    out->WriteInt32(BBOp::IntFloatSwap(onpart).val.i32);
    out->WriteInt32(0); // UNUSED
    out->WriteInt32(0); // UNUSED
    out->WriteInt8(doneflag);
    out->WriteInt8(0); // DEPRECATED (was global direct flag)

    for (uint32_t i = 0; i < numstage; ++i)
    { // X & Y was packed as high/low shorts, and hence reversed in lo-end
        out->WriteInt16(pos[i].Y);
        out->WriteInt16(pos[i].X);
    }
    for (uint32_t i = 0; i < numstage; ++i)
    {
        out->WriteInt32(permove[i].X);
    }
    for (uint32_t i = 0; i < numstage; ++i)
    {
        out->WriteInt32(permove[i].Y);
    }
    // kMoveSvgVersion_36208
    for (uint32_t i = 0; i < numstage; ++i)
    {
        out->WriteInt8(stageflags[i]);
    }
}

namespace MoveListOp
{

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
    assert(stage < mls.pos.size() && stage < mls.permove.size() && stage < mls.stageflags.size());
    if (stage >= mls.pos.size())
        return;

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
    const Point old_stage_permove = mls.permove[mls.onstage];

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
        const auto &new_stage_permove = mls.permove[mls.onstage];
        if ((old_stage_permove.X != 0) && (new_stage_permove.X != 0))
            mls.onpart = (mls.onpart * fixtof(old_stage_permove.X)) / fixtof(new_stage_permove.X);
        else if ((old_stage_permove.Y != 0) && (new_stage_permove.Y != 0))
            mls.onpart = (mls.onpart * fixtof(old_stage_permove.Y)) / fixtof(new_stage_permove.Y);
        else
            assert(false); // should never happen
    }
}

} // namespace MoveListOp
