//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include "ac/global_textbox.h"
#include "ac/common.h"
#include "ac/string.h"
#include "ac/textbox.h"
#include "game/game_objects.h"
#include "gui/guimain.h"
#include "gui/guitextbox.h"

extern GUIMain*guis;

void SetTextBoxFont(int guin,int objn, int fontnum) {

    if ((guin<0) | (guin>=game.GuiCount)) quit("!SetTextBoxFont: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetTextBoxFont: invalid object number");
    if (guis[guin].get_control_type(objn) != GOBJ_TEXTBOX)
        quit("!SetTextBoxFont: specified control is not a text box");

    GUITextBox *guit = (GUITextBox*)guis[guin].objs[objn];
    TextBox_SetFont(guit, fontnum);
}

void GetTextBoxText(int guin, int objn, char*txbuf) {
    VALIDATE_STRING(txbuf);
    if ((guin<0) | (guin>=game.GuiCount)) quit("!GetTextBoxText: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!GetTextBoxText: invalid object number");
    if (guis[guin].get_control_type(objn)!=GOBJ_TEXTBOX)
        quit("!GetTextBoxText: specified control is not a text box");

    GUITextBox*guisl=(GUITextBox*)guis[guin].objs[objn];
    TextBox_GetText(guisl, txbuf);
}

void SetTextBoxText(int guin, int objn, const char* txbuf) {
    if ((guin<0) | (guin>=game.GuiCount)) quit("!SetTextBoxText: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetTextBoxText: invalid object number");
    if (guis[guin].get_control_type(objn)!=GOBJ_TEXTBOX)
        quit("!SetTextBoxText: specified control is not a text box");

    GUITextBox*guisl=(GUITextBox*)guis[guin].objs[objn];
    TextBox_SetText(guisl, txbuf);
}
