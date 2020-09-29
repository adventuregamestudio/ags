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

#include "ac/characterextras.h"
#include "util/stream.h"

using AGS::Common::Stream;

void CharacterExtras::ReadFromFile(Stream *in, int32_t cmp_ver)
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
    if (cmp_ver >= 2)
    {
        blend_mode = in->ReadInt32();
        // TODO future implementations
        in->ReadInt32(); // transform scale
        in->ReadInt32(); // transform rotate
        in->ReadInt32(); // sprite anchor x
        in->ReadInt32(); // sprite anchor y
        in->ReadInt32(); // sprite pivot x
        in->ReadInt32(); // sprite pivot y
    }
    else
        blend_mode = 0;
}

void CharacterExtras::WriteToFile(Stream *out)
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
    // since version 2
    out->WriteInt32(blend_mode);
    // TODO future implementations
    out->WriteInt32(0); // transform scale
    out->WriteInt32(0); // transform rotate
    out->WriteInt32(0); // sprite anchor x
    out->WriteInt32(0); // sprite anchor y
    out->WriteInt32(0); // sprite pivot x
    out->WriteInt32(0); // sprite pivot y
}
