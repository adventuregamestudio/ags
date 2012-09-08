
#include "util/wgt2allg.h"
#include "ac/movelist.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

void MoveList::ReadFromFile(CDataStream *in)
{
    char padding[3] = {0,0,0};
    in->ReadArray(pos, sizeof(int), MAXNEEDSTAGES);
    numstage = in->ReadInt32();
    in->ReadArray(xpermove, sizeof(int), MAXNEEDSTAGES);
    in->ReadArray(ypermove, sizeof(int), MAXNEEDSTAGES);
    fromx = in->ReadInt32();
    fromy = in->ReadInt32();
    onstage = in->ReadInt32();
    onpart = in->ReadInt32();
    lastx = in->ReadInt32();
    lasty = in->ReadInt32();
    doneflag = in->ReadInt8();
    direct = in->ReadInt8();
    in->ReadArray(padding, sizeof(char), 2);
}

void MoveList::WriteToFile(CDataStream *out)
{
    char padding[3] = {0,0,0};
    out->WriteArray(pos, sizeof(int), MAXNEEDSTAGES);
    out->WriteInt32(numstage);
    out->WriteArray(xpermove, sizeof(int), MAXNEEDSTAGES);
    out->WriteArray(ypermove, sizeof(int), MAXNEEDSTAGES);
    out->WriteInt32(fromx);
    out->WriteInt32(fromy);
    out->WriteInt32(onstage);
    out->WriteInt32(onpart);
    out->WriteInt32(lastx);
    out->WriteInt32(lasty);
    out->WriteInt8(doneflag);
    out->WriteInt8(direct);
    out->WriteArray(padding, sizeof(char), 2);
}
