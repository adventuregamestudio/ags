
#include <stdio.h>
#include "ac/ac_messageinfo.h"

void MessageInfo::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    displayas = getc(fp);
    flags = getc(fp);
//#else
//    throw "DialogTopic::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}