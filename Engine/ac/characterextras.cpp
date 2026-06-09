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
#include "ac/characterinfo.h"
#include "ac/gamesetupstruct.h"
#include "ac/viewframe.h"
#include "ac/dynobj/cc_character.h"
#include "ac/dynobj/scriptstring.h"
#include "script/script.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

extern CCCharacter ccDynamicCharacter;
extern std::vector<ViewStruct> views;
extern GameSetupStruct game;

void CharacterExtras::UpdateGraphicSpace(const CharacterInfo *chin)
{
    // FIXME: zoom_offs does not work now, should we deprecate and cut this factor completely?
    // if not, then either:
    // * AABB should *divide* graphical offset by zoom, which may only work if AABB is made
    // of floats (otherwise value precision will be lost in integer division).
    // * OR we might require ADDITIONAL parameter into GS, which is an offset applied UNSCALED.
    _gs = GraphicSpace(
        chin->x, chin->y, eff_anchor, // origin
        Size(spr_width, spr_height), // source sprite size
        Size(width, height), // destination size (scaled)
        // real graphical aabb (maybe with extra offsets)
        RectWH((eff_offset.X + frame_xoff),
               (-chin->z + eff_offset.Y + frame_yoff),
               spr_width, spr_height),
        rotation // transforms
    );
}

void CharacterExtras::SetTurning(bool on, bool ccw, int turn_steps)
{
    if (on && (turn_steps > 0))
        flags = (flags & ~kCharf_TurningMask) | kCharf_Turning | (ccw ? kCharf_TurningCCW : 0);
    else
        flags = (flags & ~kCharf_TurningMask);
    turns = turn_steps;
}

void CharacterExtras::DecrementTurning()
{
    turns--;
    if (turns <= 0)
    {
        SetTurning(false, false, 0);
    }
}

void CharacterExtras::SetAnimating(AnimFlowStyle flow, AnimFlowDirection dir, int delay, int anim_volume)
{
    anim = ViewAnimateParams(flow, dir, delay, anim_volume);
}

void CharacterExtras::ResetAnimating()
{
    anim = ViewAnimateParams();
}

int CharacterExtras::GetEffectiveY(const CharacterInfo *chi) const
{
    return chi->y - (chi->z * zoom_offs) / 100;
}

int CharacterExtras::GetFrameSoundVolume(const CharacterInfo *chi) const
{
    return ::CalcFrameSoundVolume(
        audio_volume, anim.AudioVolume,
        (chi->flags & CHF_SCALEVOLUME) ? zoom : 100);
}

void CharacterExtras::SetLockedView(CharacterInfo *chi, int view, int loop, int frame, const Pointf &anchor, const Point &off)
{
    chi->view = view;
    chi->loop = loop;
    chi->frame = frame;
    chi->wait = 0;
    chi->flags |= CHF_FIXVIEW;
    view_anchor = anchor;
    view_offset = off;
    UpdateEffectiveValues(chi);
}

void CharacterExtras::SetUnlockedView(CharacterInfo *chi)
{
    chi->flags &= ~CHF_FIXVIEW;
    chi->view = chi->defview;
    chi->frame = 0;
    view_anchor = CharacterInfo::GetDefaultSpriteAnchor();
    view_offset = Point();
    UpdateEffectiveValues(chi);
}

void CharacterExtras::CheckViewFrame(CharacterInfo *chi)
{
    ObjectEvent objevt(kScTypeGame, RuntimeScriptValue().SetScriptObject(chi, &ccDynamicCharacter));
    ::CheckViewFrame(chi->view, chi->loop, chi->frame, GetFrameSoundVolume(chi), audio_panning, audio_speed,
        objevt, &chi->GetEvents(), kCharacterEvent_OnFrameEvent);
}

bool CharacterExtras::RunFrameEvent(CharacterInfo *chi, int view, int loop, int frame)
{
    ObjectEvent objevt(kScTypeGame, RuntimeScriptValue().SetScriptObject(chi, &ccDynamicCharacter));
    return ::RunViewFrameEvent(view, loop, frame,
        objevt, &chi->GetEvents(), kCharacterEvent_OnFrameEvent);
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

void CharacterExtras::UpdateEffectiveValues(const CharacterInfo *chin)
{
    eff_offset = chin->is_view_locked() ? view_offset : spr_offset;
    eff_anchor = chin->is_view_locked() ? view_anchor : spr_anchor;
}

void CharacterExtras::ReadFromSavegame(CharacterInfo *chin, Stream *in, CharacterSvgVersion save_ver)
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
    int cur_anim_volume = 100;
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

    if (save_ver >= kCharSvgVersion_36205 && (save_ver < kCharSvgVersion_400 || save_ver >= kCharSvgVersion_400_13))
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
    
    if ((save_ver >= kCharSvgVersion_36310) && (save_ver < kCharSvgVersion_400 || save_ver >= kCharSvgVersion_400_28))
    {
        audio_speed = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
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
        float ax = in->ReadFloat32(); // sprite anchor x
        float ay = in->ReadFloat32(); // sprite anchor y
        spr_anchor = Pointf(ax, ay);
    }
    else
    {
        blend_mode = kBlend_Normal;
    }

    if (save_ver >= kCharSvgVersion_400_03)
    {
        face_dir_ratio = in->ReadFloat32();
        movelist_handle = in->ReadInt32();
        flags = in->ReadInt32();
        turns = in->ReadInt32();
    }
    else
    {
        face_dir_ratio = 0.f;
        movelist_handle = 0;
        flags = 0;
        turns = 0;
    }

    AnimFlowStyle anim_flow = kAnimFlow_None;
    AnimFlowDirection anim_dir_initial = kAnimDirForward;
    AnimFlowDirection anim_dir_current = kAnimDirForward;
    int anim_delay = 0;
    if (save_ver >= kCharSvgVersion_400_18)
    {
        shader_id = in->ReadInt32();
        shader_handle = in->ReadInt32();
        // new anim fields are valid since kCharSvgVersion_400_20
        anim_flow = static_cast<AnimFlowStyle>(in->ReadInt8());
        anim_dir_initial = static_cast<AnimFlowDirection>(in->ReadInt8());
        anim_dir_current = static_cast<AnimFlowDirection>(in->ReadInt8());
        in->ReadInt8(); // reserved to fill int32
        anim_delay = in->ReadInt16();
        in->ReadInt16(); // reserved to fill int32
    }
    else
    {
        shader_id = 0;
        shader_handle = 0;
    }

    if (save_ver >= kCharSvgVersion_400_26)
    {
        int offx = in->ReadInt32();
        int offy = in->ReadInt32();
        spr_offset = Point(offx, offy);
        float ax = in->ReadFloat32();
        float ay = in->ReadFloat32();
        view_anchor = Pointf(ax, ay);
        offx = in->ReadInt32();
        offy = in->ReadInt32();
        view_offset = Point(offx, offy);
    }
    else
    {
        spr_anchor = Pointf(0.5f, 1.f); // default: middle-bottom
        spr_offset = Point();
    }

    // NOTE: the legacy animating fields from old save fmt are applied externally,
    // because they are loaded into deprecated CharacterInfo fields
    anim = ViewAnimateParams(anim_flow, anim_dir_initial, anim_dir_current, anim_delay, cur_anim_volume);

    UpdateEffectiveValues(chin);
}

void CharacterExtras::WriteToSavegame(const CharacterInfo *chin, Stream *out) const
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
    out->WriteInt8(static_cast<uint8_t>(anim.AudioVolume));
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
    out->WriteFloat32(spr_anchor.X); // sprite anchor x
    out->WriteFloat32(spr_anchor.Y); // sprite anchor y
    // -- kCharSvgVersion_400_03
    out->WriteFloat32(face_dir_ratio);
    out->WriteInt32(movelist_handle);
    out->WriteInt32(flags);
    out->WriteInt32(turns);
    // -- kCharSvgVersion_400_18
    out->WriteInt32(shader_id);
    out->WriteInt32(shader_handle);
    // new anim fields are valid since kCharSvgVersion_400_20
    out->WriteInt8(anim.Flow);
    out->WriteInt8(anim.InitialDirection);
    out->WriteInt8(anim.Direction);
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt16(anim.Delay);
    out->WriteInt16(0); // reserved to fill int32
    // kRoomStatSvgVersion_40026
    out->WriteInt32(spr_offset.X);
    out->WriteInt32(spr_offset.Y);
    out->WriteFloat32(view_anchor.X);
    out->WriteFloat32(view_anchor.Y);
    out->WriteInt32(view_offset.X);
    out->WriteInt32(view_offset.Y);
}
