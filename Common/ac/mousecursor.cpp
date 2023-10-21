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

#include "ac/mousecursor.h"
#include "util/stream.h"

using AGS::Common::Stream;

void MouseCursor::ReadFromFile(Stream *in)
{
    pic = in->ReadInt32();
    hotx = in->ReadInt16();
    hoty = in->ReadInt16();
    view = in->ReadInt16();
    in->Read(name, 10);
    flags = in->ReadInt8();
    in->Seek(3); // aligment padding to int32
}

void MouseCursor::WriteToFile(Stream *out)
{
    out->WriteInt32(pic);
    out->WriteInt16(hotx);
    out->WriteInt16(hoty);
    out->WriteInt16(view);
    out->Write(name, 10);
    out->WriteInt8(flags);
    out->WriteByteCount(0, 3); // aligment padding to int32
}

void MouseCursor::ReadFromSavegame(Stream *in, int cmp_ver)
{
    pic = in->ReadInt32();
    hotx = static_cast<int16_t>(in->ReadInt32());
    hoty = static_cast<int16_t>(in->ReadInt32());
    view = static_cast<int16_t>(in->ReadInt32());
    flags = static_cast<int8_t>(in->ReadInt32());
    if (cmp_ver >= kCursorSvgVersion_36016)
        animdelay = in->ReadInt32();
}

void MouseCursor::WriteToSavegame(Stream *out) const
{
    out->WriteInt32(pic);
    out->WriteInt32(hotx);
    out->WriteInt32(hoty);
    out->WriteInt32(view);
    out->WriteInt32(flags);
    out->WriteInt32(animdelay);
}
