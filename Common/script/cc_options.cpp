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

#include "cc_options.h"

int ccCompOptions = 0;

void ccSetOption(int optbit, int onoroff)
{
    if (onoroff)
        ccCompOptions |= optbit;
    else
        ccCompOptions &= ~optbit;
}

int ccGetOption(int optbit)
{
    return 0 != (ccCompOptions & optbit);
}
