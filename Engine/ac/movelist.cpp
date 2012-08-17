
#include "util/wgt2allg.h"
#include "ac/movelist.h"

void MoveList::ReadFromFile(FILE *f)
{
    char padding[3] = {0,0,0};
    fread(pos, sizeof(int), MAXNEEDSTAGES, f);
    numstage = getw(f);
    fread(xpermove, sizeof(int), MAXNEEDSTAGES, f);
    fread(ypermove, sizeof(int), MAXNEEDSTAGES, f);
    fromx = getw(f);
    fromy = getw(f);
    onstage = getw(f);
    onpart = getw(f);
    lastx = getw(f);
    lasty = getw(f);
    doneflag = getc(f);
    direct = getc(f);
    fread(padding, sizeof(char), 2, f);
}

void MoveList::WriteToFile(FILE *f)
{
    char padding[3] = {0,0,0};
    fwrite(pos, sizeof(int), MAXNEEDSTAGES, f);
    putw(numstage, f);
    fwrite(xpermove, sizeof(int), MAXNEEDSTAGES, f);
    fwrite(ypermove, sizeof(int), MAXNEEDSTAGES, f);
    putw(fromx, f);
    putw(fromy, f);
    putw(onstage, f);
    putw(onpart, f);
    putw(lastx, f);
    putw(lasty, f);
    putc(doneflag, f);
    putc(direct, f);
    fwrite(padding, sizeof(char), 2, f);
}
