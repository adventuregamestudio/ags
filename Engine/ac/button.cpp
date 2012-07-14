
#include "ac/button.h"
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/view.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_translation.h"
#include "ac/string.h"
#include "ac/viewframe.h"
#include "debug/debug.h"
#include "gui/animatingguibutton.h"
#include "gui/guimain.h"

extern GameSetupStruct game;
extern ViewStruct*views;
extern GUIMain*guis;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];

// *** BUTTON FUNCTIONS

AnimatingGUIButton animbuts[MAX_ANIMATING_BUTTONS];
int numAnimButs;

void Button_Animate(GUIButton *butt, int view, int loop, int speed, int repeat) {
    int guin = butt->guin;
    int objn = butt->objn;

    if ((view < 1) || (view > game.numviews))
        quit("!AnimateButton: invalid view specified");
    view--;

    if ((loop < 0) || (loop >= views[view].numLoops))
        quit("!AnimateButton: invalid loop specified for view");

    // if it's already animating, stop it
    FindAndRemoveButtonAnimation(guin, objn);

    if (numAnimButs >= MAX_ANIMATING_BUTTONS)
        quit("!AnimateButton: too many animating GUI buttons at once");

    int buttonId = guis[guin].objrefptr[objn] & 0x000ffff;

    guibuts[buttonId].pushedpic = 0;
    guibuts[buttonId].overpic = 0;

    animbuts[numAnimButs].ongui = guin;
    animbuts[numAnimButs].onguibut = objn;
    animbuts[numAnimButs].buttonid = buttonId;
    animbuts[numAnimButs].view = view;
    animbuts[numAnimButs].loop = loop;
    animbuts[numAnimButs].speed = speed;
    animbuts[numAnimButs].repeat = repeat;
    animbuts[numAnimButs].frame = -1;
    animbuts[numAnimButs].wait = 0;
    numAnimButs++;
    // launch into the first frame
    if (UpdateAnimatingButton(numAnimButs - 1))
        quit("!AnimateButton: no frames to animate");
}

const char* Button_GetText_New(GUIButton *butt) {
    return CreateNewScriptString(butt->text);
}

void Button_GetText(GUIButton *butt, char *buffer) {
    strcpy(buffer, butt->text);
}

void Button_SetText(GUIButton *butt, const char *newtx) {
    newtx = get_translation(newtx);
    if (strlen(newtx) > 49) quit("!SetButtonText: text too long, button has 50 chars max");

    if (strcmp(butt->text, newtx)) {
        guis_need_update = 1;
        strcpy(butt->text,newtx);
    }
}

void Button_SetFont(GUIButton *butt, int newFont) {
    if ((newFont < 0) || (newFont >= game.numfonts))
        quit("!Button.Font: invalid font number.");

    if (butt->font != newFont) {
        butt->font = newFont;
        guis_need_update = 1;
    }
}

int Button_GetFont(GUIButton *butt) {
    return butt->font;
}

int Button_GetClipImage(GUIButton *butt) {
    if (butt->flags & GUIF_CLIP)
        return 1;
    return 0;
}

void Button_SetClipImage(GUIButton *butt, int newval) {
    butt->flags &= ~GUIF_CLIP;
    if (newval)
        butt->flags |= GUIF_CLIP;

    guis_need_update = 1;
}

int Button_GetGraphic(GUIButton *butt) {
    // return currently displayed pic
    if (butt->usepic < 0)
        return butt->pic;
    return butt->usepic;
}

int Button_GetMouseOverGraphic(GUIButton *butt) {
    return butt->overpic;
}

void Button_SetMouseOverGraphic(GUIButton *guil, int slotn) {
    DEBUG_CONSOLE("GUI %d Button %d mouseover set to slot %d", guil->guin, guil->objn, slotn);

    if ((guil->isover != 0) && (guil->ispushed == 0))
        guil->usepic = slotn;
    guil->overpic = slotn;

    guis_need_update = 1;
    FindAndRemoveButtonAnimation(guil->guin, guil->objn);
}

int Button_GetNormalGraphic(GUIButton *butt) {
    return butt->pic;
}

void Button_SetNormalGraphic(GUIButton *guil, int slotn) {
    DEBUG_CONSOLE("GUI %d Button %d normal set to slot %d", guil->guin, guil->objn, slotn);
    // normal pic - update if mouse is not over, or if there's no overpic
    if (((guil->isover == 0) || (guil->overpic < 1)) && (guil->ispushed == 0))
        guil->usepic = slotn;
    guil->pic = slotn;
    // update the clickable area to the same size as the graphic
    guil->wid = spritewidth[slotn];
    guil->hit = spriteheight[slotn];

    guis_need_update = 1;
    FindAndRemoveButtonAnimation(guil->guin, guil->objn);
}

int Button_GetPushedGraphic(GUIButton *butt) {
    return butt->pushedpic;
}

void Button_SetPushedGraphic(GUIButton *guil, int slotn) {
    DEBUG_CONSOLE("GUI %d Button %d pushed set to slot %d", guil->guin, guil->objn, slotn);

    if (guil->ispushed)
        guil->usepic = slotn;
    guil->pushedpic = slotn;

    guis_need_update = 1;
    FindAndRemoveButtonAnimation(guil->guin, guil->objn);
}

int Button_GetTextColor(GUIButton *butt) {
    return butt->textcol;
}

void Button_SetTextColor(GUIButton *butt, int newcol) {
    if (butt->textcol != newcol) {
        butt->textcol = newcol;
        guis_need_update = 1;
    }
}

extern AnimatingGUIButton animbuts[MAX_ANIMATING_BUTTONS];
extern int numAnimButs;

// ** start animating buttons code

// returns 1 if animation finished
int UpdateAnimatingButton(int bu) {
    if (animbuts[bu].wait > 0) {
        animbuts[bu].wait--;
        return 0;
    }
    ViewStruct *tview = &views[animbuts[bu].view];

    animbuts[bu].frame++;

    if (animbuts[bu].frame >= tview->loops[animbuts[bu].loop].numFrames) 
    {
        if (tview->loops[animbuts[bu].loop].RunNextLoop()) {
            // go to next loop
            animbuts[bu].loop++;
            animbuts[bu].frame = 0;
        }
        else if (animbuts[bu].repeat) {
            animbuts[bu].frame = 0;
            // multi-loop anim, go back
            while ((animbuts[bu].loop > 0) && 
                (tview->loops[animbuts[bu].loop - 1].RunNextLoop()))
                animbuts[bu].loop--;
        }
        else
            return 1;
    }

    CheckViewFrame(animbuts[bu].view, animbuts[bu].loop, animbuts[bu].frame);

    // update the button's image
    guibuts[animbuts[bu].buttonid].pic = tview->loops[animbuts[bu].loop].frames[animbuts[bu].frame].pic;
    guibuts[animbuts[bu].buttonid].usepic = guibuts[animbuts[bu].buttonid].pic;
    guibuts[animbuts[bu].buttonid].pushedpic = 0;
    guibuts[animbuts[bu].buttonid].overpic = 0;
    guis_need_update = 1;

    animbuts[bu].wait = animbuts[bu].speed + tview->loops[animbuts[bu].loop].frames[animbuts[bu].frame].speed;
    return 0;
}

void StopButtonAnimation(int idxn) {
    numAnimButs--;
    for (int aa = idxn; aa < numAnimButs; aa++) {
        animbuts[aa] = animbuts[aa + 1];
    }
}

void FindAndRemoveButtonAnimation(int guin, int objn) {

    for (int ii = 0; ii < numAnimButs; ii++) {
        if ((animbuts[ii].ongui == guin) && (animbuts[ii].onguibut == objn)) {
            StopButtonAnimation(ii);
            ii--;
        }

    }
}
// ** end animating buttons code

