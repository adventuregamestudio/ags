//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
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
    assert(numstage > 0);
    float permove_x = xpermove[onstage];
    float permove_y = ypermove[onstage];
    return std::sqrt(permove_x * permove_x + permove_y * permove_y);
}

float MoveList::GetPixelUnitFraction() const
{
    assert(numstage > 0);
    float distance = GetStepLength() * onpart;
    return distance - std::floor(distance);
}

void MoveList::SetPixelUnitFraction(float frac)
{
    assert(numstage > 0);
    float permove_dist = GetStepLength();
    onpart = permove_dist > 0.f ? (1.f / permove_dist) * frac : 0.f;
}

HSaveError MoveList::ReadFromSavegame(Stream *in, int32_t cmp_ver)
{
    *this = MoveList(); // reset struct
    numstage = in->ReadInt32();
    if (numstage == 0)
    {
        return HSaveError::None();
    }
    // TODO: reimplement MoveList stages as vector to avoid these limits
    if (numstage > MAXNEEDSTAGES)
    {
        return new SavegameError(kSvgErr_IncompatibleEngine,
            String::FromFormat("Incompatible number of movelist steps (count: %d, max : %d).", numstage, MAXNEEDSTAGES));
    }

    from.X = in->ReadInt32();
    from.Y = in->ReadInt32();
    onstage = in->ReadInt32();
    onpart = in->ReadFloat32();
    in->ReadInt32(); // UNUSED
    in->ReadInt32(); // UNUSED
    doneflag = in->ReadInt8();
    direct = in->ReadInt8();

    for (int i = 0; i < numstage; ++i)
    {
        pos[i].X = in->ReadInt32();
        pos[i].Y = in->ReadInt32();
    }
    in->ReadArrayOfFloat32(xpermove, numstage);
    in->ReadArrayOfFloat32(ypermove, numstage);
    return HSaveError::None();
}

void MoveList::WriteToSavegame(Stream *out) const
{
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
    out->WriteInt8(direct);

    for (int i = 0; i < numstage; ++i)
    {
        out->WriteInt32(pos[i].X);
        out->WriteInt32(pos[i].Y);
    }
    out->WriteArrayOfFloat32(xpermove, numstage);
    out->WriteArrayOfFloat32(ypermove, numstage);
}
