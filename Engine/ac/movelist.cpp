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
    doneflag = 1;
    from = run_params.Forward ? pos[GetNumStages() - 1] : pos[0];
    curpos = from;
}

void MoveList::ResetToBegin()
{
    // For backwards direction: set stage to the one before last,
    // because it's the stage which contains move speeds between these two
    ResetToStage(run_params.Forward ? 0 : GetNumStages() - 2, 0.f);
}

void MoveList::ResetToEnd()
{
    const int to_stage = run_params.Forward ? GetNumStages() - 2 : 0;
    // We need to calculate what is the part just prior to ending the stage
    ResetToStage(to_stage, CalcLastStageProgress(to_stage));
}

void MoveList::ResetToStage(int stage, float progress)
{
    // NOTE: we clamp the stage by the one before last,
    // because it's the stage which contains move speeds between these two
    onstage = Math::Clamp<int>(stage, 0, GetNumStages() - 2);
    onpart = Math::Clamp(progress, 0.f, CalcLastStageProgress(onstage));
    from = run_params.Forward ? pos[onstage] : pos[onstage + 1];
    curpos = CalcPosFromProgress();
}

bool MoveList::Forward()
{
    onpart += 1.f;
    return OnProgressChanged();
}

bool MoveList::Backward()
{
    onpart -= 1.f;
    if (onpart < 0.f)
    {
        return RevertStage();
    }
    else
    {
        return OnProgressChanged();
    }
}

bool MoveList::NextStage()
{
    run_params.Forward ? onstage++ : onstage--;
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
        from = run_params.Forward ? pos[onstage] : pos[onstage + 1];
        curpos = from;
        return true;
    }
}

bool MoveList::RevertStage()
{
    run_params.Forward ? onstage-- : onstage++;
    if ((onstage < 0) || (onstage >= GetNumStages() - 1))
    {
        OnPathRevertedBack();
        return true;
    }
    else
    {
        onpart = CalcLastStageProgress(onstage);
        from = run_params.Forward ? pos[onstage] : pos[onstage + 1];
        // FIXME: use round to nearest here?
        curpos = CalcPosFromProgress();
        return true;
    }
}

Pointf MoveList::CalcPosFromProgress()
{
    const float move_dir_factor = run_params.Forward ? 1.f : -1.f;
    const float xpermove = permove[onstage].X * move_dir_factor;
    const float ypermove = permove[onstage].Y * move_dir_factor;
    return Pointf(
        from.X + (xpermove * onpart),
        from.Y + (ypermove * onpart));
}

float MoveList::CalcLastStageProgress(int stage)
{
    const int tar_pos_stage = run_params.Forward ? (stage + 1) : (stage);
    const float move_dir_factor = run_params.Forward ? 1.f : -1.f;
    const Point target = pos[tar_pos_stage];

    const float xparts = (permove[stage].X != 0.f) ? std::abs(target.X - pos[stage].X) / permove[stage].X : 0.f;
    const float yparts = (permove[stage].Y != 0.f) ? std::abs(target.Y - pos[stage].Y) / permove[stage].Y : 0.f;
    return std::max(xparts, yparts);
}

bool MoveList::OnProgressChanged()
{
    const int cur_stage = onstage;
    const int tar_pos_stage = run_params.Forward ? (onstage + 1) : (onstage);
    const float move_dir_factor = run_params.Forward ? 1.f : -1.f;
    const float xpermove = permove[cur_stage].X * move_dir_factor;
    const float ypermove = permove[cur_stage].Y * move_dir_factor;
    const Point target = pos[tar_pos_stage];

    // Calculate next positions
    // FIXME: use round to nearest here?
    int xps = from.X + (int)(xpermove * onpart);
    int yps = from.Y + (int)(ypermove * onpart);

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
            return true;
        }
        else
        {
            curpos = {};
            return false;
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
    if (run_params.Repeat == ANIM_ONCE)
        return true;

    ResetToBegin();
    return false;
}

void MoveList::OnPathRevertedBack()
{
    if (run_params.Repeat == ANIM_ONCE)
    {
        ResetToBegin();
    }
    else
    {
        ResetToEnd();
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

    if (cmp_ver >= kMoveSvgVersion_40006)
    {
        run_params.Repeat = in->ReadInt8();
        run_params.Forward = in->ReadInt8() == 0; // inverse, fw == 0
        in->ReadInt8();
        in->ReadInt8();
        in->ReadInt32(); // reserve up to 4 * int32 total
        in->ReadInt32(); // potential: from,to (waypoint range)
        in->ReadInt32();
    }

    OnProgressChanged();

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
    // kMoveSvgVersion_36208
    for (uint32_t i = 0; i < numstage; ++i)
    {
        out->WriteInt8(stageflags[i]);
    }

    // kMoveSvgVersion_40006
    out->WriteInt8(run_params.Repeat);
    out->WriteInt8(!run_params.Forward); // inverse, fw == 0
    out->WriteInt8(0);
    out->WriteInt8(0);
    out->WriteInt32(0); // reserve up to 4 * int32 total
    out->WriteInt32(0); // potential: from,to (waypoint range)
    out->WriteInt32(0);
}
