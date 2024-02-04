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
#include "ac/characterinfo.h"
#include <string.h>
#include "ac/game_version.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;


void CharacterInfo::ReadBaseFields(Stream *in)
{
    defview = in->ReadInt32();
    talkview = in->ReadInt32();
    view = in->ReadInt32();
    room = in->ReadInt32();
    prevroom = in->ReadInt32();
    x = in->ReadInt32();
    y = in->ReadInt32();
    wait = in->ReadInt32();
    flags = in->ReadInt32();
    following = in->ReadInt16();
    followinfo = in->ReadInt16();
    idleview = in->ReadInt32();
    idletime = in->ReadInt16();
    idleleft = in->ReadInt16();
    transparency = in->ReadInt16();
    baseline = in->ReadInt16();
    activeinv = in->ReadInt32();
    talkcolor = in->ReadInt32();
    thinkview = in->ReadInt32();
    blinkview = in->ReadInt16();
    blinkinterval = in->ReadInt16();
    blinktimer = in->ReadInt16();
    blinkframe = in->ReadInt16();
    walkspeed_y = in->ReadInt16();
    pic_yoffs = in->ReadInt16();
    z = in->ReadInt32();
    walkwait = in->ReadInt32();
    speech_anim_speed = in->ReadInt16();
    idle_anim_speed = in->ReadInt16();
    blocking_width = in->ReadInt16();
    blocking_height = in->ReadInt16();
    index_id = in->ReadInt32();
    pic_xoffs = in->ReadInt16();
    walkwaitcounter = in->ReadInt16();
    loop = in->ReadInt16();
    frame = in->ReadInt16();
    walking = in->ReadInt16();
    animating = in->ReadInt16();
    walkspeed = in->ReadInt16();
    animspeed = in->ReadInt16();
    in->ReadArrayOfInt16(inv, MAX_INV);
    in->ReadInt16(); // actx__
    in->ReadInt16(); // acty__
}

void CharacterInfo::WriteBaseFields(Stream *out) const
{
    out->WriteInt32(defview);
    out->WriteInt32(talkview);
    out->WriteInt32(view);
    out->WriteInt32(room);
    out->WriteInt32(prevroom);
    out->WriteInt32(x);
    out->WriteInt32(y);
    out->WriteInt32(wait);
    out->WriteInt32(flags);
    out->WriteInt16(following);
    out->WriteInt16(followinfo);
    out->WriteInt32(idleview);
    out->WriteInt16(idletime);
    out->WriteInt16(idleleft);
    out->WriteInt16(transparency);
    out->WriteInt16(baseline);
    out->WriteInt32(activeinv);
    out->WriteInt32(talkcolor);
    out->WriteInt32(thinkview);
    out->WriteInt16(blinkview);
    out->WriteInt16(blinkinterval);
    out->WriteInt16(blinktimer);
    out->WriteInt16(blinkframe);
    out->WriteInt16(walkspeed_y);
    out->WriteInt16(pic_yoffs);
    out->WriteInt32(z);
    out->WriteInt32(walkwait);
    out->WriteInt16(speech_anim_speed);
    out->WriteInt16(idle_anim_speed);
    out->WriteInt16(blocking_width);
    out->WriteInt16(blocking_height);
    out->WriteInt32(index_id);
    out->WriteInt16(pic_xoffs);
    out->WriteInt16(walkwaitcounter);
    out->WriteInt16(loop);
    out->WriteInt16(frame);
    out->WriteInt16(walking);
    out->WriteInt16(animating);
    out->WriteInt16(walkspeed);
    out->WriteInt16(animspeed);
    out->WriteArrayOfInt16(inv, MAX_INV);
    out->WriteInt16(0); // actx__
    out->WriteInt16(0); // acty__
}

void CharacterInfo::ReadFromFile(Stream *in, GameDataVersion data_ver)
{
    ReadBaseFields(in);
    name.ReadCount(in, LEGACY_MAX_CHAR_NAME_LEN);
    scrname.ReadCount(in, LEGACY_MAX_SCRIPT_NAME_LEN);
    on = in->ReadInt8();
    in->ReadInt8(); // alignment padding to int32
}

void CharacterInfo::WriteToFile(Stream *out) const
{
    WriteBaseFields(out);
    name.WriteCount(out, LEGACY_MAX_CHAR_NAME_LEN);
    scrname.WriteCount(out, LEGACY_MAX_SCRIPT_NAME_LEN);
    out->WriteInt8(on);
    out->WriteInt8(0); // alignment padding to int32
}

void CharacterInfo::ReadFromSavegame(Stream *in, CharacterSvgVersion save_ver)
{
    ReadBaseFields(in);
    if (save_ver < kCharSvgVersion_36115 || (save_ver >= kCharSvgVersion_400 && save_ver < kCharSvgVersion_400_03))
    { // Fixed-size name and scriptname
        name.ReadCount(in, LEGACY_MAX_CHAR_NAME_LEN);
        in->Seek(LEGACY_MAX_SCRIPT_NAME_LEN); // skip legacy scriptname
                                              // (don't overwrite static data from save!)
    }
    else
    {
        name = StrUtil::ReadString(in);
    }
    on = in->ReadInt8();

    //
    // Upgrade restored data
    if (save_ver < kCharSvgVersion_36025)
    {
        idle_anim_speed = animspeed + 5;
    }
}

void CharacterInfo::WriteToSavegame(Stream *out) const
{
    WriteBaseFields(out);
    StrUtil::WriteString(name, out); // kCharSvgVersion_36115
    out->WriteInt8(on);
}
