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

#include "ac/point.h"
#include "ac/common.h"    // quit()
#include "util/stream.h"

using AGS::Common::Stream;

void PolyPoints::add_point(int xxx,int yyy) {
    x[numpoints] = xxx;
    y[numpoints] = yyy;
    numpoints++;

    if (numpoints >= MAXPOINTS)
        quit("too many poly points added");
}

void PolyPoints::ReadFromFile(Stream *in)
{
    in->ReadArrayOfInt32(x, MAXPOINTS);
    in->ReadArrayOfInt32(y, MAXPOINTS);
    numpoints = in->ReadInt32();
}
