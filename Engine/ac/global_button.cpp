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

#include "ac/global_button.h"
#include "ac/common.h"
#include "ac/button.h"
#include "ac/string.h"
#include "game/game_objects.h"
#include "gui/guimain.h"
#include "gui/guibutton.h"

void SetButtonText(int guin,int objn, const char*newtx) {
    VALIDATE_STRING(newtx);
    if ((guin<0) | (guin>=game.GuiCount))
        quit("!SetButtonText: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].ControlCount))
        quit("!SetButtonText: invalid object number");
    if (guis[guin].GetControlType(objn)!=Common::kGuiButton)
        quit("!SetButtonText: specified control is not a button");

    GuiButton*guil=(GuiButton*)guis[guin].Controls[objn];
    Button_SetText(guil, newtx);
}


void AnimateButton(int guin, int objn, int view, int loop, int speed, int repeat) {
    if ((guin<0) | (guin>=game.GuiCount)) quit("!AnimateButton: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].ControlCount)) quit("!AnimateButton: invalid object number");
    if (guis[guin].GetControlType(objn)!=Common::kGuiButton)
        quit("!AnimateButton: specified control is not a button");

    Button_Animate((GuiButton*)guis[guin].Controls[objn], view, loop, speed, repeat);
}


int GetButtonPic(int guin, int objn, int ptype) {
    if ((guin<0) | (guin>=game.GuiCount)) quit("!GetButtonPic: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].ControlCount)) quit("!GetButtonPic: invalid object number");
    if (guis[guin].GetControlType(objn)!=Common::kGuiButton)
        quit("!GetButtonPic: specified control is not a button");
    if ((ptype < 0) | (ptype > 3)) quit("!GetButtonPic: invalid pic type");

    GuiButton*guil=(GuiButton*)guis[guin].Controls[objn];

    if (ptype == 0) {
        // currently displayed pic
        if (guil->CurrentImage < 0)
            return guil->NormalImage;
        return guil->CurrentImage;
    }
    else if (ptype==1) {
        // nomal pic
        return guil->NormalImage;
    }
    else if (ptype==2) {
        // mouseover pic
        return guil->MouseOverImage;
    }
    else { // pushed pic
        return guil->PushedImage;
    }

    quit("internal error in getbuttonpic");
}

void SetButtonPic(int guin,int objn,int ptype,int slotn) {
    if ((guin<0) | (guin>=game.GuiCount)) quit("!SetButtonPic: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].ControlCount)) quit("!SetButtonPic: invalid object number");
    if (guis[guin].GetControlType(objn)!=Common::kGuiButton)
        quit("!SetButtonPic: specified control is not a button");
    if ((ptype<1) | (ptype>3)) quit("!SetButtonPic: invalid pic type");

    GuiButton*guil=(GuiButton*)guis[guin].Controls[objn];
    if (ptype==1) {
        Button_SetNormalGraphic(guil, slotn);
    }
    else if (ptype==2) {
        // mouseover pic
        Button_SetMouseOverGraphic(guil, slotn);
    }
    else { // pushed pic
        Button_SetPushedGraphic(guil, slotn);
    }
}
