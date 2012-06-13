
#include <stdio.h>
#include "ac/ac_point.h"
#include "ac/ac_common.h"    // quit()

void PolyPoints::add_point(int xxx,int yyy) {
    x[numpoints] = xxx;
    y[numpoints] = yyy;
    numpoints++;

    if (numpoints >= MAXPOINTS)
        quit("too many poly points added");
}

void PolyPoints::ReadFromFile(FILE *fp)
{
#ifdef ALLEGRO_BIG_ENDIAN
    fread(x, sizeof(int), MAXPOINTS, fp);
    fread(y, sizeof(int), MAXPOINTS, fp);
    numpoints = getw(fp);
#else
    throw "PolyPoints::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
#endif
}
