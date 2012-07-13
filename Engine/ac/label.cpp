
#include <string.h>
#include "ac/label.h"
#include "wgt2allg.h"
#include "ac/ac_common.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_translation.h"
#include "ac/string.h"

extern int guis_need_update;
extern GameSetupStruct game;

// ** LABEL FUNCTIONS

const char* Label_GetText_New(GUILabel *labl) {
    return CreateNewScriptString(labl->GetText());
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
