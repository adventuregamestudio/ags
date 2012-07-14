
#include "ac/global_textbox.h"
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "acmain/ac_maindefines.h"
#include "ac/gamesetupstruct.h"
#include "ac/textbox.h"
#include "gui/guimain.h"
#include "gui/guitextbox.h"

extern GameSetupStruct game;
extern GUIMain*guis;

void SetTextBoxFont(int guin,int objn, int fontnum) {

    if ((guin<0) | (guin>=game.numgui)) quit("!SetTextBoxFont: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetTextBoxFont: invalid object number");
    if (guis[guin].get_control_type(objn) != GOBJ_TEXTBOX)
        quit("!SetTextBoxFont: specified control is not a text box");

    GUITextBox *guit = (GUITextBox*)guis[guin].objs[objn];
    TextBox_SetFont(guit, fontnum);
}

void GetTextBoxText(int guin, int objn, char*txbuf) {
    VALIDATE_STRING(txbuf);
    if ((guin<0) | (guin>=game.numgui)) quit("!GetTextBoxText: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!GetTextBoxText: invalid object number");
    if (guis[guin].get_control_type(objn)!=GOBJ_TEXTBOX)
        quit("!GetTextBoxText: specified control is not a text box");

    GUITextBox*guisl=(GUITextBox*)guis[guin].objs[objn];
    TextBox_GetText(guisl, txbuf);
}

void SetTextBoxText(int guin, int objn, char*txbuf) {
    if ((guin<0) | (guin>=game.numgui)) quit("!SetTextBoxText: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetTextBoxText: invalid object number");
    if (guis[guin].get_control_type(objn)!=GOBJ_TEXTBOX)
        quit("!SetTextBoxText: specified control is not a text box");

    GUITextBox*guisl=(GUITextBox*)guis[guin].objs[objn];
    TextBox_SetText(guisl, txbuf);
}
