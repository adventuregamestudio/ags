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

using namespace AGS::Common;

extern GameSetupStruct game;
extern std::vector<ViewStruct> views;

// *** BUTTON FUNCTIONS

std::vector<AnimatingGUIButton> animbuts;

void Button_Animate(GUIButton *butt, int view, int loop, int speed, int repeat) {
    int guin = butt->ParentId;
    int objn = butt->Id;

    if ((view < 1) || (view > game.numviews))
        quit("!AnimateButton: invalid view specified");
    view--;

    if ((loop < 0) || (loop >= views[view].numLoops))
        quit("!AnimateButton: invalid loop specified for view");

    // if it's already animating, stop it
    FindAndRemoveButtonAnimation(guin, objn);

    int buttonId = guis[guin].GetControlID(objn);

    guibuts[buttonId].PushedImage = 0;
    guibuts[buttonId].MouseOverImage = 0;

    AnimatingGUIButton abtn;
    abtn.ongui = guin;
    abtn.onguibut = objn;
    abtn.buttonid = buttonId;
    abtn.view = view;
    abtn.loop = loop;
    abtn.speed = speed;
    abtn.repeat = repeat;
    abtn.frame = -1;
    abtn.wait = 0;
    animbuts.push_back(abtn);
    // launch into the first frame
    if (UpdateAnimatingButton(animbuts.size() - 1))
    {
        debug_script_warn("AnimateButton: no frames to animate (button: %s, view: %d, loop: %d)",
            butt->GetScriptName().GetCStr(), view, loop);
        StopButtonAnimation(animbuts.size() - 1);
    }
}

const char* Button_GetText_New(GUIButton *butt) {
    return CreateNewScriptString(butt->GetText());
}

void Button_GetText(GUIButton *butt, char *buffer) {
    strcpy(buffer, butt->GetText().GetCStr());
}

void Button_SetText(GUIButton *butt, const char *newtx) {
    newtx = get_translation(newtx);

    if (butt->GetText() != newtx) {
        butt->SetText(newtx);
    }
}

void Button_SetFont(GUIButton *butt, int newFont) {
    if ((newFont < 0) || (newFont >= game.numfonts))
        quit("!Button.Font: invalid font number.");

    if (butt->Font != newFont) {
        butt->Font = newFont;
        butt->NotifyParentChanged();
    }
}

int Button_GetFont(GUIButton *butt) {
    return butt->Font;
}

int Button_GetClipImage(GUIButton *butt) {
    return butt->IsClippingImage() ? 1 : 0;
}

void Button_SetClipImage(GUIButton *butt, int newval) {
    if (butt->IsClippingImage() != (newval != 0))
    {
        butt->SetClipImage(newval != 0);
    }
}

int Button_GetGraphic(GUIButton *butt) {
    // return currently displayed pic
    if (butt->CurrentImage < 0)
        return butt->Image;
    return butt->CurrentImage;
}

int Button_GetMouseOverGraphic(GUIButton *butt) {
    return butt->MouseOverImage;
}

void Button_SetMouseOverGraphic(GUIButton *guil, int slotn) {
    debug_script_log("GUI %d Button %d mouseover set to slot %d", guil->ParentId, guil->Id, slotn);

    if ((guil->IsMouseOver != 0) && (guil->IsPushed == 0))
        guil->CurrentImage = slotn;
    guil->MouseOverImage = slotn;

    guil->NotifyParentChanged();
    FindAndRemoveButtonAnimation(guil->ParentId, guil->Id);
}

int Button_GetNormalGraphic(GUIButton *butt) {
    return butt->Image;
}

void Button_SetNormalGraphic(GUIButton *guil, int slotn) {
    debug_script_log("GUI %d Button %d normal set to slot %d", guil->ParentId, guil->Id, slotn);
    // normal pic - update if mouse is not over, or if there's no MouseOverImage
    if (((guil->IsMouseOver == 0) || (guil->MouseOverImage < 1)) && (guil->IsPushed == 0))
        guil->CurrentImage = slotn;
    guil->Image = slotn;
    // update the clickable area to the same size as the graphic
    if (slotn < 0 || (size_t)slotn >= game.SpriteInfos.size())
    {
        guil->Width = 0;
        guil->Height = 0;
    }
    else
    {
        guil->Width = game.SpriteInfos[slotn].Width;
        guil->Height = game.SpriteInfos[slotn].Height;
    }

    guil->NotifyParentChanged();
    FindAndRemoveButtonAnimation(guil->ParentId, guil->Id);
}

int Button_GetPushedGraphic(GUIButton *butt) {
    return butt->PushedImage;
}

void Button_SetPushedGraphic(GUIButton *guil, int slotn) {
    debug_script_log("GUI %d Button %d pushed set to slot %d", guil->ParentId, guil->Id, slotn);

    if (guil->IsPushed)
        guil->CurrentImage = slotn;
    guil->PushedImage = slotn;

    guil->NotifyParentChanged();
    FindAndRemoveButtonAnimation(guil->ParentId, guil->Id);
}

int Button_GetTextColor(GUIButton *butt) {
    return butt->TextColor;
}

void Button_SetTextColor(GUIButton *butt, int newcol) {
    if (butt->TextColor != newcol) {
        butt->TextColor = newcol;
        butt->NotifyParentChanged();
    }
}

// ** start animating buttons code

size_t GetAnimatingButtonCount()
{
    return animbuts.size();
}

AnimatingGUIButton *GetAnimatingButtonByIndex(int idxn)
{
    return idxn >= 0 && (size_t)idxn < animbuts.size() ?
        &animbuts[idxn] : nullptr;
}

void AddButtonAnimation(const AnimatingGUIButton &abtn)
{
    animbuts.push_back(abtn);
}

// returns 1 if animation finished
int UpdateAnimatingButton(int bu) {
    AnimatingGUIButton &abtn = animbuts[bu];

    if (abtn.wait > 0) {
        abtn.wait--;
        return 0;
    }
    ViewStruct *tview = &views[abtn.view];

    abtn.frame++;

    if (abtn.frame >= tview->loops[abtn.loop].numFrames) 
    {
        if (tview->loops[abtn.loop].RunNextLoop()) {
            // go to next loop
            abtn.loop++;
            abtn.frame = 0;
        }
        else if (abtn.repeat) {
            abtn.frame = 0;
            // multi-loop anim, go back
            while ((abtn.loop > 0) && 
                (tview->loops[abtn.loop - 1].RunNextLoop()))
                abtn.loop--;
        }
        else
            return 1;
    }

    CheckViewFrame(abtn.view, abtn.loop, abtn.frame);

    // update the button's image
    guibuts[abtn.buttonid].Image = tview->loops[abtn.loop].frames[abtn.frame].pic;
    guibuts[abtn.buttonid].CurrentImage = guibuts[abtn.buttonid].Image;
    guibuts[abtn.buttonid].PushedImage = 0;
    guibuts[abtn.buttonid].MouseOverImage = 0;
    guibuts[abtn.buttonid].NotifyParentChanged();

    abtn.wait = abtn.speed + tview->loops[abtn.loop].frames[abtn.frame].speed;
    return 0;
}

void StopButtonAnimation(int idxn) {
    animbuts.erase(animbuts.begin() + idxn);
}

void RemoveAllButtonAnimations()
{
    animbuts.clear();
}

// Returns the index of the AnimatingGUIButton object corresponding to the
// given button ID; returns -1 if no such animation exists
int FindAnimatedButton(int guin, int objn)
{
    for (size_t i = 0; i < animbuts.size(); ++i)
    {
        if (animbuts[i].ongui == guin && animbuts[i].onguibut == objn)
            return i;
    }
    return -1;
}

void FindAndRemoveButtonAnimation(int guin, int objn)
{
    int idx = FindAnimatedButton(guin, objn);
    if (idx >= 0)
        StopButtonAnimation(idx);
}

// ** end animating buttons code

void Button_Click(GUIButton *butt, int mbut)
{
    process_interface_click(butt->ParentId, butt->Id, mbut);
}

bool Button_IsAnimating(GUIButton *butt)
{
    return FindAnimatedButton(butt->ParentId, butt->Id) >= 0;
}

// NOTE: in correspondance to similar functions for Character & Object,
// GetView returns (view index + 1), while GetLoop and GetFrame return
// zero-based index and 0 in case of no animation.
int Button_GetAnimView(GUIButton *butt)
{
    int idx = FindAnimatedButton(butt->ParentId, butt->Id);
    return idx >= 0 ? animbuts[idx].view + 1 : 0;
}

int Button_GetAnimLoop(GUIButton *butt)
{
    int idx = FindAnimatedButton(butt->ParentId, butt->Id);
    return idx >= 0 ? animbuts[idx].loop : 0;
}

int Button_GetAnimFrame(GUIButton *butt)
{
    int idx = FindAnimatedButton(butt->ParentId, butt->Id);
    return idx >= 0 ? animbuts[idx].frame : 0;
}

int Button_GetTextAlignment(GUIButton *butt)
{
    return butt->TextAlignment;
}

void Button_SetTextAlignment(GUIButton *butt, int align)
{
    if (butt->TextAlignment != align) {
        butt->TextAlignment = (FrameAlignment)align;
        butt->NotifyParentChanged();
    }
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

RuntimeScriptValue Sc_Button_GetAnimating(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(GUIButton, Button_IsAnimating);
}

RuntimeScriptValue Sc_Button_GetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetTextAlignment);
}

RuntimeScriptValue Sc_Button_SetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetTextAlignment);
}

RuntimeScriptValue Sc_Button_GetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetAnimFrame);
}

RuntimeScriptValue Sc_Button_GetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetAnimLoop);
}

RuntimeScriptValue Sc_Button_GetView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetAnimView);
}

void RegisterButtonAPI()
{
    ccAddExternalObjectFunction("Button::Animate^4",            Sc_Button_Animate);
    ccAddExternalObjectFunction("Button::Click^1",              Sc_Button_Click);
    ccAddExternalObjectFunction("Button::GetText^1",            Sc_Button_GetText);
    ccAddExternalObjectFunction("Button::SetText^1",            Sc_Button_SetText);
    ccAddExternalObjectFunction("Button::get_TextAlignment",    Sc_Button_GetTextAlignment);
    ccAddExternalObjectFunction("Button::set_TextAlignment",    Sc_Button_SetTextAlignment);
    ccAddExternalObjectFunction("Button::get_Animating",        Sc_Button_GetAnimating);
    ccAddExternalObjectFunction("Button::get_ClipImage",        Sc_Button_GetClipImage);
    ccAddExternalObjectFunction("Button::set_ClipImage",        Sc_Button_SetClipImage);
    ccAddExternalObjectFunction("Button::get_Font",             Sc_Button_GetFont);
    ccAddExternalObjectFunction("Button::set_Font",             Sc_Button_SetFont);
    ccAddExternalObjectFunction("Button::get_Frame",            Sc_Button_GetFrame);
    ccAddExternalObjectFunction("Button::get_Graphic",          Sc_Button_GetGraphic);
    ccAddExternalObjectFunction("Button::get_Loop",             Sc_Button_GetLoop);
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
    ccAddExternalObjectFunction("Button::get_View",             Sc_Button_GetView);

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
