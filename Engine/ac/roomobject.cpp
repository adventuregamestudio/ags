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
#include "ac/roomobject.h"
#include "ac/common.h"
#include "ac/common_defines.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/object.h"
#include "ac/roomstatus.h"
#include "ac/runtime_defines.h"
#include "ac/viewframe.h"
#include "ac/dynobj/cc_object.h"
#include "debug/debug_log.h"
#include "main/update.h"
#include "script/script.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

extern std::vector<ViewStruct> views;
extern GameSetupStruct game;
extern CCObject ccDynamicObject;
extern ScriptObject scrObj[MAX_ROOM_OBJECTS];
extern RoomStruct thisroom;

RoomObject::RoomObject()
{
    x = y = 0;
    transparent = 0;
    tint_r = tint_g = 0;
    tint_b = tint_level = 0;
    tint_light = 0;
    zoom = 100;
    spr_width = spr_height = 0;
    frame_xoff = frame_yoff = 0;
    width = height = 0;
    num = 0;
    baseline = -1;
    view = NoView;
    loop = frame = 0;
    wait = 0;
    moving = -1;
    flags = 0;
    blocking_width = blocking_height = 0;
    blend_mode = kBlend_Normal;
    rotation = 0.f;

    UpdateGraphicSpace();
}

int RoomObject::get_width() const {
    // FIXME: don't use global game object here, instead make sure last_width is always valid
    if (width == 0)
        return game.SpriteInfos[num].Width;
    return width;
}

int RoomObject::get_height() const {
    // FIXME: don't use global game object here, instead make sure last_height is always valid
    if (height == 0)
        return game.SpriteInfos[num].Height;
    return height;
}

int RoomObject::get_baseline() const {
    if (baseline < 1)
        return y;
    return baseline;
}

void RoomObject::UpdateCyclingView(int ref_id)
{
	if (!is_enabled()) return;
    if (moving>0) {
      do_movelist_move(moving, x, y);
      if (moving == 0)
        OnStopMoving();
    }
    if (!is_animating()) return;
    if (view == RoomObject::NoView) return;
    if (wait>0) { wait--; return; }

    if (!CycleViewAnim(view, loop, frame, anim))
    {
        anim = ViewAnimateParams();
    }

    ViewFrame*vfptr=&views[view].loops[loop].frames[frame];
    if (vfptr->pic > UINT16_MAX)
        debug_script_warn("Warning: object's (id %d) sprite %d is outside of internal range (%d), reset to 0",
            ref_id, vfptr->pic, UINT16_MAX);
    num = Math::InRangeOrDef<uint16_t>(vfptr->pic, 0);

    if (!is_animating())
      return;

    wait = anim.Delay + vfptr->speed;
    CheckViewFrame();
}

void RoomObject::OnStopMoving()
{
    // Release movelist
    if (moving > 0)
    {
        remove_movelist(moving);
        moving = 0;
    }
    // If we have a movelist reference attached, then invalidate and dec refcount
    if (movelist_handle > 0)
    {
        release_script_movelist(movelist_handle);
        movelist_handle = 0;
    }
}

// Calculate wanted frame sound volume based on multiple factors
int RoomObject::GetFrameSoundVolume() const
{
    // NOTE: room objects don't have "scale volume" flag at the moment
    return ::CalcFrameSoundVolume(anim_volume, anim.AudioVolume);
}

void RoomObject::CheckViewFrame()
{
    ObjectEvent objevt(kScTypeRoom, RuntimeScriptValue().SetScriptObject(&scrObj[id], &ccDynamicObject));
    ::CheckViewFrame(view, loop, frame, GetFrameSoundVolume(),
        objevt, &thisroom.Objects[id].GetEvents(), kRoomObjectEvent_OnFrameEvent);
}

bool RoomObject::RunFrameEvent(int view, int loop, int frame)
{
    ObjectEvent objevt(kScTypeRoom, RuntimeScriptValue().SetScriptObject(&scrObj[id], &ccDynamicObject));
    return ::RunViewFrameEvent(view, loop, frame,
        objevt, &thisroom.Objects[id].GetEvents(), kRoomObjectEvent_OnFrameEvent);
}

void RoomObject::ReadFromSavegame(Stream *in, int cmp_ver)
{
    x = in->ReadInt32();
    y = in->ReadInt32();
    transparent = in->ReadInt32();
    tint_r = in->ReadInt16();
    tint_g = in->ReadInt16();
    tint_b = in->ReadInt16();
    tint_level = in->ReadInt16();
    tint_light = in->ReadInt16();
    zoom = in->ReadInt16();
    width = in->ReadInt16();
    height = in->ReadInt16();
    num = in->ReadInt16();
    baseline = in->ReadInt16();
    view = in->ReadInt16();
    loop = in->ReadInt16();
    frame = in->ReadInt16();
    wait = in->ReadInt16();
    moving = in->ReadInt16();
    int legacy_animating = in->ReadInt8(); // legacy animating
    int anim_delay = in->ReadInt8(); // legacy anim delay (8-bit)
    if (cmp_ver >= kRoomStatSvgVersion_40003)
    {
        flags = in->ReadInt32();
    }
    else
    {
        uint8_t on = in->ReadInt8(); // old enabled + visible flag
        flags = in->ReadInt8();
        // Only treat "on" as visible and make objects from old saves all enabled
        flags |= OBJF_ENABLED | (OBJF_VISIBLE) * on;
    }

    blocking_width = in->ReadInt16();
    blocking_height = in->ReadInt16();
    if (cmp_ver >= kRoomStatSvgVersion_36016)
    {
        name = StrUtil::ReadString(in);
    }

    int cur_anim_volume = 100;
    if (cmp_ver >= kRoomStatSvgVersion_36025)
    {
        // anim vols order inverted compared to character, by mistake :(
        cur_anim_volume = static_cast<uint8_t>(in->ReadInt8());
        anim_volume = static_cast<uint8_t>(in->ReadInt8());
        in->ReadInt8(); // reserved to fill int32
        in->ReadInt8();
    }
    else
    {
        cur_anim_volume = 100;
        anim_volume = 100;
    }

    if ((cmp_ver >= kRoomStatSvgVersion_36304) && (cmp_ver < kRoomStatSvgVersion_400) ||
        (cmp_ver >= kRoomStatSvgVersion_40026))
    {
        blocking_x = in->ReadInt16();
        blocking_y = in->ReadInt16();
    }
    else
    {
        blocking_x = 0;
        blocking_y = 0;
    }

    if (cmp_ver >= kRoomStatSvgVersion_400)
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

    if (cmp_ver >= kRoomStatSvgVersion_40016)
    {
        movelist_handle = in->ReadInt32();
        in->ReadInt32(); // reserve up to 4 ints
        in->ReadInt32();
        in->ReadInt32();
    }
    else
    {
        movelist_handle = 0;
    }

    AnimFlowStyle anim_flow = kAnimFlow_None;
    AnimFlowDirection anim_dir_initial = kAnimDirForward;
    AnimFlowDirection anim_dir_current = kAnimDirForward;
    if (cmp_ver >= kRoomStatSvgVersion_40018)
    {
        shader_id = in->ReadInt32();
        shader_handle = in->ReadInt32();
        // new anim fields are valid since kRoomStatSvgVersion_40020
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

    if (cmp_ver >= kRoomStatSvgVersion_40026)
    {
        int offx = in->ReadInt32();
        int offy = in->ReadInt32();
        spr_offset = Point(offx, offy);
    }
    else
    {
        spr_anchor = Pointf(0.f, 1.f); // default: left-bottom
        spr_offset = Point();
    }

    // Apply animation params either from old or new save
    if (cmp_ver < kRoomStatSvgVersion_40020)
    {
        switch (legacy_animating % 10)
        {
        case LEGACY_OBJANIM_ONCE: anim_flow = kAnimFlow_Once; break;
        case LEGACY_OBJANIM_REPEAT: anim_flow = kAnimFlow_Repeat; break;
        case LEGACY_OBJANIM_ONCERESET: anim_flow = kAnimFlow_OnceAndReset; break;
        default: anim_flow = kAnimFlow_None; break;
        }
        anim_dir_initial = legacy_animating < 10 ? kAnimDirForward : kAnimDirBackward;
        anim_dir_current = anim_dir_initial;
    }

    anim = ViewAnimateParams(anim_flow, anim_dir_initial, anim_dir_current, anim_delay, cur_anim_volume);
    spr_width = width;
    spr_height = height;
    UpdateGraphicSpace();
}

void RoomObject::WriteToSavegame(Stream *out) const
{
    out->WriteInt32(x);
    out->WriteInt32(y);
    out->WriteInt32(transparent);
    out->WriteInt16(tint_r);
    out->WriteInt16(tint_g);
    out->WriteInt16(tint_b);
    out->WriteInt16(tint_level);
    out->WriteInt16(tint_light);
    out->WriteInt16(zoom);
    out->WriteInt16(width);
    out->WriteInt16(height);
    out->WriteInt16(num);
    out->WriteInt16(baseline);
    out->WriteInt16(view);
    out->WriteInt16(loop);
    out->WriteInt16(frame);
    out->WriteInt16(wait);
    out->WriteInt16(moving);
    out->WriteInt8(0); // legacy animating (8-bit field)
    out->WriteInt8(anim.Delay);
    out->WriteInt32(flags);
    out->WriteInt16(blocking_width);
    out->WriteInt16(blocking_height);
    // kRoomStatSvgVersion_36016
    StrUtil::WriteString(name, out);
    // kRoomStatSvgVersion_36025
    out->WriteInt8(static_cast<uint8_t>(anim.AudioVolume));
    out->WriteInt8(static_cast<uint8_t>(anim_volume));
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt8(0);
    // kRoomStatSvgVersion_36304
    out->WriteInt16(blocking_x);
    out->WriteInt16(blocking_y);
    // kRoomStatSvgVersion_400
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
    // kRoomStatSvgVersion_40016
    out->WriteInt32(movelist_handle);
    out->WriteInt32(0); // reserve up to 4 ints
    out->WriteInt32(0);
    out->WriteInt32(0);
    // kRoomStatSvgVersion_40018
    out->WriteInt32(shader_id);
    out->WriteInt32(shader_handle);
    // new anim fields are valid since kRoomStatSvgVersion_40020
    out->WriteInt8(anim.Flow);
    out->WriteInt8(anim.InitialDirection);
    out->WriteInt8(anim.Direction);
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt16(anim.Delay);
    out->WriteInt16(0); // reserved to fill int32
    // kRoomStatSvgVersion_40026
    out->WriteInt32(spr_offset.X);
    out->WriteInt32(spr_offset.Y);
}

void RoomObject::UpdateGraphicSpace()
{
    _gs = GraphicSpace(
        x, y, spr_anchor, // origin
        Size(spr_width, spr_height), // source sprite size
        Size(width, height), // destination size (scaled)
        // real graphical aabb (maybe with extra offsets)
        RectWH(frame_xoff + spr_offset.X, frame_yoff + spr_offset.Y, spr_width, spr_height),
        rotation // transforms
    );
}
