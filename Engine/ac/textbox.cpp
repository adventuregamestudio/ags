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
#include "ac/textbox.h"
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/string.h"

extern int guis_need_update;
extern GameSetupStruct game;


// ** TEXT BOX FUNCTIONS

const char* TextBox_GetText_New(GUITextBox *texbox) {
    return CreateNewScriptStringAsRetVal(texbox->text);
}

void TextBox_GetText(GUITextBox *texbox, char *buffer) {
    strcpy(buffer, texbox->text);
}

void TextBox_SetText(GUITextBox *texbox, const char *newtex) {
    if (strlen(newtex) > 190)
        quit("!SetTextBoxText: text too long");

    if (strcmp(texbox->text, newtex)) {
        strcpy(texbox->text, newtex);
        guis_need_update = 1;
    }
}

int TextBox_GetTextColor(GUITextBox *guit) {
    return guit->textcol;
}

void TextBox_SetTextColor(GUITextBox *guit, int colr)
{
    if (guit->textcol != colr) 
    {
        guit->textcol = colr;
        guis_need_update = 1;
    }
}

int TextBox_GetFont(GUITextBox *guit) {
    return guit->font;
}

void TextBox_SetFont(GUITextBox *guit, int fontnum) {
    if ((fontnum < 0) || (fontnum >= game.numfonts))
        quit("!SetTextBoxFont: invalid font number.");

    if (guit->font != fontnum) {
        guit->font = fontnum;
        guis_need_update = 1;
    }
}
