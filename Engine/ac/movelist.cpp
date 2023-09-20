//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
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
    float permove_x = fixtof(xpermove[onstage]);
    float permove_y = fixtof(ypermove[onstage]);
    return std::sqrt(permove_x * permove_x + permove_y * permove_y);
}

float MoveList::GetPixelUnitFraction() const
{
    assert(numstage > 0);
    float distance = GetStepLength() * fixtof(onpart);
    return distance - std::floor(distance);
}

void MoveList::SetPixelUnitFraction(float frac)
{
    assert(numstage > 0);
    float permove_dist = GetStepLength();
    onpart = permove_dist > 0.f ? ftofix((1.f / permove_dist) * frac) : 0;
}

void MoveList::ReadFromFile_Legacy(Stream *in)
{
    for (int i = 0; i < MAXNEEDSTAGES_LEGACY; ++i)
    { // X & Y was packed as high/low shorts, and hence reversed in lo-end
        pos[i].Y = in->ReadInt16();
        pos[i].X = in->ReadInt16();
    }
    numstage = in->ReadInt32();
    in->ReadArrayOfInt32(xpermove, MAXNEEDSTAGES_LEGACY);
    in->ReadArrayOfInt32(ypermove, MAXNEEDSTAGES_LEGACY);
    from.X = in->ReadInt32();
    from.Y = in->ReadInt32();
    onstage = in->ReadInt32();
    onpart = itofix(in->ReadInt32());
    in->ReadInt32(); // UNUSED
    in->ReadInt32(); // UNUSED
    doneflag = in->ReadInt8();
    direct = in->ReadInt8();
}

HSaveError MoveList::ReadFromFile(Stream *in, int32_t cmp_ver)
{
    *this = MoveList();

    if (cmp_ver < 1)
    {
        ReadFromFile_Legacy(in);
        return HSaveError::None();
    }

    numstage = in->ReadInt32();
    if ((numstage == 0) && cmp_ver >= 2)
        return HSaveError::None();
    // TODO: reimplement MoveList stages as vector to avoid these limits
    if (numstage > MAXNEEDSTAGES)
    {
        return new SavegameError(kSvgErr_IncompatibleEngine,
            String::FromFormat("Incompatible number of movelist steps (count: %d, max : %d).", numstage, MAXNEEDSTAGES));
    }

    from.X = in->ReadInt32();
    from.Y = in->ReadInt32();
    onstage = in->ReadInt32();
    onpart = in->ReadInt32();
    in->ReadInt32(); // UNUSED
    in->ReadInt32();
    doneflag = in->ReadInt8();
    direct = in->ReadInt8();

    if (cmp_ver < 2)
        onpart = itofix(onpart); // convert to fixed-point value

    for (int i = 0; i < numstage; ++i)
    { // X & Y was packed as high/low shorts, and hence reversed in lo-end
        pos[i].Y = in->ReadInt16();
        pos[i].X = in->ReadInt16();
    }
    in->ReadArrayOfInt32(xpermove, numstage);
    in->ReadArrayOfInt32(ypermove, numstage);
    return HSaveError::None();
}

void MoveList::WriteToFile(Stream *out) const
{
    out->WriteInt32(numstage);
    if (numstage == 0)
        return;

    out->WriteInt32(from.X);
    out->WriteInt32(from.Y);
    out->WriteInt32(onstage);
    out->WriteInt32(onpart);
    out->WriteInt32(0); // UNUSED
    out->WriteInt32(0);
    out->WriteInt8(doneflag);
    out->WriteInt8(direct);

    for (int i = 0; i < numstage; ++i)
    { // X & Y was packed as high/low shorts, and hence reversed in lo-end
        out->WriteInt16(pos[i].Y);
        out->WriteInt16(pos[i].X);
    }
    out->WriteArrayOfInt32(xpermove, numstage);
    out->WriteArrayOfInt32(ypermove, numstage);
}
