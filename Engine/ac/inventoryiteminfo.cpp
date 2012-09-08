
#include <stdio.h>
#include "ac/inventoryiteminfo.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

void InventoryItemInfo::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    in->ReadArray(name, sizeof(char), 25);
    in->Seek(Common::kSeekCurrent, 3);
    pic = in->ReadInt32();
    cursorPic = in->ReadInt32();
    hotx = in->ReadInt32();
    hoty = in->ReadInt32();
    in->ReadArray(reserved, sizeof(int), 5);
    flags = in->ReadInt8();
    in->Seek(Common::kSeekCurrent, 3);
//#else
//    throw "InventoryItemInfo::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

void InventoryItemInfo::WriteToFile(CDataStream *out)
{
    char padding[3] = {0,0,0};

    out->WriteArray(name, sizeof(char), 25);
    out->WriteArray(padding, sizeof(char), 3);
    out->WriteInt32(pic);
    out->WriteInt32(cursorPic);
    out->WriteInt32(hotx);
    out->WriteInt32(hoty);
    out->WriteArray(reserved, sizeof(int), 5);
    out->WriteInt8(flags);
    out->WriteArray(padding, sizeof(char), 3);
}
