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
#ifndef __AGS_EE_MEDIA_OPENAL_H__
#define __AGS_EE_MEDIA_OPENAL_H__
#include "core/platform.h"

#if AGS_PLATFORM_OS_MACOS
    #include <OpenAL/OpenAL.h>
#else
    #include <AL/al.h>
    #include <AL/alc.h>
    // [sonneveld] disabled until I add extension detection
    // #include "alext.h"
#endif

// Prints any OpenAL errors to the log
void dump_al_errors();

#endif // __AGS_EE_MEDIA_OPENAL_H__
