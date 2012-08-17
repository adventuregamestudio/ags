
#include <stdio.h>
#include "ac/inventoryiteminfo.h"

void InventoryItemInfo::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    fread(name, sizeof(char), 25, fp);
    fseek(fp, 3, SEEK_CUR);
    pic = getw(fp);
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

void InventoryItemInfo::WriteToFile(FILE *fp)
{
    char padding[3] = {0,0,0};

    fwrite(name, sizeof(char), 25, fp);
    fwrite(padding, sizeof(char), 3, fp);
    putw(pic, fp);
    putw(cursorPic, fp);
    putw(hotx, fp);
    putw(hoty, fp);
    fwrite(reserved, sizeof(int), 5, fp);
    putc(flags, fp);
    fwrite(padding, sizeof(char), 3, fp);
}
