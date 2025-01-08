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
//
// Screen effects and script API
//
//=============================================================================
#ifndef __AGS_EE_AC__SCREEN_H
#define __AGS_EE_AC__SCREEN_H

#include "gamestate.h"

namespace AGS { namespace Common { class Bitmap; } }
namespace AGS { namespace Engine { class IDriverDependantBitmap; } }

// Runs fade-in effect using explicit settings
void run_fade_in_effect(ScreenTransitionStyle style, int speed);
// Runs fade-out effect using explicit settings
void run_fade_out_effect(ScreenTransitionStyle style, int speed);
// Runs fade-in transition using default settings
void current_fade_in_effect();
// Runs fade-out transition using default settings
void current_fade_out_effect();

#endif // __AGS_EE_AC__SCREEN_H
