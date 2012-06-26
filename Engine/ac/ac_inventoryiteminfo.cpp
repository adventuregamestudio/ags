
#include <stdio.h>
#include "ac/ac_inventoryiteminfo.h"

void InventoryItemInfo::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    fread(name, sizeof(char), 25, fp);
    fseek(fp, 3, SEEK_CUR);
    pic = getw(fp);
    // FIXME: there's no 'cursor' member, could this be cursorPic?
    // cursor = getw(fp);
    cursorPic = getw(fp);
    hotx = getw(fp);
    hoty = getw(fp);
    fread(reserved, sizeof(int), 5, fp);
    flags = getc(fp);
    fseek(fp, 3, SEEK_CUR);
//#else
//    throw "InventoryItemInfo::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}
