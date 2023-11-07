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
#ifndef __AGS_EE_PLATFORM__WIN_EXCEPTION_HANDLING_H
#define __AGS_EE_PLATFORM__WIN_EXCEPTION_HANDLING_H

#include "util/ini_util.h"

#if !AGS_PLATFORM_WINDOWS_MINGW
void setup_malloc_handling();
int  initialize_engine_with_exception_handling(
    int (initialize_engine)(const AGS::Common::ConfigTree &startup_opts),
    const AGS::Common::ConfigTree &startup_opts);
#endif

#endif // __AGS_EE_PLATFORM__WIN_EXCEPTION_HANDLING_H
