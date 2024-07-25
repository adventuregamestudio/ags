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
    uint32_t numstage = in->ReadInt32();
    pos.resize(numstage);
    permove.resize(numstage);
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
    direct = in->ReadInt8();

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
    out->WriteInt8(direct);

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
}
