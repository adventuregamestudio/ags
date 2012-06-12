
#include "ac_mousecursor.h"

MouseCursor::MouseCursor() { pic = 2054; hotx = 0; hoty = 0; name[0] = 0; flags = 0; view = -1; }

#ifdef ALLEGRO_BIG_ENDIAN
void MouseCursor::ReadFromFile(FILE *fp)
{
    pic = getw(fp);
    hotx = __getshort__bigendian(fp);
    hoty = __getshort__bigendian(fp);
    view = __getshort__bigendian(fp);
    // may need to read padding?
    fread(name, sizeof(char), 10, fp);
    flags = getc(fp);
    fseek(fp, 3, SEEK_CUR);
}
#endif
