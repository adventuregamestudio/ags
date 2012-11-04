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

#include <string.h>
#include "util/wgt2allg.h"
#include "ac/label.h"
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_translation.h"
#include "ac/string.h"

extern int guis_need_update;
extern GameSetupStruct game;

// ** LABEL FUNCTIONS

const char* Label_GetText_New(GUILabel *labl) {
    return CreateNewScriptStringAsRetVal(labl->GetText());
}

void Label_GetText(GUILabel *labl, char *buffer) {
    strcpy(buffer, labl->GetText());
}

void Label_SetText(GUILabel *labl, const char *newtx) {
    newtx = get_translation(newtx);

    if (strcmp(labl->GetText(), newtx)) {
        guis_need_update = 1;
        labl->SetText(newtx);
    }
}

int Label_GetColor(GUILabel *labl) {
    return labl->textcol;
}

void Label_SetColor(GUILabel *labl, int colr) {
    if (labl->textcol != colr) {
        labl->textcol = colr;
        guis_need_update = 1;
    }
}

int Label_GetFont(GUILabel *labl) {
    return labl->font;
}

void Label_SetFont(GUILabel *guil, int fontnum) {
    if ((fontnum < 0) || (fontnum >= game.numfonts))
        quit("!SetLabelFont: invalid font number.");

    if (fontnum != guil->font) {
        guil->font = fontnum;
        guis_need_update = 1;
    }
}
