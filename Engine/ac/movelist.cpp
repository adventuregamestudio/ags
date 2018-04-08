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
#include "ac/common.h"
#include "util/stream.h"

using AGS::Common::Stream;

void MoveList::ReadFromFile_Legacy(Stream *in)
{
    in->ReadArrayOfInt32(pos, MAXNEEDSTAGES_LEGACY);
    numstage = in->ReadInt32();
    in->ReadArrayOfInt32(xpermove, MAXNEEDSTAGES_LEGACY);
    in->ReadArrayOfInt32(ypermove, MAXNEEDSTAGES_LEGACY);
    fromx = in->ReadInt32();
    fromy = in->ReadInt32();
    onstage = in->ReadInt32();
    onpart = in->ReadInt32();
    lastx = in->ReadInt32();
    lasty = in->ReadInt32();
    doneflag = in->ReadInt8();
    direct = in->ReadInt8();
}

void MoveList::ReadFromFile(Stream *in)
{
    numstage = in->ReadInt32();

    if (numstage > MAXNEEDSTAGES)
        quit("attempt to read a movelist with too many stages");

    fromx = in->ReadInt32();
    fromy = in->ReadInt32();
    onstage = in->ReadInt32();
    onpart = in->ReadInt32();
    lastx = in->ReadInt32();
    lasty = in->ReadInt32();
    doneflag = in->ReadInt8();
    direct = in->ReadInt8();

    in->ReadArrayOfInt32(pos, numstage);
    in->ReadArrayOfInt32(xpermove, numstage);
    in->ReadArrayOfInt32(ypermove, numstage);
}

void MoveList::WriteToFile(Stream *out)
{
    out->WriteInt32(numstage);
    out->WriteInt32(fromx);
    out->WriteInt32(fromy);
    out->WriteInt32(onstage);
    out->WriteInt32(onpart);
    out->WriteInt32(lastx);
    out->WriteInt32(lasty);
    out->WriteInt8(doneflag);
    out->WriteInt8(direct);

    out->WriteArrayOfInt32(pos, numstage);
    out->WriteArrayOfInt32(xpermove, numstage);
    out->WriteArrayOfInt32(ypermove, numstage);
}
