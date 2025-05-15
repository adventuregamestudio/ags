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
#include "debug/debug_log.h"
#include "main/update.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

extern std::vector<ViewStruct> views;
extern GameSetupStruct game;

RoomObject::RoomObject()
{
    x = y = 0;
    transparent = 0;
    tint_r = tint_g = 0;
    tint_b = tint_level = 0;
    tint_light = 0;
    zoom = 100;
    spr_width = spr_height = 0;
    last_width = last_height = 0;
    num = 0;
    baseline = -1;
    view = NoView;
    loop = frame = 0;
    wait = 0;
    moving = -1;
    cycling = 0;
    overall_speed = 0;
    flags = 0;
    blocking_width = blocking_height = 0;
    blend_mode = kBlend_Normal;
    rotation = 0.f;

    UpdateGraphicSpace();
}

int RoomObject::get_width() const {
    if (last_width == 0)
        return game.SpriteInfos[num].Width;
    return last_width;
}

int RoomObject::get_height() const {
    if (last_height == 0)
        return game.SpriteInfos[num].Height;
    return last_height;
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
    if (cycling==0) return;
    if (view == RoomObject::NoView) return;
    if (wait>0) { wait--; return; }

    if (!CycleViewAnim(view, loop, frame, get_anim_forwards(), get_anim_repeat()))
        cycling = 0; // finished animating

    ViewFrame*vfptr=&views[view].loops[loop].frames[frame];
    if (vfptr->pic > UINT16_MAX)
        debug_script_warn("Warning: object's (id %d) sprite %d is outside of internal range (%d), reset to 0",
            ref_id, vfptr->pic, UINT16_MAX);
    num = Math::InRangeOrDef<uint16_t>(vfptr->pic, 0);

    if (cycling == 0)
      return;

    wait=vfptr->speed+overall_speed;
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
    return ::CalcFrameSoundVolume(anim_volume, cur_anim_volume);
}

void RoomObject::CheckViewFrame()
{
    ::CheckViewFrame(view, loop, frame, GetFrameSoundVolume());
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
    last_width = in->ReadInt16();
    last_height = in->ReadInt16();
    num = in->ReadInt16();
    baseline = in->ReadInt16();
    view = in->ReadInt16();
    loop = in->ReadInt16();
    frame = in->ReadInt16();
    wait = in->ReadInt16();
    moving = in->ReadInt16();
    cycling = in->ReadInt8();
    overall_speed = in->ReadInt8();
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
    if (cmp_ver >= kRoomStatSvgVersion_36025)
    { // anim vols order inverted compared to character, by mistake :(
        cur_anim_volume = static_cast<uint8_t>(in->ReadInt8());
        anim_volume = static_cast<uint8_t>(in->ReadInt8());
        in->ReadInt8(); // reserved to fill int32
        in->ReadInt8();
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
        in->ReadInt32(); // sprite anchor x
        in->ReadInt32(); // sprite anchor y
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

    if (cmp_ver >= kRoomStatSvgVersion_40018)
    {
        shader_id = in->ReadInt32();
        shader_handle = in->ReadInt32();
        in->ReadInt32(); // reserve
        in->ReadInt32();
    }
    else
    {
        shader_id = 0;
        shader_handle = 0;
    }

    spr_width = last_width;
    spr_height = last_height;
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
    out->WriteInt16(last_width);
    out->WriteInt16(last_height);
    out->WriteInt16(num);
    out->WriteInt16(baseline);
    out->WriteInt16(view);
    out->WriteInt16(loop);
    out->WriteInt16(frame);
    out->WriteInt16(wait);
    out->WriteInt16(moving);
    out->WriteInt8(cycling);
    out->WriteInt8(overall_speed);
    out->WriteInt32(flags);
    out->WriteInt16(blocking_width);
    out->WriteInt16(blocking_height);
    // since version 1
    StrUtil::WriteString(name, out);
    // since version 2
    out->WriteInt8(static_cast<uint8_t>(cur_anim_volume));
    out->WriteInt8(static_cast<uint8_t>(anim_volume));
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt8(0);
    // since version 10
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
    // kRoomStatSvgVersion_40016
    out->WriteInt32(movelist_handle);
    out->WriteInt32(0); // reserve up to 4 ints
    out->WriteInt32(0);
    out->WriteInt32(0);
    // kRoomStatSvgVersion_40018
    out->WriteInt32(shader_id);
    out->WriteInt32(shader_handle);
    out->WriteInt32(0); // reserve
    out->WriteInt32(0);
}

void RoomObject::UpdateGraphicSpace()
{
    _gs = GraphicSpace(x, y - last_height, spr_width, spr_height, last_width, last_height, rotation);
}
