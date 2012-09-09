
#include <stdio.h>
#include "ac/point.h"
#include "ac/common.h"    // quit()
#include "util/datastream.h"

using AGS::Common::CDataStream;

void PolyPoints::add_point(int xxx,int yyy) {
    x[numpoints] = xxx;
    y[numpoints] = yyy;
    numpoints++;

    if (numpoints >= MAXPOINTS)
        quit("too many poly points added");
}

void PolyPoints::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    in->ReadArrayOfInt32(x, MAXPOINTS);
    in->ReadArrayOfInt32(y, MAXPOINTS);
    numpoints = in->ReadInt32();
//#else
//    throw "PolyPoints::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}
