//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_translation.h"
#include "ac/object.h"
#include "ac/string.h"
#include "ac/viewframe.h"
#include "ac/dynobj/cc_guicontrol.h"
#include "debug/debug_log.h"
#include "gui/animatingguibutton.h"
#include "gui/guimain.h"
#include "script/script.h"
#include "main/game_run.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern std::vector<ViewStruct> views;
extern CCGUIButton ccDynamicGUIButton;

// *** BUTTON FUNCTIONS

std::vector<AnimatingGUIButton> animbuts;

// Update the actual button's image from the current animation frame
void UpdateButtonState(const AnimatingGUIButton &abtn)
{
    // Assign view frame as normal image and reset all the rest
    const auto &vf = views[abtn.view].loops[abtn.loop].frames[abtn.frame];
    guibuts[abtn.buttonid].SetImages(
        vf.pic, 0, 0, vf.flags, vf.xoffs, vf.yoffs);
}

void ABut_CheckViewFrame(AnimatingGUIButton &abtn)
{
    ObjectEvent objevt(kScTypeGame, RuntimeScriptValue().SetScriptObject(&guibuts[abtn.buttonid], &ccDynamicGUIButton));
    CheckViewFrame(abtn.view, abtn.loop, abtn.frame, abtn.anim.AudioVolume,
        objevt, &guibuts[abtn.buttonid].GetEvents(), kButtonEvent_OnFrameEvent);
    UpdateButtonState(abtn);
}

bool Button_RunFrameEventImpl(GUIButton &btn, int view, int loop, int frame)
{
    ObjectEvent objevt(kScTypeGame, RuntimeScriptValue().SetScriptObject(&btn, &ccDynamicGUIButton));
    return RunViewFrameEvent(view, loop, frame,
        objevt, &btn.GetEvents(), kButtonEvent_OnFrameEvent);
}

void Button_Animate(GUIButton *butt, int view, int loop, int speed, int repeat,
    int blocking, int direction, int sframe, int volume)
{
    int guin = butt->GetParentID();
    int objn = butt->GetID();

    view--; // convert to internal 0-based view ID
    ValidateViewAnimVLF("Button.Animate", butt->GetName().GetCStr(), view, loop, sframe);
    ValidateViewAnimParams("Button.Animate", butt->GetName().GetCStr(), blocking, repeat, direction);

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
    abtn.anim = ViewAnimateParams(static_cast<AnimFlowStyle>(repeat), static_cast<AnimFlowDirection>(direction), speed, volume);
    abtn.frame = SetFirstAnimFrame(view, loop, sframe, static_cast<AnimFlowDirection>(direction));
    abtn.wait = abtn.anim.Delay + views[abtn.view].loops[abtn.loop].frames[abtn.frame].speed;
    // launch into the first frame, and play the first frame's sound
    ABut_CheckViewFrame(abtn);
    animbuts.push_back(std::move(abtn));

    // Blocking animate
    if (blocking)
    {
        // Wait until the animation completes.
        // Override disabled effect for the animating button and its parent GUI.
        GUI::SetExcludedFromDisabled(butt, true);
        GameLoopUntilButAnimEnd(guin, objn);
        GUI::SetExcludedFromDisabled(butt, false);
    }
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
    if (butt->GetText() != newtx) {
        butt->SetText(newtx);
    }
}

void Button_SetFont(GUIButton *butt, int newFont) {
    newFont = ValidateFontNumber("Button.Font", newFont);
    butt->SetFont(newFont);
}

int Button_GetFont(GUIButton *butt) {
    return butt->GetFont();
}

int Button_GetColorStyle(GUIButton *butt)
{
    if (butt->IsDynamicColors())
    {
        if (butt->IsFlatStyle())
            return eGUIButtonDynamicFlat;
        else
            return eGUIButtonDynamic;
    }
    else
    {
        return eGUIButtonDefault;
    }
}

void Button_SetColorStyle(GUIButton *butt, int style)
{
    const ScriptButtonColorStyle old_style = static_cast<ScriptButtonColorStyle>(Button_GetColorStyle(butt));
    if (old_style != style)
    {
        switch (style)
        {
        case eGUIButtonDefault:
            butt->SetDynamicColors(false);
            butt->SetFlatStyle(false);
            break;
        case eGUIButtonDynamic:
            butt->SetDynamicColors(true);
            butt->SetFlatStyle(false);
            break;
        case eGUIButtonDynamicFlat:
            butt->SetDynamicColors(true);
            butt->SetFlatStyle(true);
            break;
        }
    }
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

int Button_GetGraphicFlip(GUIButton *butt)
{
    return butt->GetImageFlip();
}

void Button_SetGraphicFlip(GUIButton *butt, int flip)
{
    butt->SetImageFlip((GraphicFlip)flip);
}

int Button_GetMouseOverBackgroundColor(GUIButton *butt)
{
    return butt->GetMouseOverBackColor();
}

void Button_SetMouseOverBackgroundColor(GUIButton *butt, int color)
{
    butt->SetMouseOverBackColor(color);
}

int Button_GetPushedBackgroundColor(GUIButton *butt)
{
    return butt->GetPushedBackColor();
}

void Button_SetPushedBackgroundColor(GUIButton *butt, int color)
{
    butt->SetPushedBackColor(color);
}

int Button_GetMouseOverBorderColor(GUIButton *butt)
{
    return butt->GetMouseOverBorderColor();
}

void Button_SetMouseOverBorderColor(GUIButton *butt, int color)
{
    butt->SetMouseOverBorderColor(color);
}

int Button_GetPushedBorderColor(GUIButton *butt)
{
    return butt->GetPushedBorderColor();
}

void Button_SetPushedBorderColor(GUIButton *butt, int color)
{
    butt->SetPushedBorderColor(color);
}

int Button_GetMouseOverGraphic(GUIButton *butt) {
    return butt->GetMouseOverImage();
}

void Button_SetMouseOverGraphic(GUIButton *butt, int slotn)
{
    slotn = std::max(0, slotn);
    if (butt->GetMouseOverImage() != slotn)
        debug_script_log("GUI %d Button %d mouseover set to slot %d", butt->GetParentID(), butt->GetID(), slotn);
    butt->SetMouseOverImage(slotn);
    FindAndRemoveButtonAnimation(butt->GetParentID(), butt->GetID());
}

int Button_GetNormalGraphic(GUIButton *butt) {
    return butt->GetNormalImage();
}

void Button_SetNormalGraphic(GUIButton *butt, int slotn)
{
    slotn = std::max(0, slotn);
    if (butt->GetNormalImage() != slotn)
        debug_script_log("GUI %d Button %d normal set to slot %d", butt->GetParentID(), butt->GetID(), slotn);
    // NormalGraphic = 0 will turn the Button into a standard colored button
    if (slotn == 0)
    {
        butt->SetNormalImage(slotn);
    }
    // Any other sprite - update the clickable area to the same size as the graphic
    else
    {
        const int width = static_cast<size_t>(slotn) < game.SpriteInfos.size() ? game.SpriteInfos[slotn].Width : 0;
        const int height = static_cast<size_t>(slotn) < game.SpriteInfos.size() ? game.SpriteInfos[slotn].Height : 0;
        butt->SetNormalImage(slotn);
        butt->SetSize(width, height);
    }

    FindAndRemoveButtonAnimation(butt->GetParentID(), butt->GetID());
}

int Button_GetPushedGraphic(GUIButton *butt) {
    return butt->GetPushedImage();
}

void Button_SetPushedGraphic(GUIButton *butt, int slotn)
{
    slotn = std::max(0, slotn);
    if (butt->GetPushedImage() != slotn)
        debug_script_log("GUI %d Button %d pushed set to slot %d", butt->GetParentID(), butt->GetID(), slotn);
    butt->SetPushedImage(slotn);
    FindAndRemoveButtonAnimation(butt->GetParentID(), butt->GetID());
}

int Button_GetBorderShadeColor(GUIButton *butt)
{
    return butt->GetBorderShadeColor();
}

void Button_SetBorderShadeColor(GUIButton *butt, int color)
{
    butt->SetBorderShadeColor(color);
}

int Button_GetTextColor(GUIButton *butt)
{
    return butt->GetTextColor();
}

void Button_SetTextColor(GUIButton *butt, int color)
{
    butt->SetTextColor(color);
}

int Button_GetMouseOverTextColor(GUIButton *butt)
{
    return butt->GetMouseOverTextColor();
}

void Button_SetMouseOverTextColor(GUIButton *butt, int color)
{
    butt->SetMouseOverTextColor(color);
}

int Button_GetPushedTextColor(GUIButton *butt)
{
    return butt->GetPushedTextColor();
}

void Button_SetPushedTextColor(GUIButton *butt, int color)
{
    butt->SetPushedTextColor(color);
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
    if (!CycleViewAnim(abtn.view, abtn.loop, abtn.frame, abtn.anim))
        return false;

    ABut_CheckViewFrame(abtn);
    abtn.wait = abtn.anim.Delay + views[abtn.view].loops[abtn.loop].frames[abtn.frame].speed;
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
    process_interface_click(butt->GetParentID(), butt->GetID(), mbut);
}

bool Button_RunFrameEvent(GUIButton *butt, int view, int loop, int frame)
{
    view--; // convert to internal 0-based view ID
    AssertFrame("Button.RunFrameEvent", butt->GetName().GetCStr(), view, loop, frame);
    return Button_RunFrameEventImpl(*butt, view, loop, frame);
}

bool Button_IsAnimating(GUIButton *butt)
{
    return FindButtonAnimation(butt->GetParentID(), butt->GetID()) >= 0;
}

// NOTE: in correspondance to similar functions for Character & Object,
// GetView returns (view index + 1), while GetLoop and GetFrame return
// zero-based index and 0 in case of no animation.
int Button_GetAnimView(GUIButton *butt)
{
    int idx = FindButtonAnimation(butt->GetParentID(), butt->GetID());
    return idx >= 0 ? animbuts[idx].view + 1 : 0;
}

int Button_GetAnimLoop(GUIButton *butt)
{
    int idx = FindButtonAnimation(butt->GetParentID(), butt->GetID());
    return idx >= 0 ? animbuts[idx].loop : 0;
}

int Button_GetAnimFrame(GUIButton *butt)
{
    int idx = FindButtonAnimation(butt->GetParentID(), butt->GetID());
    return idx >= 0 ? animbuts[idx].frame : 0;
}

int Button_GetTextAlignment(GUIButton *butt)
{
    return butt->GetTextAlignment();
}

void Button_SetTextAlignment(GUIButton *butt, int align)
{
    butt->SetTextAlignment((FrameAlignment)align);
}

int Button_GetTextPaddingHorizontal(GUIButton *butt)
{
    return butt->GetPaddingX();
}

void Button_SetTextPaddingHorizontal(GUIButton *butt, int pad)
{
    // Original Button.TextPadding included button's border
    if (loaded_game_file_version < kGameVersion_363_04)
        pad--;
    butt->SetPaddingX(pad);
}

int Button_GetTextPaddingVertical(GUIButton *butt)
{
    return butt->GetPaddingY();
}

void Button_SetTextPaddingVertical(GUIButton *butt, int pad)
{
    // Original Button.TextPadding included button's border
    if (loaded_game_file_version < kGameVersion_363_04)
        pad--;
    butt->SetPaddingY(pad);
}

bool Button_GetWrapText(GUIButton *butt)
{
    return butt->IsWrapText();
}

void Button_SetWrapText(GUIButton *butt, bool wrap)
{
    if (butt->IsWrapText() != wrap)
    {
        butt->SetWrapText(wrap);
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

RuntimeScriptValue Sc_Button_GetColorStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetColorStyle);
}

RuntimeScriptValue Sc_Button_SetColorStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetColorStyle);
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

RuntimeScriptValue Sc_Button_GetGraphicFlip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetGraphicFlip);
}

RuntimeScriptValue Sc_Button_SetGraphicFlip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetGraphicFlip);
}

RuntimeScriptValue Sc_Button_GetMouseOverBackgroundColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetMouseOverBackgroundColor);
}

RuntimeScriptValue Sc_Button_SetMouseOverBackgroundColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetMouseOverBackgroundColor);
}

RuntimeScriptValue Sc_Button_GetPushedBackgroundColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetPushedBackgroundColor);
}

RuntimeScriptValue Sc_Button_SetPushedBackgroundColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetPushedBackgroundColor);
}

RuntimeScriptValue Sc_Button_GetMouseOverBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetMouseOverBorderColor);
}

RuntimeScriptValue Sc_Button_SetMouseOverBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetMouseOverBorderColor);
}

RuntimeScriptValue Sc_Button_GetPushedBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetPushedBorderColor);
}

RuntimeScriptValue Sc_Button_SetPushedBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetPushedBorderColor);
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

RuntimeScriptValue Sc_Button_GetBorderShadeColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetBorderShadeColor);
}

RuntimeScriptValue Sc_Button_SetBorderShadeColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetBorderShadeColor);
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

RuntimeScriptValue Sc_Button_GetMouseOverTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetMouseOverTextColor);
}

RuntimeScriptValue Sc_Button_SetMouseOverTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetMouseOverTextColor);
}

RuntimeScriptValue Sc_Button_GetPushedTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetPushedTextColor);
}

RuntimeScriptValue Sc_Button_SetPushedTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetPushedTextColor);
}

RuntimeScriptValue Sc_Button_Click(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_Click);
}

RuntimeScriptValue Sc_Button_RunFrameEvent(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(GUIButton, Button_RunFrameEvent);
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

RuntimeScriptValue Sc_Button_GetWrapText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(GUIButton, Button_GetWrapText);
}

RuntimeScriptValue Sc_Button_SetWrapText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(GUIButton, Button_SetWrapText);
}

RuntimeScriptValue Sc_Button_GetTextPaddingHorizontal(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetTextPaddingHorizontal);
}

RuntimeScriptValue Sc_Button_SetTextPaddingHorizontal(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetTextPaddingHorizontal);
}

RuntimeScriptValue Sc_Button_GetTextPaddingVertical(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetTextPaddingVertical);
}

RuntimeScriptValue Sc_Button_SetTextPaddingVertical(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetTextPaddingVertical);
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
        { "Button::RunFrameEvent^3",      API_FN_PAIR(Button_RunFrameEvent) },
        { "Button::GetText^1",            API_FN_PAIR(Button_GetText) },
        { "Button::SetText^1",            API_FN_PAIR(Button_SetText) },
        { "Button::get_TextAlignment",    API_FN_PAIR(Button_GetTextAlignment) },
        { "Button::set_TextAlignment",    API_FN_PAIR(Button_SetTextAlignment) },
        { "Button::get_TextPaddingHorizontal", API_FN_PAIR(Button_GetTextPaddingHorizontal) },
        { "Button::set_TextPaddingHorizontal", API_FN_PAIR(Button_SetTextPaddingHorizontal) },
        { "Button::get_TextPaddingVertical", API_FN_PAIR(Button_GetTextPaddingVertical) },
        { "Button::set_TextPaddingVertical", API_FN_PAIR(Button_SetTextPaddingVertical) },
        { "Button::get_WrapText",         API_FN_PAIR(Button_GetWrapText) },
        { "Button::set_WrapText",         API_FN_PAIR(Button_SetWrapText) },
        { "Button::get_Animating",        API_FN_PAIR(Button_IsAnimating) },
        { "Button::get_ColorStyle",       API_FN_PAIR(Button_GetColorStyle) },
        { "Button::set_ColorStyle",       API_FN_PAIR(Button_SetColorStyle) },
        { "Button::get_ClipImage",        API_FN_PAIR(Button_GetClipImage) },
        { "Button::set_ClipImage",        API_FN_PAIR(Button_SetClipImage) },
        { "Button::get_Font",             API_FN_PAIR(Button_GetFont) },
        { "Button::set_Font",             API_FN_PAIR(Button_SetFont) },
        { "Button::get_Frame",            API_FN_PAIR(Button_GetAnimFrame) },
        { "Button::get_Graphic",          API_FN_PAIR(Button_GetGraphic) },
        { "Button::get_GraphicFlip",      API_FN_PAIR(Button_GetGraphicFlip) },
        { "Button::set_GraphicFlip",      API_FN_PAIR(Button_SetGraphicFlip) },
        { "Button::get_Loop",             API_FN_PAIR(Button_GetAnimLoop) },
        { "Button::get_BorderShadeColor", API_FN_PAIR(Button_GetBorderShadeColor) },
        { "Button::set_BorderShadeColor", API_FN_PAIR(Button_SetBorderShadeColor) },
        { "Button::get_MouseOverBackgroundColor", API_FN_PAIR(Button_GetMouseOverBackgroundColor) },
        { "Button::set_MouseOverBackgroundColor", API_FN_PAIR(Button_SetMouseOverBackgroundColor) },
        { "Button::get_PushedBackgroundColor", API_FN_PAIR(Button_GetPushedBackgroundColor) },
        { "Button::set_PushedBackgroundColor", API_FN_PAIR(Button_SetPushedBackgroundColor) },
        { "Button::get_MouseOverBorderColor", API_FN_PAIR(Button_GetMouseOverBorderColor) },
        { "Button::set_MouseOverBorderColor", API_FN_PAIR(Button_SetMouseOverBorderColor) },
        { "Button::get_PushedBorderColor", API_FN_PAIR(Button_GetPushedBorderColor) },
        { "Button::set_PushedBorderColor", API_FN_PAIR(Button_SetPushedBorderColor) },
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
        { "Button::get_MouseOverTextColor", API_FN_PAIR(Button_GetMouseOverTextColor) },
        { "Button::set_MouseOverTextColor", API_FN_PAIR(Button_SetMouseOverTextColor) },
        { "Button::get_PushedTextColor",  API_FN_PAIR(Button_GetPushedTextColor) },
        { "Button::set_PushedTextColor",  API_FN_PAIR(Button_SetPushedTextColor) },
        { "Button::get_View",             API_FN_PAIR(Button_GetAnimView) },
    };

    ccAddExternalFunctions(button_api);
}
