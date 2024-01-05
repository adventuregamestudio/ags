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
// Description of a button animation; stored separately from the GUI button.
//
//=============================================================================
#ifndef __AGS_EE_GUI__ANIMATINGGUIBUTTON_H
#define __AGS_EE_GUI__ANIMATINGGUIBUTTON_H

#include "core/types.h"
#include "ac/runtime_defines.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

struct AnimatingGUIButton
{
    // index into guibuts array, GUI, button
    short buttonid = -1, ongui = -1, onguibut = -1;
    // current animation status
    uint16_t view = 0, loop = 0, frame = 0;
    short speed = 0, repeat = 0, blocking = 0, direction = 0, wait = 0;
    // relative volume of the frame sounds
    int volume = -1;

    void ReadFromSavegame(Common::Stream *in, int cmp_ver);
    void WriteToSavegame(Common::Stream *out);
};

#endif // __AGS_EE_GUI__ANIMATINGGUIBUTTON_H
