
#include <stdio.h>
#include "ac/messageinfo.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

void MessageInfo::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    displayas = in->ReadInt8();
    flags = in->ReadInt8();
//#else
//    throw "DialogTopic::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}