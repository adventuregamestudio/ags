
#include <stdio.h>
#include "ac/ac_mousecursor.h"
#include "platform/file.h"

MouseCursor::MouseCursor() { pic = 2054; hotx = 0; hoty = 0; name[0] = 0; flags = 0; view = -1; }

void MouseCursor::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    pic = getw(fp);
    hotx = getshort(fp);//__getshort__bigendian(fp);
    hoty = getshort(fp);//__getshort__bigendian(fp);
    view = getshort(fp);//__getshort__bigendian(fp);
    // may need to read padding?
    fread(name, sizeof(char), 10, fp);
    flags = getc(fp);
    fseek(fp, 3, SEEK_CUR);
//#else
//    throw "MouseCursor::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}
