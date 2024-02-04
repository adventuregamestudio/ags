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
#include "gui/animatingguibutton.h"
#include "gui/guidefines.h"
#include "util/stream.h"

using namespace AGS::Common;

void AnimatingGUIButton::ReadFromSavegame(Stream *in, int cmp_ver)
{
    buttonid = in->ReadInt16();
    ongui = in->ReadInt16();
    onguibut = in->ReadInt16();
    view = in->ReadInt16();
    loop = in->ReadInt16();
    frame = in->ReadInt16();
    speed = in->ReadInt16();
    uint16_t anim_flags = in->ReadInt16(); // was repeat (0,1)
    wait = in->ReadInt16();

    if (cmp_ver < kGuiSvgVersion_36020) anim_flags &= 0x1; // restrict to repeat only
    repeat = (anim_flags & 0x1) ? ANIM_REPEAT : ANIM_ONCE;
    blocking = (anim_flags >> 1) & 0x1;
    direction = (anim_flags >> 2) & 0x1;

    if (cmp_ver >= kGuiSvgVersion_36025)
    {
        volume = in->ReadInt8();
        in->ReadInt8(); // reserved to fill int32
        in->ReadInt8();
        in->ReadInt8();
    }
}

void AnimatingGUIButton::WriteToSavegame(Stream *out)
{
    uint16_t anim_flags =
        (repeat & 0x1) | // either ANIM_ONCE or ANIM_REPEAT
        (blocking & 0x1) << 1 |
        (direction & 0x1) << 2;

    out->WriteInt16(buttonid);
    out->WriteInt16(ongui);
    out->WriteInt16(onguibut);
    out->WriteInt16(view);
    out->WriteInt16(loop);
    out->WriteInt16(frame);
    out->WriteInt16(speed);
    out->WriteInt16(anim_flags); // was repeat (0,1)
    out->WriteInt16(wait);
    out->WriteInt8(static_cast<uint8_t>(volume));
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt8(0);
    out->WriteInt8(0);
}
