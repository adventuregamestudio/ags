
#include <stdio.h>
#include "acrun/ac_roomobject.h"
#include "ac/ac_defines.h"

extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];

int RoomObject::get_width() {
    if (last_width == 0)
        return spritewidth[num];
    return last_width;
}
int RoomObject::get_height() {
    if (last_height == 0)
        return spriteheight[num];
    return last_height;
}
int RoomObject::get_baseline() {
    if (baseline < 1)
        return y;
    return baseline;
}


void RoomObject::ReadFromFile(FILE *fp)
{
#ifdef ALLEGRO_BIG_ENDIAN
    fread(&x, sizeof(int), 2, fp);
    fread(&tint_r, sizeof(short), 15, fp);
    fread(&cycling, sizeof(char), 4, fp);
    fread(&blocking_width, sizeof(short), 2, fp);
    fseek(fp, 2, SEEK_CUR);
#else
    throw "RoomObject::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
#endif
}
void RoomObject::WriteToFile(FILE *fp)
{
#ifdef ALLEGRO_BIG_ENDIAN
    fwrite(&x, sizeof(int), 2, fp);
    fwrite(&tint_r, sizeof(short), 15, fp);
    fwrite(&cycling, sizeof(char), 4, fp);
    fwrite(&blocking_width, sizeof(short), 2, fp);
    putc(0, fp); putc(0, fp);
#else
    throw "RoomObject::WriteToFile() is not implemented for little-endian platforms and should not be called.";
#endif
}
