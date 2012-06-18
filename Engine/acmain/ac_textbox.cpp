
#include "acmain/ac_maindefines.h"
#include "acmain/ac_guitextbox.h"

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
