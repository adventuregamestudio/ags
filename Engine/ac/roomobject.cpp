//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#include "ac/roomobject.h"
#include "ac/common.h"
#include "ac/common_defines.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/object.h"
#include "ac/runtime_defines.h"
#include "ac/viewframe.h"
#include "debug/debug_log.h"
#include "main/update.h"
#include "util/math.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

extern std::vector<ViewStruct> views;
extern GameState play;
extern GameSetupStruct game;

RoomObject::RoomObject()
{
    x = y = 0;
    transparent = 0;
    tint_r = tint_g = 0;
    tint_b = tint_level = 0;
    tint_light = 0;
    zoom = 0;
    last_width = last_height = 0;
    num = 0;
    baseline = 0;
    view = loop = frame = 0;
    wait = moving = 0;
    cycling = 0;
    overall_speed = 0;
    on = 0;
    flags = 0;
    blocking_width = blocking_height = 0;
}

int RoomObject::get_width() {
    if (last_width == 0)
        return game.SpriteInfos[num].Width;
    return last_width;
}
int RoomObject::get_height() {
    if (last_height == 0)
        return game.SpriteInfos[num].Height;
    return last_height;
}
int RoomObject::get_baseline() {
    if (baseline < 1)
        return y;
    return baseline;
}

void RoomObject::UpdateCyclingView(int ref_id)
{
	if (on != 1) return;
    if (moving>0) {
      do_movelist_move(&moving,&x,&y);
      }
    if (cycling==0) return;
    if (view == RoomObject::NO_VIEW) return;
    if (wait>0) { wait--; return; }

    cycling = CycleViewAnim(view, loop, frame, cycling < ANIM_BACKWARDS, cycling % ANIM_BACKWARDS);

    ViewFrame*vfptr=&views[view].loops[loop].frames[frame];
    if (vfptr->pic > UINT16_MAX)
        debug_script_warn("Warning: object's (id %d) sprite %d is outside of internal range (%d), reset to 0",
            ref_id, vfptr->pic, UINT16_MAX);
    num = Math::InRangeOrDef<uint16_t>(vfptr->pic, 0);

    if (cycling == 0)
      return;

    wait=vfptr->speed+overall_speed;
    CheckViewFrame (view, loop, frame);
}

void RoomObject::ReadFromSavegame(Stream *in, int save_ver)
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
    on = in->ReadInt8();
    flags = in->ReadInt8();
    blocking_width = in->ReadInt16();
    blocking_height = in->ReadInt16();
    if (save_ver > 0)
    {
        name = StrUtil::ReadString(in);
    }
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
    out->WriteInt8(on);
    out->WriteInt8(flags);
    out->WriteInt16(blocking_width);
    out->WriteInt16(blocking_height);
    StrUtil::WriteString(name, out);
}
