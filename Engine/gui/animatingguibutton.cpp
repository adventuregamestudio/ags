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

    int anim_delay = 0;
    AnimFlowStyle anim_flow = kAnimFlow_None;
    AnimFlowDirection anim_dir_initial = kAnimDirForward;
    AnimFlowDirection anim_dir_current = kAnimDirForward;
    int anim_volume = 0;

    if (cmp_ver < kGuiSvgVersion_40020)
    {
        // Older saves, pre 4.0.0.20
        anim_delay = in->ReadInt16();
        uint16_t anim_flags = in->ReadInt16(); // was repeat (0,1)
        wait = in->ReadInt16();
        if (cmp_ver >= kGuiSvgVersion_36025)
        {
            anim_volume = in->ReadInt8();
            in->ReadInt8(); // reserved to fill int32
            in->ReadInt8();
            in->ReadInt8();
        }

        if (cmp_ver < kGuiSvgVersion_36020)
            anim_flags &= 0x1; // restrict to repeat only
        anim_flow = (anim_flags & 0x1) ? kAnimFlow_Repeat : kAnimFlow_Once;
        anim_dir_initial = static_cast<AnimFlowDirection>((anim_flags >> 2) & 0x1);
        anim_dir_current = anim_dir_initial;
    }
    else
    {
        // Newer saves, since 4.0.0.20
        anim_flow = static_cast<AnimFlowStyle>(in->ReadInt8());
        anim_dir_initial = static_cast<AnimFlowDirection>(in->ReadInt8());
        anim_dir_current = static_cast<AnimFlowDirection>(in->ReadInt8());
        in->ReadInt8(); // reserved to fill int32
        anim_delay = in->ReadInt16();
        wait = in->ReadInt16();
        anim_volume = in->ReadInt8();
        in->ReadInt8(); // reserved to fill int32
        in->ReadInt8();
        in->ReadInt8();
    }

    anim = ViewAnimateParams(anim_flow, anim_dir_initial, anim_dir_current, anim_delay, anim_volume);
}

void AnimatingGUIButton::WriteToSavegame(Stream *out) const
{
    out->WriteInt16(buttonid);
    out->WriteInt16(ongui);
    out->WriteInt16(onguibut);
    out->WriteInt16(view);
    out->WriteInt16(loop);
    out->WriteInt16(frame);
    out->WriteInt8(anim.Flow);
    out->WriteInt8(anim.InitialDirection);
    out->WriteInt8(anim.Direction);
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt16(anim.Delay);
    out->WriteInt16(wait);
    out->WriteInt8(static_cast<uint8_t>(anim.AudioVolume));
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt8(0);
    out->WriteInt8(0);
}
