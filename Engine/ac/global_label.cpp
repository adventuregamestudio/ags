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

#include "ac/global_label.h"
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/label.h"
#include "ac/string.h"
#include "gui/guimain.h"

extern GameSetupStruct game;
extern GUIMain*guis;

void SetLabelColor(int guin,int objn, int colr) {
    if ((guin<0) | (guin>=game.numgui))
        quit("!SetLabelColor: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs))
        quit("!SetLabelColor: invalid object number");
    if (guis[guin].get_control_type(objn)!=GOBJ_LABEL)
        quit("!SetLabelColor: specified control is not a label");

    GUILabel*guil=(GUILabel*)guis[guin].objs[objn];
    Label_SetColor(guil, colr);
}

void SetLabelText(int guin,int objn, const char*newtx) {
    VALIDATE_STRING(newtx);
    if ((guin<0) | (guin>=game.numgui)) quit("!SetLabelText: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetLabelTexT: invalid object number");
    if (guis[guin].get_control_type(objn)!=GOBJ_LABEL)
        quit("!SetLabelText: specified control is not a label");

    GUILabel*guil=(GUILabel*)guis[guin].objs[objn];
    Label_SetText(guil, newtx);
}

void SetLabelFont(int guin,int objn, int fontnum) {

    if ((guin<0) | (guin>=game.numgui)) quit("!SetLabelFont: invalid GUI number");
    if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetLabelFont: invalid object number");
    if (guis[guin].get_control_type(objn)!=GOBJ_LABEL)
        quit("!SetLabelFont: specified control is not a label");

    GUILabel*guil=(GUILabel*)guis[guin].objs[objn];
    Label_SetFont(guil, fontnum);
}
