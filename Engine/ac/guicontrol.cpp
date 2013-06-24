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

#include <stdio.h>
#include "ac/common.h"
#include "ac/guicontrol.h"
#include "ac/global_gui.h"
#include "debug/debug_log.h"
#include "game/game_objects.h"
#include "game/script_objects.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guimain.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_gui.h"
#include "ac/dynobj/cc_guiobject.h"

GuiObject *GetGUIControlAtLocation(int xx, int yy) {
    int guinum = GetGUIAt(xx, yy);
    if (guinum == -1)
        return NULL;

    multiply_up_coordinates(&xx, &yy);

    int oldmousex = mousex, oldmousey = mousey;
    mousex = xx - guis[guinum].GetX();
    mousey = yy - guis[guinum].GetY();
    int toret = guis[guinum].FindControlUnderMouse(0, false);
    mousex = oldmousex;
    mousey = oldmousey;
    if (toret < 0)
        return NULL;

    return guis[guinum].Controls[toret];
}

int GUIControl_GetVisible(GuiObject *guio) {
  return guio->IsVisible();
}

void GUIControl_SetVisible(GuiObject *guio, int visible) 
{
  if ((visible != 0) != guio->IsVisible()) 
  {
    if (visible)
      guio->Show();
    else
      guio->Hide();

    guis[guio->ParentId].OnControlPositionChanged();
    guis_need_update = 1;
  }
}

int GUIControl_GetClickable(GuiObject *guio) {
  if (guio->IsClickable())
    return 1;
  return 0;
}

void GUIControl_SetClickable(GuiObject *guio, int enabled) {
  if (enabled)
    guio->SetClickable(true);
  else
    guio->SetClickable(false);

  guis[guio->ParentId].OnControlPositionChanged();
  guis_need_update = 1;
}

int GUIControl_GetEnabled(GuiObject *guio) {
  if (guio->IsDisabled())
    return 0;
  return 1;
}

void GUIControl_SetEnabled(GuiObject *guio, int enabled) {
  if (enabled)
    guio->Enable();
  else
    guio->Disable();

  guis[guio->ParentId].OnControlPositionChanged();
  guis_need_update = 1;
}


int GUIControl_GetID(GuiObject *guio) {
  return guio->Id;
}

ScriptGUI* GUIControl_GetOwningGUI(GuiObject *guio) {
  return &scrGui[guio->ParentId];
}

GuiButton* GUIControl_GetAsButton(GuiObject *guio) {
  if (guis[guio->ParentId].GetControlType(guio->Id) != Common::kGuiButton)
    return NULL;

  return (GuiButton*)guio;
}

GuiInvWindow* GUIControl_GetAsInvWindow(GuiObject *guio) {
  if (guis[guio->ParentId].GetControlType(guio->Id) != Common::kGuiInvWindow)
    return NULL;

  return (GuiInvWindow*)guio;
}

GuiLabel* GUIControl_GetAsLabel(GuiObject *guio) {
  if (guis[guio->ParentId].GetControlType(guio->Id) != Common::kGuiLabel)
    return NULL;

  return (GuiLabel*)guio;
}

GuiListBox* GUIControl_GetAsListBox(GuiObject *guio) {
  if (guis[guio->ParentId].GetControlType(guio->Id) != Common::kGuiListBox)
    return NULL;

  return (GuiListBox*)guio;
}

GuiSlider* GUIControl_GetAsSlider(GuiObject *guio) {
  if (guis[guio->ParentId].GetControlType(guio->Id) != Common::kGuiSlider)
    return NULL;

  return (GuiSlider*)guio;
}

GuiTextBox* GUIControl_GetAsTextBox(GuiObject *guio) {
  if (guis[guio->ParentId].GetControlType(guio->Id) != Common::kGuiTextBox)
    return NULL;

  return (GuiTextBox*)guio;
}

int GUIControl_GetX(GuiObject *guio) {
  return divide_down_coordinate(guio->GetX());
}

void GUIControl_SetX(GuiObject *guio, int xx) {
  guio->SetX(multiply_up_coordinate(xx));
  guis[guio->ParentId].OnControlPositionChanged();
  guis_need_update = 1;
}

int GUIControl_GetY(GuiObject *guio) {
  return divide_down_coordinate(guio->GetY());
}

void GUIControl_SetY(GuiObject *guio, int yy) {
  guio->SetY(multiply_up_coordinate(yy));
  guis[guio->ParentId].OnControlPositionChanged();
  guis_need_update = 1;
}

void GUIControl_SetPosition(GuiObject *guio, int xx, int yy) {
  GUIControl_SetX(guio, xx);
  GUIControl_SetY(guio, yy);
}


int GUIControl_GetWidth(GuiObject *guio) {
  return divide_down_coordinate(guio->GetWidth());
}

void GUIControl_SetWidth(GuiObject *guio, int newwid) {
  guio->SetWidth(multiply_up_coordinate(newwid));
  guio->OnResized();
  guis[guio->ParentId].OnControlPositionChanged();
  guis_need_update = 1;
}

int GUIControl_GetHeight(GuiObject *guio) {
  return divide_down_coordinate(guio->GetHeight());
}

void GUIControl_SetHeight(GuiObject *guio, int newhit) {
  guio->SetHeight(multiply_up_coordinate(newhit));
  guio->OnResized();
  guis[guio->ParentId].OnControlPositionChanged();
  guis_need_update = 1;
}

void GUIControl_SetSize(GuiObject *guio, int newwid, int newhit) {
  if ((newwid < 2) || (newhit < 2))
    quit("!SetGUIObjectSize: new size is too small (must be at least 2x2)");

  DEBUG_CONSOLE("SetGUIObject %d,%d size %d,%d", guio->ParentId, guio->Id, newwid, newhit);
  GUIControl_SetWidth(guio, newwid);
  GUIControl_SetHeight(guio, newhit);
}

void GUIControl_SendToBack(GuiObject *guio) {
  if (guis[guio->ParentId].SendControlToBack(guio->Id))
    guis_need_update = 1;
}

void GUIControl_BringToFront(GuiObject *guio) {
  if (guis[guio->ParentId].BringControlToFront(guio->Id))
    guis_need_update = 1;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// void (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_BringToFront(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GuiObject, GUIControl_BringToFront);
}

// GuiObject *(int xx, int yy)
RuntimeScriptValue Sc_GetGUIControlAtLocation(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(GuiObject, ccDynamicGUIObject, GetGUIControlAtLocation);
}

// void (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_SendToBack(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GuiObject, GUIControl_SendToBack);
}

// void (GuiObject *guio, int xx, int yy)
RuntimeScriptValue Sc_GUIControl_SetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(GuiObject, GUIControl_SetPosition);
}

// void (GuiObject *guio, int newwid, int newhit)
RuntimeScriptValue Sc_GUIControl_SetSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(GuiObject, GUIControl_SetSize);
}

// GuiButton* (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsButton(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GuiObject, GuiButton, ccDynamicGUI, GUIControl_GetAsButton);
}

// GuiInvWindow* (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsInvWindow(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GuiObject, GuiInvWindow, ccDynamicGUI, GUIControl_GetAsInvWindow);
}

// GuiLabel* (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsLabel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GuiObject, GuiLabel, ccDynamicGUI, GUIControl_GetAsLabel);
}

// GuiListBox* (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsListBox(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GuiObject, GuiListBox, ccDynamicGUI, GUIControl_GetAsListBox);
}

// GuiSlider* (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsSlider(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GuiObject, GuiSlider, ccDynamicGUI, GUIControl_GetAsSlider);
}

// GuiTextBox* (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsTextBox(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GuiObject, GuiTextBox, ccDynamicGUI, GUIControl_GetAsTextBox);
}

// int (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiObject, GUIControl_GetClickable);
}

// void (GuiObject *guio, int enabled)
RuntimeScriptValue Sc_GUIControl_SetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiObject, GUIControl_SetClickable);
}

// int (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiObject, GUIControl_GetEnabled);
}

// void (GuiObject *guio, int enabled)
RuntimeScriptValue Sc_GUIControl_SetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiObject, GUIControl_SetEnabled);
}

// int (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiObject, GUIControl_GetHeight);
}

// void (GuiObject *guio, int newhit)
RuntimeScriptValue Sc_GUIControl_SetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiObject, GUIControl_SetHeight);
}

// int (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiObject, GUIControl_GetID);
}

// ScriptGUI* (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetOwningGUI(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GuiObject, ScriptGUI, ccDynamicGUI, GUIControl_GetOwningGUI);
}

// int (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiObject, GUIControl_GetVisible);
}

// void (GuiObject *guio, int visible)
RuntimeScriptValue Sc_GUIControl_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiObject, GUIControl_SetVisible);
}

// int (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiObject, GUIControl_GetWidth);
}

// void (GuiObject *guio, int newwid)
RuntimeScriptValue Sc_GUIControl_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiObject, GUIControl_SetWidth);
}

// int (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiObject, GUIControl_GetX);
}

// void (GuiObject *guio, int xx)
RuntimeScriptValue Sc_GUIControl_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiObject, GUIControl_SetX);
}

// int (GuiObject *guio)
RuntimeScriptValue Sc_GUIControl_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiObject, GUIControl_GetY);
}

// void (GuiObject *guio, int yy)
RuntimeScriptValue Sc_GUIControl_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiObject, GUIControl_SetY);
}



void RegisterGUIControlAPI()
{
    ccAddExternalObjectFunction("GUIControl::BringToFront^0",   Sc_GUIControl_BringToFront);
    ccAddExternalStaticFunction("GUIControl::GetAtScreenXY^2",  Sc_GetGUIControlAtLocation);
    ccAddExternalObjectFunction("GUIControl::SendToBack^0",     Sc_GUIControl_SendToBack);
    ccAddExternalObjectFunction("GUIControl::SetPosition^2",    Sc_GUIControl_SetPosition);
    ccAddExternalObjectFunction("GUIControl::SetSize^2",        Sc_GUIControl_SetSize);
    ccAddExternalObjectFunction("GUIControl::get_AsButton",     Sc_GUIControl_GetAsButton);
    ccAddExternalObjectFunction("GUIControl::get_AsInvWindow",  Sc_GUIControl_GetAsInvWindow);
    ccAddExternalObjectFunction("GUIControl::get_AsLabel",      Sc_GUIControl_GetAsLabel);
    ccAddExternalObjectFunction("GUIControl::get_AsListBox",    Sc_GUIControl_GetAsListBox);
    ccAddExternalObjectFunction("GUIControl::get_AsSlider",     Sc_GUIControl_GetAsSlider);
    ccAddExternalObjectFunction("GUIControl::get_AsTextBox",    Sc_GUIControl_GetAsTextBox);
    ccAddExternalObjectFunction("GUIControl::get_Clickable",    Sc_GUIControl_GetClickable);
    ccAddExternalObjectFunction("GUIControl::set_Clickable",    Sc_GUIControl_SetClickable);
    ccAddExternalObjectFunction("GUIControl::get_Enabled",      Sc_GUIControl_GetEnabled);
    ccAddExternalObjectFunction("GUIControl::set_Enabled",      Sc_GUIControl_SetEnabled);
    ccAddExternalObjectFunction("GUIControl::get_Height",       Sc_GUIControl_GetHeight);
    ccAddExternalObjectFunction("GUIControl::set_Height",       Sc_GUIControl_SetHeight);
    ccAddExternalObjectFunction("GUIControl::get_ID",           Sc_GUIControl_GetID);
    ccAddExternalObjectFunction("GUIControl::get_OwningGUI",    Sc_GUIControl_GetOwningGUI);
    ccAddExternalObjectFunction("GUIControl::get_Visible",      Sc_GUIControl_GetVisible);
    ccAddExternalObjectFunction("GUIControl::set_Visible",      Sc_GUIControl_SetVisible);
    ccAddExternalObjectFunction("GUIControl::get_Width",        Sc_GUIControl_GetWidth);
    ccAddExternalObjectFunction("GUIControl::set_Width",        Sc_GUIControl_SetWidth);
    ccAddExternalObjectFunction("GUIControl::get_X",            Sc_GUIControl_GetX);
    ccAddExternalObjectFunction("GUIControl::set_X",            Sc_GUIControl_SetX);
    ccAddExternalObjectFunction("GUIControl::get_Y",            Sc_GUIControl_GetY);
    ccAddExternalObjectFunction("GUIControl::set_Y",            Sc_GUIControl_SetY);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("GUIControl::BringToFront^0",   (void*)GUIControl_BringToFront);
    ccAddExternalFunctionForPlugin("GUIControl::GetAtScreenXY^2",  (void*)GetGUIControlAtLocation);
    ccAddExternalFunctionForPlugin("GUIControl::SendToBack^0",     (void*)GUIControl_SendToBack);
    ccAddExternalFunctionForPlugin("GUIControl::SetPosition^2",    (void*)GUIControl_SetPosition);
    ccAddExternalFunctionForPlugin("GUIControl::SetSize^2",        (void*)GUIControl_SetSize);
    ccAddExternalFunctionForPlugin("GUIControl::get_AsButton",     (void*)GUIControl_GetAsButton);
    ccAddExternalFunctionForPlugin("GUIControl::get_AsInvWindow",  (void*)GUIControl_GetAsInvWindow);
    ccAddExternalFunctionForPlugin("GUIControl::get_AsLabel",      (void*)GUIControl_GetAsLabel);
    ccAddExternalFunctionForPlugin("GUIControl::get_AsListBox",    (void*)GUIControl_GetAsListBox);
    ccAddExternalFunctionForPlugin("GUIControl::get_AsSlider",     (void*)GUIControl_GetAsSlider);
    ccAddExternalFunctionForPlugin("GUIControl::get_AsTextBox",    (void*)GUIControl_GetAsTextBox);
    ccAddExternalFunctionForPlugin("GUIControl::get_Clickable",    (void*)GUIControl_GetClickable);
    ccAddExternalFunctionForPlugin("GUIControl::set_Clickable",    (void*)GUIControl_SetClickable);
    ccAddExternalFunctionForPlugin("GUIControl::get_Enabled",      (void*)GUIControl_GetEnabled);
    ccAddExternalFunctionForPlugin("GUIControl::set_Enabled",      (void*)GUIControl_SetEnabled);
    ccAddExternalFunctionForPlugin("GUIControl::get_Height",       (void*)GUIControl_GetHeight);
    ccAddExternalFunctionForPlugin("GUIControl::set_Height",       (void*)GUIControl_SetHeight);
    ccAddExternalFunctionForPlugin("GUIControl::get_ID",           (void*)GUIControl_GetID);
    ccAddExternalFunctionForPlugin("GUIControl::get_OwningGUI",    (void*)GUIControl_GetOwningGUI);
    ccAddExternalFunctionForPlugin("GUIControl::get_Visible",      (void*)GUIControl_GetVisible);
    ccAddExternalFunctionForPlugin("GUIControl::set_Visible",      (void*)GUIControl_SetVisible);
    ccAddExternalFunctionForPlugin("GUIControl::get_Width",        (void*)GUIControl_GetWidth);
    ccAddExternalFunctionForPlugin("GUIControl::set_Width",        (void*)GUIControl_SetWidth);
    ccAddExternalFunctionForPlugin("GUIControl::get_X",            (void*)GUIControl_GetX);
    ccAddExternalFunctionForPlugin("GUIControl::set_X",            (void*)GUIControl_SetX);
    ccAddExternalFunctionForPlugin("GUIControl::get_Y",            (void*)GUIControl_GetY);
    ccAddExternalFunctionForPlugin("GUIControl::set_Y",            (void*)GUIControl_SetY);
}
