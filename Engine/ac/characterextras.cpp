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
#include "ac/characterextras.h"
#include "ac/characterinfo.h"
#include "ac/viewframe.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

void CharacterExtras::UpdateGraphicSpace(const CharacterInfo *chin)
{
    _gs = GraphicSpace(
        chin->x - width / 2 + chin->pic_xoffs * zoom_offs / 100,
        chin->y - height    - (chin->z + chin->pic_yoffs) * zoom_offs / 100,
        spr_width, spr_height, width, height, rotation);
}

int CharacterExtras::GetEffectiveY(CharacterInfo *chi) const
{
    return chi->y - (chi->z * zoom_offs) / 100;
}

int CharacterExtras::GetFrameSoundVolume(CharacterInfo *chi) const
{
    return ::CalcFrameSoundVolume(
        anim_volume, cur_anim_volume,
        (chi->flags & CHF_SCALEVOLUME) ? zoom : 100);
}

void CharacterExtras::CheckViewFrame(CharacterInfo *chi)
{
    ::CheckViewFrame(chi->view, chi->loop, chi->frame, GetFrameSoundVolume(chi));
}

void CharacterExtras::SetFollowing(CharacterInfo *chi, int follow_who, int distance, int eagerness, bool sort_behind)
{
    if (follow_who < 0)
    {
        following = -1;
        follow_dist = 0;
        follow_eagerness = 0;
        chi->set_following_sortbehind(false);
    }
    else
    {
        following = follow_who;
        follow_dist = distance;
        follow_eagerness = (distance == FOLLOW_ALWAYSONTOP) ? 0 : eagerness;
        chi->set_following_sortbehind(sort_behind && (distance == FOLLOW_ALWAYSONTOP));
    }
}

void CharacterExtras::ReadFromSavegame(Stream *in, CharacterSvgVersion save_ver)
{
    in->ReadArrayOfInt16(invorder, MAX_INVORDER);
    invorder_count = in->ReadInt16();
    width = in->ReadInt16();
    height = in->ReadInt16();
    zoom = in->ReadInt16();
    xwas = in->ReadInt16();
    ywas = in->ReadInt16();
    tint_r = in->ReadInt16();
    tint_g = in->ReadInt16();
    tint_b = in->ReadInt16();
    tint_level = in->ReadInt16();
    tint_light = in->ReadInt16();
    process_idle_this_time = in->ReadInt8();
    slow_move_counter = in->ReadInt8();
    animwait = in->ReadInt16();
    if (save_ver >= kCharSvgVersion_36025)
    {
        anim_volume = static_cast<uint8_t>(in->ReadInt8());
        cur_anim_volume = static_cast<uint8_t>(in->ReadInt8());
        in->ReadInt8(); // reserved to fill int32
        in->ReadInt8();
    }
    if (save_ver >= kCharSvgVersion_36205)
    {
        following = in->ReadInt32();
        follow_dist = in->ReadInt32();
        follow_eagerness = in->ReadInt32();
    }
    if (save_ver >= kCharSvgVersion_400)
    {
        blend_mode = (BlendMode)in->ReadInt32();
        // Reserved for colour options
        in->ReadInt32(); // colour flags
        // Reserved for transform options
        in->ReadInt32(); // sprite transform flags1
        in->ReadInt32(); // sprite transform flags2
        in->ReadInt32(); // transform scale x
        in->ReadInt32(); // transform scale y
        in->ReadInt32(); // transform skew x
        in->ReadInt32(); // transform skew y
        rotation = in->ReadFloat32(); // transform rotate
        in->ReadInt32(); // sprite pivot x
        in->ReadInt32(); // sprite pivot y
        in->ReadInt32(); // sprite anchor x
        in->ReadInt32(); // sprite anchor y
    }
    else
    {
        blend_mode = kBlend_Normal;
    }
    if (save_ver >= kCharSvgVersion_400_03)
    {
        face_dir_ratio = in->ReadFloat32();
        // Reserve for 4 total int32
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
    }
}

void CharacterExtras::WriteToSavegame(Stream *out) const
{
    out->WriteArrayOfInt16(invorder, MAX_INVORDER);
    out->WriteInt16(invorder_count);
    out->WriteInt16(width);
    out->WriteInt16(height);
    out->WriteInt16(zoom);
    out->WriteInt16(xwas);
    out->WriteInt16(ywas);
    out->WriteInt16(tint_r);
    out->WriteInt16(tint_g);
    out->WriteInt16(tint_b);
    out->WriteInt16(tint_level);
    out->WriteInt16(tint_light);
    out->WriteInt8(process_idle_this_time);
    out->WriteInt8(slow_move_counter);
    out->WriteInt16(animwait);
    // kCharSvgVersion_36025
    out->WriteInt8(static_cast<uint8_t>(anim_volume));
    out->WriteInt8(static_cast<uint8_t>(cur_anim_volume));
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt8(0);
    // kCharSvgVersion_36205
    out->WriteInt32(following);
    out->WriteInt32(follow_dist);
    out->WriteInt32(follow_eagerness);
    // kCharSvgVersion_400
    out->WriteInt32(blend_mode);
    // Reserved for colour options
    out->WriteInt32(0); // colour flags
    // Reserved for transform options
    out->WriteInt32(0); // sprite transform flags1
    out->WriteInt32(0); // sprite transform flags2
    out->WriteInt32(0); // transform scale x
    out->WriteInt32(0); // transform scale y
    out->WriteInt32(0); // transform skew x
    out->WriteInt32(0); // transform skew y
    out->WriteFloat32(rotation); // transform rotate
    out->WriteInt32(0); // sprite pivot x
    out->WriteInt32(0); // sprite pivot y
    out->WriteInt32(0); // sprite anchor x
    out->WriteInt32(0); // sprite anchor y
    // -- kCharSvgVersion_400_03
    out->WriteFloat32(face_dir_ratio);
    // Reserve for 4 total int32
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
}
