
#include "ac/rundefines.h"

struct AnimatingGUIButton {
    // index into guibuts array, GUI, button
    short buttonid, ongui, onguibut;
    // current animation status
    short view, loop, frame;
    short speed, repeat, wait;
};

extern AnimatingGUIButton animbuts[MAX_ANIMATING_BUTTONS];
extern int numAnimButs;

