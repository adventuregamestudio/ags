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
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALDEBUG_H
#define __AGS_EE_AC__GLOBALDEBUG_H

#include <cstdio>
#include "util/string.h"

AGS::Common::String GetRuntimeInfo();
void script_debug(int cmdd,int dataa);

#endif // __AGS_EE_AC__GLOBALDEBUG_H
