//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <algorithm>
#include <stdio.h>
#include "ac/button.h"
#include "ac/common.h"
#include "ac/gui.h"
#include "ac/view.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_translation.h"
#include "ac/object.h"
#include "ac/string.h"
#include "ac/viewframe.h"
#include "debug/debug_log.h"
#include "gui/animatingguibutton.h"
#include "gui/guimain.h"
#include "main/game_run.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern std::vector<ViewStruct> views;

// *** BUTTON FUNCTIONS

std::vector<AnimatingGUIButton> animbuts;

// Update the actual button's image from the current animation frame
void UpdateButtonState(const AnimatingGUIButton &abtn)
{
    // Assign view frame as normal image and reset all the rest
    guibuts[abtn.buttonid].SetImages(
        views[abtn.view].loops[abtn.loop].frames[abtn.frame].pic, 0, 0);
}

void Button_Animate(GUIButton *butt, int view, int loop, int speed, int repeat,
    int blocking, int direction, int sframe, int volume)
{
    int guin = butt->ParentId;
    int objn = butt->Id;

    view--; // convert to internal 0-based view ID
    ValidateViewAnimVLF("Button.Animate", view, loop, sframe);
    ValidateViewAnimParams("Button.Animate", repeat, blocking, direction);

    volume = Math::Clamp(volume, 0, 100);

    // if it's already animating, stop it
    FindAndRemoveButtonAnimation(guin, objn);

    int but_id = guis[guin].GetControlID(objn);
    AnimatingGUIButton abtn;
    abtn.ongui = guin;
    abtn.onguibut = objn;
    abtn.buttonid = but_id;
    abtn.view = view;
    abtn.loop = loop;
    abtn.speed = speed;
    abtn.repeat = (repeat != 0) ? ANIM_REPEAT : ANIM_ONCE; // for now, clamp to supported modes
    abtn.blocking = blocking;
    abtn.direction = direction;
    abtn.frame = SetFirstAnimFrame(view, loop, sframe, direction);
    abtn.wait = abtn.speed + views[abtn.view].loops[abtn.loop].frames[abtn.frame].speed;
    abtn.volume = volume;
    animbuts.push_back(abtn);
    // launch into the first frame, and play the first frame's sound
    UpdateButtonState(abtn);
    CheckViewFrame(abtn.view, abtn.loop, abtn.frame, volume);

    // Blocking animate
    if (blocking)
        GameLoopUntilButAnimEnd(guin, objn);
}

void Button_Animate4(GUIButton *butt, int view, int loop, int speed, int repeat) {
    Button_Animate(butt, view, loop, speed, repeat, IN_BACKGROUND, FORWARDS, 0, 100 /* full volume */);
}

void Button_Animate7(GUIButton *butt, int view, int loop, int speed, int repeat,
    int blocking, int direction, int sframe) {
    Button_Animate(butt, view, loop, speed, repeat, blocking, direction, sframe, 100 /* full volume */);
}

const char* Button_GetText_New(GUIButton *butt) {
    return CreateNewScriptString(butt->GetText());
}

void Button_GetText(GUIButton *butt, char *buffer) {
    snprintf(buffer, MAX_MAXSTRLEN, "%s", butt->GetText().GetCStr());
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
        butt->MarkChanged();
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
    if (butt->GetCurrentImage() < 0)
        return butt->GetNormalImage();
    return butt->GetCurrentImage();
}

int Button_GetMouseOverGraphic(GUIButton *butt) {
    return butt->GetMouseOverImage();
}

void Button_SetMouseOverGraphic(GUIButton *guil, int slotn) {
    debug_script_log("GUI %d Button %d mouseover set to slot %d", guil->ParentId, guil->Id, slotn);

    guil->SetMouseOverImage(slotn);
    FindAndRemoveButtonAnimation(guil->ParentId, guil->Id);
}

int Button_GetNormalGraphic(GUIButton *butt) {
    return butt->GetNormalImage();
}

void Button_SetNormalGraphic(GUIButton *guil, int slotn) {
    debug_script_log("GUI %d Button %d normal set to slot %d", guil->ParentId, guil->Id, slotn);
    // update the clickable area to the same size as the graphic
    int width, height;
    if (slotn < 0 || (size_t)slotn >= game.SpriteInfos.size())
    {
        width = 0;
        height = 0;
    }
    else
    {
        width = game.SpriteInfos[slotn].Width;
        height = game.SpriteInfos[slotn].Height;
    }

    if ((slotn != guil->GetNormalImage()) || (width != guil->GetWidth()) || (height != guil->GetHeight()))
    {
        guil->SetNormalImage(slotn);
        guil->SetSize(width, height);
    }

    FindAndRemoveButtonAnimation(guil->ParentId, guil->Id);
}

int Button_GetPushedGraphic(GUIButton *butt) {
    return butt->GetPushedImage();
}

void Button_SetPushedGraphic(GUIButton *guil, int slotn) {
    debug_script_log("GUI %d Button %d pushed set to slot %d", guil->ParentId, guil->Id, slotn);

    guil->SetPushedImage(slotn);
    FindAndRemoveButtonAnimation(guil->ParentId, guil->Id);
}

int Button_GetTextColor(GUIButton *butt) {
    return butt->TextColor;
}

void Button_SetTextColor(GUIButton *butt, int newcol) {
    if (butt->TextColor != newcol) {
        butt->TextColor = newcol;
        butt->MarkChanged();
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
bool UpdateAnimatingButton(int bu)
{
    AnimatingGUIButton &abtn = animbuts[bu];
    if (abtn.wait > 0) {
        abtn.wait--;
        return true;
    }
    if (!CycleViewAnim(abtn.view, abtn.loop, abtn.frame, !abtn.direction, abtn.repeat))
        return false;
    CheckViewFrame(abtn.view, abtn.loop, abtn.frame, abtn.volume);
    abtn.wait = abtn.speed + views[abtn.view].loops[abtn.loop].frames[abtn.frame].speed;
    UpdateButtonState(abtn);
    return true;
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
int FindButtonAnimation(int guin, int objn)
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
    int idx = FindButtonAnimation(guin, objn);
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
    return FindButtonAnimation(butt->ParentId, butt->Id) >= 0;
}

// NOTE: in correspondance to similar functions for Character & Object,
// GetView returns (view index + 1), while GetLoop and GetFrame return
// zero-based index and 0 in case of no animation.
int Button_GetAnimView(GUIButton *butt)
{
    int idx = FindButtonAnimation(butt->ParentId, butt->Id);
    return idx >= 0 ? animbuts[idx].view + 1 : 0;
}

int Button_GetAnimLoop(GUIButton *butt)
{
    int idx = FindButtonAnimation(butt->ParentId, butt->Id);
    return idx >= 0 ? animbuts[idx].loop : 0;
}

int Button_GetAnimFrame(GUIButton *butt)
{
    int idx = FindButtonAnimation(butt->ParentId, butt->Id);
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
        butt->MarkChanged();
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

// void | GUIButton *butt, int view, int loop, int speed, int repeat
RuntimeScriptValue Sc_Button_Animate4(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(GUIButton, Button_Animate4);
}

RuntimeScriptValue Sc_Button_Animate7(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT7(GUIButton, Button_Animate7);
}

RuntimeScriptValue Sc_Button_Animate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT8(GUIButton, Button_Animate);
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

RuntimeScriptValue Sc_Button_IsAnimating(void *self, const RuntimeScriptValue *params, int32_t param_count)
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

RuntimeScriptValue Sc_Button_GetAnimFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetAnimFrame);
}

RuntimeScriptValue Sc_Button_GetAnimLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetAnimLoop);
}

RuntimeScriptValue Sc_Button_GetAnimView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetAnimView);
}

void RegisterButtonAPI()
{
    ScFnRegister button_api[] = {
        { "Button::Animate^4",            API_FN_PAIR(Button_Animate4) },
        { "Button::Animate^7",            API_FN_PAIR(Button_Animate7) },
        { "Button::Animate^8",            API_FN_PAIR(Button_Animate) },
        { "Button::Click^1",              API_FN_PAIR(Button_Click) },
        { "Button::GetText^1",            API_FN_PAIR(Button_GetText) },
        { "Button::SetText^1",            API_FN_PAIR(Button_SetText) },
        { "Button::get_TextAlignment",    API_FN_PAIR(Button_GetTextAlignment) },
        { "Button::set_TextAlignment",    API_FN_PAIR(Button_SetTextAlignment) },
        { "Button::get_Animating",        API_FN_PAIR(Button_IsAnimating) },
        { "Button::get_ClipImage",        API_FN_PAIR(Button_GetClipImage) },
        { "Button::set_ClipImage",        API_FN_PAIR(Button_SetClipImage) },
        { "Button::get_Font",             API_FN_PAIR(Button_GetFont) },
        { "Button::set_Font",             API_FN_PAIR(Button_SetFont) },
        { "Button::get_Frame",            API_FN_PAIR(Button_GetAnimFrame) },
        { "Button::get_Graphic",          API_FN_PAIR(Button_GetGraphic) },
        { "Button::get_Loop",             API_FN_PAIR(Button_GetAnimLoop) },
        { "Button::get_MouseOverGraphic", API_FN_PAIR(Button_GetMouseOverGraphic) },
        { "Button::set_MouseOverGraphic", API_FN_PAIR(Button_SetMouseOverGraphic) },
        { "Button::get_NormalGraphic",    API_FN_PAIR(Button_GetNormalGraphic) },
        { "Button::set_NormalGraphic",    API_FN_PAIR(Button_SetNormalGraphic) },
        { "Button::get_PushedGraphic",    API_FN_PAIR(Button_GetPushedGraphic) },
        { "Button::set_PushedGraphic",    API_FN_PAIR(Button_SetPushedGraphic) },
        { "Button::get_Text",             API_FN_PAIR(Button_GetText_New) },
        { "Button::set_Text",             API_FN_PAIR(Button_SetText) },
        { "Button::get_TextColor",        API_FN_PAIR(Button_GetTextColor) },
        { "Button::set_TextColor",        API_FN_PAIR(Button_SetTextColor) },
        { "Button::get_View",             API_FN_PAIR(Button_GetAnimView) },
    };

    ccAddExternalFunctions(button_api);
}
