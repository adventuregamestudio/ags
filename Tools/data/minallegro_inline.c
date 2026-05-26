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


// similar to own allegro's inline.c
// but minimal so it doesn't pull all allegro's inline functions
// the idea here is to do minimal inline when building without optimizations (e.g. Debug)

#define AL_INLINE(type, name, args, code)    type name args code

#include "allegro/color.h"

// unless we define ALLEGRO_DOS, color.inl needs a symbol for set_color from gfx.c
// we don't use set_color at all in spritepak which is the reason we are writing this.
// we fix the linker error with symbol not found for _set_color in libtools by having a dummy implementation

void set_color(const int idx, AL_CONST RGB * p)
{
    // this should neve be called
    (void) idx;
    (void) p;
}
