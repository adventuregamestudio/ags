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

#include "ac/button.h"
#include "ac/common.h"
#include "ac/gui.h"
#include "ac/view.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_translation.h"
#include "ac/string.h"
#include "ac/viewframe.h"
#include "debug/debug_log.h"
#include "gui/animatingguibutton.h"
#include "gui/guimain.h"

extern GameSetupStruct game;
extern ViewStruct*views;
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

    int buttonId = guis[guin].CtrlRefs[objn] & 0x000ffff;

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

void Button_Click(GUIButton *butt, int mbut)
{
    process_interface_click(butt->guin, butt->objn, mbut);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/dynobj/scriptstring.h"

extern ScriptString myScriptStringImpl;

// void | GUIButton *butt, int view, int loop, int speed, int repeat
RuntimeScriptValue Sc_Button_Animate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(GUIButton, Button_Animate);
}

// const char* | GUIButton *butt
RuntimeScriptValue Sc_Button_GetText_New(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIButton, const char, myScriptStringImpl, Button_GetText_New);
}

// void | GUIButton *butt, char *buffer
RuntimeScriptValue Sc_Button_GetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUIButton, Button_GetText, char);
}

// void | GUIButton *butt, const char *newtx
RuntimeScriptValue Sc_Button_SetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUIButton, Button_SetText, const char);
}

// void | GUIButton *butt, int newFont
RuntimeScriptValue Sc_Button_SetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetFont);
}

// int | GUIButton *butt
RuntimeScriptValue Sc_Button_GetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetFont);
}

// int | GUIButton *butt
RuntimeScriptValue Sc_Button_GetClipImage(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetClipImage);
}

// void | GUIButton *butt, int newval
RuntimeScriptValue Sc_Button_SetClipImage(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetClipImage);
}

// int | GUIButton *butt
RuntimeScriptValue Sc_Button_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetGraphic);
}

// int | GUIButton *butt
RuntimeScriptValue Sc_Button_GetMouseOverGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetMouseOverGraphic);
}

// void | GUIButton *guil, int slotn
RuntimeScriptValue Sc_Button_SetMouseOverGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetMouseOverGraphic);
}

// int | GUIButton *butt
RuntimeScriptValue Sc_Button_GetNormalGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetNormalGraphic);
}

// void | GUIButton *guil, int slotn
RuntimeScriptValue Sc_Button_SetNormalGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetNormalGraphic);
}

// int | GUIButton *butt
RuntimeScriptValue Sc_Button_GetPushedGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetPushedGraphic);
}

// void | GUIButton *guil, int slotn
RuntimeScriptValue Sc_Button_SetPushedGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetPushedGraphic);
}

// int | GUIButton *butt
RuntimeScriptValue Sc_Button_GetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetTextColor);
}

// void | GUIButton *butt, int newcol
RuntimeScriptValue Sc_Button_SetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetTextColor);
}

RuntimeScriptValue Sc_Button_Click(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_Click);
}

void RegisterButtonAPI()
{
    ccAddExternalObjectFunction("Button::Animate^4",            Sc_Button_Animate);
    ccAddExternalObjectFunction("Button::Click^1",              Sc_Button_Click);
	ccAddExternalObjectFunction("Button::GetText^1",            Sc_Button_GetText);
	ccAddExternalObjectFunction("Button::SetText^1",            Sc_Button_SetText);
	ccAddExternalObjectFunction("Button::get_ClipImage",        Sc_Button_GetClipImage);
	ccAddExternalObjectFunction("Button::set_ClipImage",        Sc_Button_SetClipImage);
	ccAddExternalObjectFunction("Button::get_Font",             Sc_Button_GetFont);
	ccAddExternalObjectFunction("Button::set_Font",             Sc_Button_SetFont);
	ccAddExternalObjectFunction("Button::get_Graphic",          Sc_Button_GetGraphic);
	ccAddExternalObjectFunction("Button::get_MouseOverGraphic", Sc_Button_GetMouseOverGraphic);
	ccAddExternalObjectFunction("Button::set_MouseOverGraphic", Sc_Button_SetMouseOverGraphic);
	ccAddExternalObjectFunction("Button::get_NormalGraphic",    Sc_Button_GetNormalGraphic);
	ccAddExternalObjectFunction("Button::set_NormalGraphic",    Sc_Button_SetNormalGraphic);
	ccAddExternalObjectFunction("Button::get_PushedGraphic",    Sc_Button_GetPushedGraphic);
	ccAddExternalObjectFunction("Button::set_PushedGraphic",    Sc_Button_SetPushedGraphic);
	ccAddExternalObjectFunction("Button::get_Text",             Sc_Button_GetText_New);
	ccAddExternalObjectFunction("Button::set_Text",             Sc_Button_SetText);
	ccAddExternalObjectFunction("Button::get_TextColor",        Sc_Button_GetTextColor);
	ccAddExternalObjectFunction("Button::set_TextColor",        Sc_Button_SetTextColor);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("Button::Animate^4",            (void*)Button_Animate);
    ccAddExternalFunctionForPlugin("Button::GetText^1",            (void*)Button_GetText);
    ccAddExternalFunctionForPlugin("Button::SetText^1",            (void*)Button_SetText);
    ccAddExternalFunctionForPlugin("Button::get_ClipImage",        (void*)Button_GetClipImage);
    ccAddExternalFunctionForPlugin("Button::set_ClipImage",        (void*)Button_SetClipImage);
    ccAddExternalFunctionForPlugin("Button::get_Font",             (void*)Button_GetFont);
    ccAddExternalFunctionForPlugin("Button::set_Font",             (void*)Button_SetFont);
    ccAddExternalFunctionForPlugin("Button::get_Graphic",          (void*)Button_GetGraphic);
    ccAddExternalFunctionForPlugin("Button::get_MouseOverGraphic", (void*)Button_GetMouseOverGraphic);
    ccAddExternalFunctionForPlugin("Button::set_MouseOverGraphic", (void*)Button_SetMouseOverGraphic);
    ccAddExternalFunctionForPlugin("Button::get_NormalGraphic",    (void*)Button_GetNormalGraphic);
    ccAddExternalFunctionForPlugin("Button::set_NormalGraphic",    (void*)Button_SetNormalGraphic);
    ccAddExternalFunctionForPlugin("Button::get_PushedGraphic",    (void*)Button_GetPushedGraphic);
    ccAddExternalFunctionForPlugin("Button::set_PushedGraphic",    (void*)Button_SetPushedGraphic);
    ccAddExternalFunctionForPlugin("Button::get_Text",             (void*)Button_GetText_New);
    ccAddExternalFunctionForPlugin("Button::set_Text",             (void*)Button_SetText);
    ccAddExternalFunctionForPlugin("Button::get_TextColor",        (void*)Button_GetTextColor);
    ccAddExternalFunctionForPlugin("Button::set_TextColor",        (void*)Button_SetTextColor);
}
