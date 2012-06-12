
#include "ac_point.h"
#include "ac_quit.h"    // quit()

void PolyPoints::add_point(int xxx,int yyy) {
    x[numpoints] = xxx;
    y[numpoints] = yyy;
    numpoints++;

    if (numpoints >= MAXPOINTS)
        quit("too many poly points added");
}

#ifdef ALLEGRO_BIG_ENDIAN
void PolyPoints::ReadFromFile(FILE *fp)
{
    fread(x, sizeof(int), MAXPOINTS, fp);
    fread(y, sizeof(int), MAXPOINTS, fp);
    numpoints = getw(fp);
}
#endif