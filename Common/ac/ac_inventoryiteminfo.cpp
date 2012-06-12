
#include "ac_inventoryiteminfo.h"

#ifdef ALLEGRO_BIG_ENDIAN
void ReadFromFile(FILE *fp)
{
    fread(name, sizeof(char), 25, fp);
    fseek(fp, 3, SEEK_CUR);
    pic = getw(fp);
    cursor = getw(fp);
    hotx = getw(fp);
    hoty = getw(fp);
    fread(reserved, sizeof(int), 5, fp);
    flags = getc(fp);
    fseek(fp, 3, SEEK_CUR);
}
#endif