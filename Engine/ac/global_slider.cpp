
#include "ac/global_slider.h"
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/slider.h"
#include "gui/guimain.h"
#include "gui/guislider.h"

extern GameSetupStruct game;
extern GUIMain*guis;

void SetSliderValue(int guin,int objn, int valn) {
    if ((guin<0) | (guin>=game.numgui)) quit("!SetSliderValue: invalid GUI number");
    if (guis[guin].get_control_type(objn)!=GOBJ_SLIDER)
        quit("!SetSliderValue: specified control is not a slider");

    GUISlider*guisl=(GUISlider*)guis[guin].objs[objn];
    Slider_SetValue(guisl, valn);
}

int GetSliderValue(int guin,int objn) {
    if ((guin<0) | (guin>=game.numgui)) quit("!GetSliderValue: invalid GUI number");
    if (guis[guin].get_control_type(objn)!=GOBJ_SLIDER)
        quit("!GetSliderValue: specified control is not a slider");

    GUISlider*guisl=(GUISlider*)guis[guin].objs[objn];
    return Slider_GetValue(guisl);
}
