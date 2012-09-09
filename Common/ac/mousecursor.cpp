
#include <stdio.h>
#include "ac/mousecursor.h"
#include "util/file.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

MouseCursor::MouseCursor() { pic = 2054; hotx = 0; hoty = 0; name[0] = 0; flags = 0; view = -1; }

void MouseCursor::ReadFromFile(CDataStream *in)
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

void MouseCursor::WriteToFile(CDataStream *out)
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
