
#include <string.h>
#include "ac/textbox.h"
#include "ac/ac_common.h"
#include "wgt2allg.h"
#include "ac/gamesetupstruct.h"
#include "ac/string.h"

extern int guis_need_update;
extern GameSetupStruct game;


// ** TEXT BOX FUNCTIONS

const char* TextBox_GetText_New(GUITextBox *texbox) {
    return CreateNewScriptString(texbox->text);
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
