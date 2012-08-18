
#include "ac/characterextras.h"

void CharacterExtras::ReadFromFile(FILE *f)
{
    fread(invorder, sizeof(short), MAX_INVORDER, f);
    invorder_count = getshort(f);
    width = getshort(f);
    height = getshort(f);
    zoom = getshort(f);
    xwas = getshort(f);
    ywas = getshort(f);
    tint_r = getshort(f);
    tint_g = getshort(f);
    tint_b = getshort(f);
    tint_level = getshort(f);
    tint_light = getshort(f);
    process_idle_this_time = getc(f);
    slow_move_counter = getc(f);
    animwait = getshort(f);
    fseek(f, get_padding(MAX_INVORDER * sizeof(short) + 2), SEEK_CUR);
}

void CharacterExtras::WriteToFile(FILE *f)
{
    char padding[3] = {0,0,0};
    fwrite(invorder, sizeof(short), MAX_INVORDER, f);
    putshort(invorder_count, f);
    putshort(width, f);
    putshort(height, f);
    putshort(zoom, f);
    putshort(xwas, f);
    putshort(ywas, f);
    putshort(tint_r, f);
    putshort(tint_g, f);
    putshort(tint_b, f);
    putshort(tint_level, f);
    putshort(tint_light, f);
    putc(process_idle_this_time, f);
    putc(slow_move_counter, f);
    putshort(animwait, f);
    fwrite(padding, sizeof(char), get_padding(MAX_INVORDER * sizeof(short) + 2), f);
}
