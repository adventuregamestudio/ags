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
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/guicontrol.h"
#include "ac/global_gui.h"
#include "debug/debug_log.h"
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

extern GUIMain*guis;
extern ScriptGUI*scrGui;
extern RuntimeScriptValue GlobalReturnValue;
extern CCGUI ccDynamicGUI;
extern CCGUIObject ccDynamicGUIObject;

GUIObject *GetGUIControlAtLocation(int xx, int yy) {
    int guinum = GetGUIAt(xx, yy);
    if (guinum == -1)
        return NULL;

    multiply_up_coordinates(&xx, &yy);

    int oldmousex = mousex, oldmousey = mousey;
    mousex = xx - guis[guinum].x;
    mousey = yy - guis[guinum].y;
    int toret = guis[guinum].find_object_under_mouse(0, false);
    mousex = oldmousex;
    mousey = oldmousey;
    if (toret < 0)
        return NULL;

    GlobalReturnValue.SetDynamicObject(guis[guinum].objs[toret], &ccDynamicGUIObject);
    return guis[guinum].objs[toret];
}

int GUIControl_GetVisible(GUIObject *guio) {
  return guio->IsVisible();
}

void GUIControl_SetVisible(GUIObject *guio, int visible) 
{
  if (visible != guio->IsVisible()) 
  {
    if (visible)
      guio->Show();
    else
      guio->Hide();

    guis[guio->guin].control_positions_changed();
    guis_need_update = 1;
  }
}

int GUIControl_GetClickable(GUIObject *guio) {
  if (guio->IsClickable())
    return 1;
  return 0;
}

void GUIControl_SetClickable(GUIObject *guio, int enabled) {
  if (enabled)
    guio->SetClickable(true);
  else
    guio->SetClickable(false);

  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

int GUIControl_GetEnabled(GUIObject *guio) {
  if (guio->IsDisabled())
    return 0;
  return 1;
}

void GUIControl_SetEnabled(GUIObject *guio, int enabled) {
  if (enabled)
    guio->Enable();
  else
    guio->Disable();

  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}


int GUIControl_GetID(GUIObject *guio) {
  return guio->objn;
}

ScriptGUI* GUIControl_GetOwningGUI(GUIObject *guio) {
  GlobalReturnValue.SetDynamicObject(&scrGui[guio->guin], &ccDynamicGUI);
  return &scrGui[guio->guin];
}

GUIButton* GUIControl_GetAsButton(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_BUTTON)
    return NULL;

  GlobalReturnValue.SetDynamicObject(guio, &ccDynamicGUIObject);
  return (GUIButton*)guio;
}

GUIInv* GUIControl_GetAsInvWindow(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_INVENTORY)
    return NULL;

  GlobalReturnValue.SetDynamicObject(guio, &ccDynamicGUIObject);
  return (GUIInv*)guio;
}

GUILabel* GUIControl_GetAsLabel(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_LABEL)
    return NULL;

  GlobalReturnValue.SetDynamicObject(guio, &ccDynamicGUIObject);
  return (GUILabel*)guio;
}

GUIListBox* GUIControl_GetAsListBox(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_LISTBOX)
    return NULL;

  GlobalReturnValue.SetDynamicObject(guio, &ccDynamicGUIObject);
  return (GUIListBox*)guio;
}

GUISlider* GUIControl_GetAsSlider(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_SLIDER)
    return NULL;

  GlobalReturnValue.SetDynamicObject(guio, &ccDynamicGUIObject);
  return (GUISlider*)guio;
}

GUITextBox* GUIControl_GetAsTextBox(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_TEXTBOX)
    return NULL;

  GlobalReturnValue.SetDynamicObject(guio, &ccDynamicGUIObject);
  return (GUITextBox*)guio;
}

int GUIControl_GetX(GUIObject *guio) {
  return divide_down_coordinate(guio->x);
}

void GUIControl_SetX(GUIObject *guio, int xx) {
  guio->x = multiply_up_coordinate(xx);
  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

int GUIControl_GetY(GUIObject *guio) {
  return divide_down_coordinate(guio->y);
}

void GUIControl_SetY(GUIObject *guio, int yy) {
  guio->y = multiply_up_coordinate(yy);
  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

void GUIControl_SetPosition(GUIObject *guio, int xx, int yy) {
  GUIControl_SetX(guio, xx);
  GUIControl_SetY(guio, yy);
}


int GUIControl_GetWidth(GUIObject *guio) {
  return divide_down_coordinate(guio->wid);
}

void GUIControl_SetWidth(GUIObject *guio, int newwid) {
  guio->wid = multiply_up_coordinate(newwid);
  guio->Resized();
  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

int GUIControl_GetHeight(GUIObject *guio) {
  return divide_down_coordinate(guio->hit);
}

void GUIControl_SetHeight(GUIObject *guio, int newhit) {
  guio->hit = multiply_up_coordinate(newhit);
  guio->Resized();
  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

void GUIControl_SetSize(GUIObject *guio, int newwid, int newhit) {
  if ((newwid < 2) || (newhit < 2))
    quit("!SetGUIObjectSize: new size is too small (must be at least 2x2)");

  DEBUG_CONSOLE("SetGUIObject %d,%d size %d,%d", guio->guin, guio->objn, newwid, newhit);
  GUIControl_SetWidth(guio, newwid);
  GUIControl_SetHeight(guio, newhit);
}

void GUIControl_SendToBack(GUIObject *guio) {
  if (guis[guio->guin].send_to_back(guio->objn))
    guis_need_update = 1;
}

void GUIControl_BringToFront(GUIObject *guio) {
  if (guis[guio->guin].bring_to_front(guio->objn))
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

// void (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_BringToFront(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIObject, GUIControl_BringToFront);
}

// GUIObject *(int xx, int yy)
RuntimeScriptValue Sc_GetGUIControlAtLocation(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(GUIObject, ccDynamicGUIObject, GetGUIControlAtLocation);
}

// void (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_SendToBack(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIObject, GUIControl_SendToBack);
}

// void (GUIObject *guio, int xx, int yy)
RuntimeScriptValue Sc_GUIControl_SetPosition(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(GUIObject, GUIControl_SetPosition);
}

// void (GUIObject *guio, int newwid, int newhit)
RuntimeScriptValue Sc_GUIControl_SetSize(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(GUIObject, GUIControl_SetSize);
}

// GUIButton* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsButton(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, GUIButton, ccDynamicGUI, GUIControl_GetAsButton);
}

// GUIInv* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsInvWindow(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, GUIInv, ccDynamicGUI, GUIControl_GetAsInvWindow);
}

// GUILabel* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsLabel(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, GUILabel, ccDynamicGUI, GUIControl_GetAsLabel);
}

// GUIListBox* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsListBox(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, GUIListBox, ccDynamicGUI, GUIControl_GetAsListBox);
}

// GUISlider* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsSlider(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, GUISlider, ccDynamicGUI, GUIControl_GetAsSlider);
}

// GUITextBox* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsTextBox(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, GUITextBox, ccDynamicGUI, GUIControl_GetAsTextBox);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetClickable(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetClickable);
}

// void (GUIObject *guio, int enabled)
RuntimeScriptValue Sc_GUIControl_SetClickable(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetClickable);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetEnabled(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetEnabled);
}

// void (GUIObject *guio, int enabled)
RuntimeScriptValue Sc_GUIControl_SetEnabled(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetEnabled);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetHeight(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetHeight);
}

// void (GUIObject *guio, int newhit)
RuntimeScriptValue Sc_GUIControl_SetHeight(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetHeight);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetID(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetID);
}

// ScriptGUI* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetOwningGUI(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, ScriptGUI, ccDynamicGUI, GUIControl_GetOwningGUI);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetVisible(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetVisible);
}

// void (GUIObject *guio, int visible)
RuntimeScriptValue Sc_GUIControl_SetVisible(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetVisible);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetWidth(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetWidth);
}

// void (GUIObject *guio, int newwid)
RuntimeScriptValue Sc_GUIControl_SetWidth(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetWidth);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetX(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetX);
}

// void (GUIObject *guio, int xx)
RuntimeScriptValue Sc_GUIControl_SetX(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetX);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetY(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetY);
}

// void (GUIObject *guio, int yy)
RuntimeScriptValue Sc_GUIControl_SetY(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetY);
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
}
