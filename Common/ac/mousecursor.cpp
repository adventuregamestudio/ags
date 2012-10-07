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

#include <stdio.h>
#include "ac/mousecursor.h"
#include "util/file.h"
#include "util/datastream.h"

using AGS::Common::DataStream;

MouseCursor::MouseCursor() { pic = 2054; hotx = 0; hoty = 0; name[0] = 0; flags = 0; view = -1; }

void MouseCursor::ReadFromFile(DataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    pic = in->ReadInt32();
    hotx = in->ReadInt16();//__getshort__bigendian(fp);
    hoty = in->ReadInt16();//__getshort__bigendian(fp);
    view = in->ReadInt16();//__getshort__bigendian(fp);
    // may need to read padding?
    in->Read(name, 10);
    flags = in->ReadInt8();
    in->Seek(Common::kSeekCurrent, 3);
//#else
//    throw "MouseCursor::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

void MouseCursor::WriteToFile(DataStream *out)
{
    char padding[3] = {0,0,0};

    out->WriteInt32(pic);
    out->WriteInt16(hotx);
    out->WriteInt16(hoty);
    out->WriteInt16(view);
    out->Write(name, 10);
    out->WriteInt8(flags);
    out->Write(padding, 3);
}
