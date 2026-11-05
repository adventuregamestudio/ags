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
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

float MoveList::GetStepLength() const
{
    assert(GetNumStages() > 0);
    const float permove_x = permove[onstage].X;
    const float permove_y = permove[onstage].Y;
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

void MoveList::Complete()
{
    switch (run_params.Flow)
    {
    case kAnimFlow_Once:
    case kAnimFlow_OnceAndBack:
        from = run_params.IsForward() ? pos[GetNumStages() - 1] : pos[0];
        break;
    case kAnimFlow_OnceAndReset:
        from = run_params.IsForward() ? pos[0] : pos[GetNumStages() - 1];
        break;
    }

    curpos = from;
    doneflag = 1;
}

void MoveList::ResetToBegin()
{
    // For backwards direction: set stage to the one before last,
    // because it's the stage which contains move speeds between these two
    ResetToStage(run_params.IsForward() ? 0 : GetNumStages() - 2, 0.f);
}

void MoveList::ResetToEnd()
{
    const int to_stage = run_params.IsForward() ? GetNumStages() - 2 : 0;
    // We need to calculate what is the part just prior to ending the stage
    ResetToStage(to_stage, 1.f);
}

void MoveList::ResetToStage(int stage, float progress)
{
    // NOTE: we clamp the stage by the one before last,
    // because it's the stage which contains move speeds between these two
    onstage = Math::Clamp<int>(stage, 0, GetNumStages() - 2);
    onpart = CalcPartsFromProgress(onstage, Math::Clamp(progress, 0.f, 1.f));
    from = run_params.IsForward() ? pos[onstage] : pos[onstage + 1];
    curpos = CalcCurrentPos();
}

void MoveList::SetStageDoneSteps(float parts)
{
    onpart = Math::Clamp(parts, 0.f, CalcStagePartsNum(onstage));
    curpos = CalcCurrentPos();
}

void MoveList::SetStageProgress(float progress)
{
    onpart = CalcPartsFromProgress(onstage, Math::Clamp(progress, 0.f, 1.f));
    curpos = CalcCurrentPos();
}

bool MoveList::Forward(bool do_step)
{
    // Safety fix, in case onpart became invalid
    if (std::isnan(onpart) || onpart < 0.f)
       onpart = 0.f;

    if (do_step)
    {
        onpart += 1.f;
        return OnProgressChanged(1.f);
    }
    else
    {
        // Reapply whole progress at once as a single step from 0
        return OnProgressChanged(onpart);
    }
}

bool MoveList::Backward(bool do_step)
{
    // Safety fix, in case onpart became invalid
    if (std::isnan(onpart) || onpart < 0.f)
        onpart = 0.f;

    if (do_step)
    {
        onpart -= 1.f;
        if (onpart < 0.f)
        {
            // TODO: perform smooth transition also in the case of RevertStage
            return RevertStage();
        }
        else
        {
            return OnProgressChanged(-1.f);
        }
    }
    else
    {
        // Reapply whole progress at once as a single step from 0
        return OnProgressChanged(onpart);
    }
}

bool MoveList::NextStage()
{
    run_params.IsForward() ? onstage++ : onstage--;
    if (((onstage < 0) || (onstage >= GetNumStages() - 1))
        && OnPathCompleted())
    {
        Complete();
        return false;
    }
    else
    {
        onpart = 0.f;
        doneflag = 0;
        from = run_params.IsForward() ? pos[onstage] : pos[onstage + 1];
        curpos = from;
        return true;
    }
}

bool MoveList::RevertStage()
{
    run_params.IsForward() ? onstage-- : onstage++;
    if ((onstage < 0) || (onstage >= GetNumStages() - 1))
    {
        OnPathRevertedBack();
        return true;
    }
    else
    {
        onpart = CalcStagePartsNum(onstage);
        from = run_params.IsForward() ? pos[onstage] : pos[onstage + 1];
        // FIXME: use round to nearest here?
        curpos = CalcCurrentPos();
        return true;
    }
}

float MoveList::GetStageProgress() const
{
    return onpart / CalcPartsFromProgress(onstage, 1.f);
}

Pointf MoveList::CalcCurrentPos() const
{
    const float move_dir_factor = run_params.IsForward() ? 1.f : -1.f;
    const float xpermove = permove[onstage].X * move_dir_factor;
    const float ypermove = permove[onstage].Y * move_dir_factor;
    return Pointf(
        from.X + (xpermove * onpart),
        from.Y + (ypermove * onpart));
}

float MoveList::CalcStagePartsNum(uint32_t stage) const
{
    return CalcPartsFromProgress(stage, 1.f);
}

float MoveList::CalcPartsFromProgress(uint32_t stage, float progress) const
{
    // NOTE: the last stage has no movement, so no progress can be made
    if (stage < 0 || stage >= GetNumStages() - 1)
        return 0.f;

    // The stage lies between pos[stage] and pos[stage + 1] in either direction of the run
    const Point &stage_end = pos[stage + 1];

    progress = Math::Clamp(progress, 0.f, 1.f);
    const float xparts = (permove[stage].X != 0.f) ? std::fabs(((stage_end.X - pos[stage].X) * progress) / permove[stage].X) : 0.f;
    const float yparts = (permove[stage].Y != 0.f) ? std::fabs(((stage_end.Y - pos[stage].Y) * progress) / permove[stage].Y) : 0.f;
    return std::max(xparts, yparts);
}

bool MoveList::OnProgressChanged(float step)
{
    const int cur_stage = onstage;
    const int tar_pos_stage = run_params.IsForward() ? (onstage + 1) : (onstage);
    const float move_dir_factor = run_params.IsForward() ? 1.f : -1.f;
    const float xpermove = permove[cur_stage].X * move_dir_factor;
    const float ypermove = permove[cur_stage].Y * move_dir_factor;
    const Point target = pos[tar_pos_stage];

    // Calculate next positions
    const float max_xps = from.X + (xpermove * onpart);
    const float max_yps = from.Y + (ypermove * onpart);
    const float old_xps = from.X + (xpermove * (onpart - step)), old_yps = from.Y + (ypermove * (onpart - step));

    // FIXME: use round to nearest here?
    int xps = static_cast<int>(max_xps), yps = static_cast<int>(max_yps);
    // Check if finished either horizontal or vertical movement;
    // snap to the target (in case run over)
    if (((xpermove > 0) && (xps >= target.X)) || ((xpermove < 0) && (xps <= target.X)))
        xps = target.X;
    if (((ypermove > 0) && (yps >= target.Y)) || ((ypermove < 0) && (yps <= target.Y)))
        yps = target.Y;

    // NOTE: since we're using floats now, let's assume that xps and yps will
    // reach target with proper timing, in accordance to their speeds.
    // If something goes wrong on big distances with very sharp angles,
    // we may restore one of the old fixups, like, snap at a remaining 1 px.

    // Handle end of move stage
    if (xps == target.X && yps == target.Y)
    {
        // this stage is done, go on to the next stage, or stop
        if (NextStage())
        {
            // Smooth out next stage transition by carrying over remaining % of the previous move
            const float frac_complete_x = (max_xps != old_xps) ? (xps - old_xps) / (max_xps - old_xps) : 1.f;
            const float frac_complete_y = (max_yps != old_yps) ? (yps - old_yps) / (max_yps - old_yps) : 1.f;
            const float lost_frac = std::max(1.f - frac_complete_x, 1.f - frac_complete_y);
            onpart += lost_frac;
            return true; // proceed to the next stage
        }
        else
        {
            return false; // end of path
        }
    }
    else
    {
        curpos = Point(xps, yps);
        return true;
    }
}

bool MoveList::OnPathCompleted()
{
    switch (run_params.Flow)
    {
    case kAnimFlow_OnceAndReset:
        // Reset to begin and stop
        ResetToBegin();
        return true;
    case kAnimFlow_OnceAndBack:
        // Test if we are already back, in which case stop
        if (run_params.Direction != run_params.InitialDirection)
            return true;
        // Change direction and continue
        run_params.Direction = run_params.IsForward() ? kAnimDirBackward : kAnimDirForward;
        ResetToBegin(); // reset to be sure that the position is correct
        return false;
    case kAnimFlow_Repeat:
        // Reset to begin and continue
        ResetToBegin();
        return false;
    case kAnimFlow_RepeatAlternate:
        // Change direction and continue
        run_params.Direction = run_params.IsForward() ? kAnimDirBackward : kAnimDirForward;
        ResetToBegin(); // reset to be sure that the position is correct
        return false;
    default:
        // just stop
        return true;
    }
}

void MoveList::OnPathRevertedBack()
{
    switch (run_params.Flow)
    {
    case kAnimFlow_Once:
    case kAnimFlow_OnceAndReset:
        ResetToBegin();
        break;
    case kAnimFlow_OnceAndBack:
        if (run_params.Direction == run_params.InitialDirection)
        {
            ResetToBegin();
        }
        else
        {
            ResetToBegin();
            run_params.Direction = run_params.IsForward() ? kAnimDirBackward : kAnimDirForward;
        }
        break;
    case kAnimFlow_Repeat:
        ResetToEnd();
        break;
    case kAnimFlow_RepeatAlternate:
        ResetToBegin();
        run_params.Direction = run_params.IsForward() ? kAnimDirBackward : kAnimDirForward;
        break;
    }
}

HSaveError MoveList::ReadFromSavegame(Stream *in, int32_t cmp_ver)
{
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
    onpart = in->ReadFloat32();
    in->ReadInt32(); // UNUSED
    in->ReadInt32(); // UNUSED
    doneflag = in->ReadInt8();
    const uint8_t old_direct_flag = in->ReadInt8();

    for (uint32_t i = 0; i < numstage; ++i)
    {
        pos[i].X = in->ReadInt32();
        pos[i].Y = in->ReadInt32();
    }
    for (uint32_t i = 0; i < numstage; ++i)
    {
        permove[i].X = in->ReadFloat32();
    }
    for (uint32_t i = 0; i < numstage; ++i)
    {
        permove[i].Y = in->ReadFloat32();
    }

    // Stage flags
    if ((cmp_ver >= kMoveSvgVersion_36208 && cmp_ver < kMoveSvgVersion_400) ||
        cmp_ver >= kMoveSvgVersion_40016)
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

    if (cmp_ver >= kMoveSvgVersion_40006)
    {
        AnimFlowStyle run_flow = static_cast<AnimFlowStyle>(in->ReadInt8());
        AnimFlowDirection run_dir_init = static_cast<AnimFlowDirection>(in->ReadInt8());
        // Run direction current is valid since kMoveSvgVersion_40020
        AnimFlowDirection run_dir_cur = static_cast<AnimFlowDirection>(in->ReadInt8());
        in->ReadInt8();
        in->ReadInt32(); // reserve up to 4 * int32 total
        in->ReadInt32(); // potential: from,to (waypoint range)
        in->ReadInt32();

        if (cmp_ver < kMoveSvgVersion_40020)
            run_params = RunPathParams(run_flow, run_dir_init);
        else
            run_params = RunPathParams(run_flow, run_dir_init, run_dir_cur);
    }
    else
    {
        run_params = RunPathParams(kAnimFlow_Once, kAnimDirForward);
    }

    curpos = CalcCurrentPos();
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
    out->WriteFloat32(onpart);
    out->WriteInt32(0); // UNUSED
    out->WriteInt32(0); // UNUSED
    out->WriteInt8(doneflag);
    out->WriteInt8(0); // DEPRECATED (was global direct flag)

    for (uint32_t i = 0; i < numstage; ++i)
    {
        out->WriteInt32(pos[i].X);
        out->WriteInt32(pos[i].Y);
    }
    for (uint32_t i = 0; i < numstage; ++i)
    {
        out->WriteFloat32(permove[i].X);
    }
    for (uint32_t i = 0; i < numstage; ++i)
    {
        out->WriteFloat32(permove[i].Y);
    }

    // kMoveSvgVersion_36208 || kMoveSvgVersion_40016
    for (uint32_t i = 0; i < numstage; ++i)
    {
        out->WriteInt8(stageflags[i]);
    }

    // kMoveSvgVersion_40006
    out->WriteInt8(run_params.Flow);
    out->WriteInt8(run_params.InitialDirection);
    // Run direction current is valid since kMoveSvgVersion_40020
    out->WriteInt8(run_params.Direction);
    out->WriteInt8(0);
    out->WriteInt32(0); // reserve up to 4 * int32 total
    out->WriteInt32(0); // potential: from,to (waypoint range)
    out->WriteInt32(0);
}

namespace MoveListOp
{

// Negative move speeds become fractional, e.g. -2 = 1/2 speed.
inline float InputSpeedToVelocity(float speed_val)
{
    if (speed_val < 0.f)
        return 1.f / (-speed_val);
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
    assert(index < mls.pos.size() && index < mls.permove.size() && index < mls.stageflags.size());
    if (index >= mls.pos.size())
        return;

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

static bool CalculateMoveList(MoveList &mls, const std::vector<Point> &path,
    float move_speed_x, float move_speed_y, const std::vector<Pointf> &move_speeds,
    uint8_t stage_flag, const RunPathParams &run_params)
{
    MoveList mlist;
    mlist.pos = path;
    // Ensure there are at least 2 pos elements (required for the further algorithms)
    if (mlist.pos.size() == 1)
        mlist.pos.push_back(mlist.pos[0]);
    mlist.permove.resize(path.size());
    mlist.stageflags.resize(path.size(), stage_flag);

    if (move_speeds.empty())
    {
        const float fspeed_x = InputSpeedToVelocity(move_speed_x);
        const float fspeed_y = InputSpeedToVelocity(move_speed_y);
        for (uint32_t i = 0; i < mlist.GetNumStages() - 1; i++)
            CalculateMoveStage(mlist, i, fspeed_x, fspeed_y);
    }
    else
    {
        for (uint32_t i = 0; i < mlist.GetNumStages() - 1 && i < move_speeds.size(); i++)
        {
            const float fspeed_x = InputSpeedToVelocity(move_speeds[i].X);
            const float fspeed_y = InputSpeedToVelocity(move_speeds[i].Y);
            CalculateMoveStage(mlist, i, fspeed_x, fspeed_y);
        }
    }

    mlist.run_params = run_params;
    mlist.ResetToBegin();
    mls = mlist;
    return true;
}

bool CalculateMoveList(MoveList &mls, const std::vector<Point> &path, float move_speed_x, float move_speed_y,
    uint8_t stage_flag, const RunPathParams &run_params)
{
    assert(!path.empty());
    assert(move_speed_x != 0.f || move_speed_y != 0.f);
    if (path.empty() || (move_speed_x == 0.f) && (move_speed_y == 0.f))
        return false;

    return CalculateMoveList(mls, path, move_speed_x, move_speed_y, std::vector<Pointf>(), stage_flag, run_params);
}

bool CalculateMoveList(MoveList &mls, const std::vector<Point> &path,
    const std::vector<Pointf> &move_speeds, uint8_t stage_flag, const RunPathParams &run_params)
{
    assert(!path.empty());
    assert(move_speeds.size() >= path.size() - 1);
    if (path.empty() || (move_speeds.size() < path.size() - 1))
        return false;

    return CalculateMoveList(mls, path, 0.f, 0.f, move_speeds, stage_flag, run_params);
}

bool AddWaypointDirect(MoveList &mls, int x, int y, float move_speed_x, float move_speed_y, uint8_t stage_flag)
{
    // Safety fixup, because the MoveList logic requires at least 2 points
    if (mls.GetNumStages() == 0)
    {
        mls.pos.emplace_back(x, y);
    }

    const float fspeed_x = InputSpeedToVelocity(move_speed_x);
    const float fspeed_y = InputSpeedToVelocity(move_speed_y);

    mls.pos.emplace_back( x, y );
    mls.permove.resize(mls.pos.size());
    mls.stageflags.resize(mls.pos.size(), stage_flag);
    mls.stageflags[mls.GetNumStages() - 2] = stage_flag;
    // Calculate new stage starting from the one before last
    CalculateMoveStage(mls, mls.GetNumStages() - 2, fspeed_x, fspeed_y);
    return true;
}

void RecalculateMoveSpeeds(MoveList &mls, float old_speed_x, float old_speed_y, float new_speed_x, float new_speed_y)
{
    const float old_movspeed_x = InputSpeedToVelocity(old_speed_x);
    const float old_movspeed_y = InputSpeedToVelocity(old_speed_y);
    const float new_movspeed_x = InputSpeedToVelocity(new_speed_x);
    const float new_movspeed_y = InputSpeedToVelocity(new_speed_y);
    // save current stage's step lengths, for later onpart's update
    const Pointf old_stage_permove = mls.permove[mls.onstage];

    for (uint32_t i = 0; (i < mls.GetNumStages()) && ((mls.permove[i].X != 0.f) || (mls.permove[i].Y != 0.f)); ++i)
    {
        // First three cases where the speed is a plain factor, therefore
        // we may simply divide on old one and multiple on a new one
        if ((old_movspeed_x == old_movspeed_y) && (new_movspeed_x == new_movspeed_y) // diagonal move at straight 45 degrees
            || (mls.permove[i].X == 0) // straight vertical move
            || (mls.permove[i].Y == 0)) // straight horizontal move
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
        const auto &new_stage_permove = mls.permove[mls.onstage];
        if ((old_stage_permove.X != 0.f) && (new_stage_permove.X != 0.f))
            mls.SetStageDoneSteps((mls.onpart * old_stage_permove.X) / mls.permove[mls.onstage].X);
        else if ((old_stage_permove.Y != 0.f) && (new_stage_permove.Y != 0.f))
            mls.SetStageDoneSteps((mls.onpart * old_stage_permove.Y) / mls.permove[mls.onstage].Y);
        else
            assert(false); // should never happen
    }
}

} // namespace MoveListOp
