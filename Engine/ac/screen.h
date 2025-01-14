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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SCREEN_H
#define __AGS_EE_AC__SCREEN_H

namespace AGS { namespace Common { class Bitmap; } }
namespace AGS { namespace Engine { class IDriverDependantBitmap; } }

void fadein_impl(PALETTE p, int speed);
void fadeout_impl(int spdd);
void current_fade_out_effect ();
AGS::Engine::IDriverDependantBitmap* prepare_screen_for_transition_in(bool opaque);

// Screenshot made in the last room, used during some of the transition effects
extern AGS::Common::Bitmap *saved_viewport_bitmap;

#endif // __AGS_EE_AC__SCREEN_H
