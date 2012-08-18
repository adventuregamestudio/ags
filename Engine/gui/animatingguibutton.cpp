
#include "gui/animatingguibutton.h"

void AnimatingGUIButton::ReadFromFile(FILE *f)
{
    buttonid = getshort(f);
    ongui = getshort(f);
    onguibut = getshort(f);
    view = getshort(f);
    loop = getshort(f);
    frame = getshort(f);
    speed = getshort(f);
    repeat = getshort(f);
    wait = getshort(f);
    fseek(f, 3, SEEK_CUR);
}

void AnimatingGUIButton::WriteToFile(FILE *f)
{
    char padding[3] = {0,0,0};
    putshort(buttonid, f);
    putshort(ongui, f);
    putshort(onguibut, f);
    putshort(view, f);
    putshort(loop, f);
    putshort(frame, f);
    putshort(speed, f);
    putshort(repeat, f);
    putshort(wait, f);
    fwrite(padding, sizeof(char), 3, f);
}
