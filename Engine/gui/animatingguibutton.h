//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_GUI__ANIMATINGGUIBUTTON_H
#define __AGS_EE_GUI__ANIMATINGGUIBUTTON_H

#include "ac/runtime_defines.h"
#include "util/file.h"

struct AnimatingGUIButton {
    // index into guibuts array, GUI, button
    short buttonid, ongui, onguibut;
    // current animation status
    short view, loop, frame;
    short speed, repeat, wait;

    void ReadFromFile(FILE *f);
    void WriteToFile(FILE *f);
};

#endif // __AGS_EE_GUI__ANIMATINGGUIBUTTON_H
