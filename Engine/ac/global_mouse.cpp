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
#include "ac/global_mouse.h"
#include "ac/gamestate.h"

void HideMouseCursor () {
    play.mouse_cursor_hidden = 1;
}

void ShowMouseCursor () {
    play.mouse_cursor_hidden = 0;
}
