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
//
// Deprecated interactions stuff.
// Perhaps move to some legacy knowledge base; or restore when it's possible to
// support ancient games.
//
//=============================================================================

#if defined (OBSOLETE)

#include "game/interactions_deprecated.h"
#include "util/stream.h"

using namespace AGS::Common;

void freadmissout(short *pptr, Stream *in)
{
    in->ReadArrayOfInt16(&pptr[0], 5);
    in->ReadArrayOfInt16(&pptr[7], NUM_CONDIT - 7);
    pptr[5] = pptr[6] = 0;
}

#endif // OBSOLETE
