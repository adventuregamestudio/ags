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
#include "ac/inventoryiteminfo.h"
#include "util/datastream.h"

using AGS::Common::DataStream;

void InventoryItemInfo::ReadFromFile(DataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    in->Read(name, 25);
    in->Seek(Common::kSeekCurrent, 3);
    pic = in->ReadInt32();
    cursorPic = in->ReadInt32();
    hotx = in->ReadInt32();
    hoty = in->ReadInt32();
    in->ReadArrayOfInt32(reserved, 5);
    flags = in->ReadInt8();
    in->Seek(Common::kSeekCurrent, 3);
//#else
//    throw "InventoryItemInfo::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

void InventoryItemInfo::WriteToFile(DataStream *out)
{
    char padding[3] = {0,0,0};

    out->Write(name, 25);
    out->Write(padding, 3);
    out->WriteInt32(pic);
    out->WriteInt32(cursorPic);
    out->WriteInt32(hotx);
    out->WriteInt32(hoty);
    out->WriteArrayOfInt32(reserved, 5);
    out->WriteInt8(flags);
    out->Write(padding, 3);
}
