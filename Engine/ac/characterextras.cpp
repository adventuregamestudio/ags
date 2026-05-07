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
#include "ac/characterextras.h"
#include "ac/viewframe.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

int CharacterExtras::GetEffectiveY(CharacterInfo *chi) const
{
    return chi->y - (chi->z * zoom_offs) / 100;
}

int CharacterExtras::GetFrameSoundVolume(CharacterInfo *chi) const
{
    return ::CalcFrameSoundVolume(
        audio_volume, cur_audio_volume,
        (chi->flags & CHF_SCALEVOLUME) ? zoom : 100);
}

void CharacterExtras::CheckViewFrame(CharacterInfo *chi)
{
    ::CheckViewFrame(chi->view, chi->loop, chi->frame, GetFrameSoundVolume(chi), audio_panning, audio_speed);
}

void CharacterExtras::SetFollowing(CharacterInfo *chi, int follow_who, int distance, int eagerness, bool sort_behind)
{
    if (follow_who < 0)
    {
        following = -1;
        follow_dist = 0;
        follow_eagerness = 0;
    }
    else
    {
        following = follow_who;
        follow_dist = distance;
        follow_eagerness = (distance == FOLLOW_ALWAYSONTOP) ? 0 : eagerness;
    }

    // Set Character's following flags, and legacy fields, for backwards compatibility
    chi->set_following(following, follow_dist, follow_eagerness, sort_behind);
}

void CharacterExtras::ReadFromSavegame(Stream *in, CharacterSvgVersion save_ver)
{
    if (save_ver < kCharSvgVersion_36310)
    {
        int16_t invorder[LEGACY_MAX_CHAR_INVENTORY];
        in->ReadArrayOfInt16(invorder, LEGACY_MAX_CHAR_INVENTORY);
        int invorder_count = in->ReadInt16();
        inventory.resize(invorder_count);
        std::copy(invorder, invorder + invorder_count, inventory.begin());
    }
    else
    {
        uint32_t inv_count = in->ReadInt32();
        inventory.resize(inv_count);
        in->ReadArrayOfInt32(inventory.data(), inv_count);
        if (inv_count > MAX_CHAR_INVENTORY)
            inventory.resize(MAX_CHAR_INVENTORY);
    }
    
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
        audio_volume = static_cast<uint8_t>(in->ReadInt8());
        cur_audio_volume = static_cast<uint8_t>(in->ReadInt8());
        audio_panning = in->ReadInt8();
        in->ReadInt8(); // reserved
    }
    else
    {
        audio_volume = 100;
        cur_audio_volume = 100;
        audio_panning = 0;
    }

    if (save_ver >= kCharSvgVersion_36205)
    {
        following = in->ReadInt32();
        follow_dist = in->ReadInt32();
        follow_eagerness = in->ReadInt32();
    }
    else
    {
        following = -1;
        follow_dist = 0;
        follow_eagerness = 0;
    }

    if (save_ver >= kCharSvgVersion_36310)
    {
        audio_speed = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
    }
}

void CharacterExtras::WriteToSavegame(Stream *out) const
{
    out->WriteInt32(static_cast<uint32_t>(inventory.size()));
    out->WriteArrayOfInt32(inventory.data(), inventory.size());
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
    out->WriteInt8(static_cast<uint8_t>(audio_volume));
    out->WriteInt8(static_cast<uint8_t>(cur_audio_volume));
    out->WriteInt8(audio_panning);
    out->WriteInt8(0); // reserved
    // kCharSvgVersion_36205
    out->WriteInt32(following);
    out->WriteInt32(follow_dist);
    out->WriteInt32(follow_eagerness);
    // kCharSvgVersion_36310
    out->WriteInt32(audio_speed);
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
}
