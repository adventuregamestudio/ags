
#include "ac_messageinfo.h"

#ifdef ALLEGRO_BIG_ENDIAN
void MessageInfo::ReadFromFile(FILE *fp)
{
    displayas = getc(fp);
    flags = getc(fp);
}
#endif