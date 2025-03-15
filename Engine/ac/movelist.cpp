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
}

void MoveList::ResetToBegin()
{
    if (run_params.Forward)
    {
        onstage = 0;
        from = pos[onstage];
    }
    else
    {
        // For backwards direction: set stage to the one before last,
        // because it's the stage which contains move speeds between these two
        onstage = GetNumStages() - 2;
        from = pos[onstage + 1];
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
        onpart = -1.f;
        doneflag = 0;
        from = run_params.Forward ? pos[onstage] : pos[onstage + 1];
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
