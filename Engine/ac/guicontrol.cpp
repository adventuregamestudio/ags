//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <vector>
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_gui.h"
#include "ac/gui.h"
#include "ac/guicontrol.h"
#include "ac/mouse.h"
#include "ac/object.h"
#include "ac/properties.h"
#include "ac/string.h"
#include "ac/dynobj/cc_gui.h"
#include "ac/dynobj/cc_guicontrol.h"
#include "debug/debug_log.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern std::vector<ScriptGUI> scrGui;

extern CCGUIControl ccDynamicGUIControl;
extern CCGUIButton ccDynamicGUIButton;
extern CCGUIInvWindow ccDynamicGUIInvWindow;
extern CCGUILabel ccDynamicGUILabel;
extern CCGUIListBox ccDynamicGUIListBox;
extern CCGUISlider ccDynamicGUISlider;
extern CCGUITextBox ccDynamicGUITextBox;

GUIObject *GetGUIControlAtLocation(int xx, int yy) {
    int guinum = GetGUIAt(xx, yy);
    if (guinum == -1)
        return nullptr;

    int toret = guis[guinum].FindControlAt(xx, yy, 0, false);
    if (toret < 0)
        return nullptr;

    return guis[guinum].GetControl(toret);
}

int GUIControl_GetVisible(GUIObject *guio) {
  return guio->IsVisible();
}

void GUIControl_SetVisible(GUIObject *guio, int visible) 
{
  const bool on = visible != 0;
  if (on != guio->IsVisible())
  {
    guio->SetVisible(on);
  }
}

int GUIControl_GetClickable(GUIObject *guio) {
  if (guio->IsClickable())
    return 1;
  return 0;
}

void GUIControl_SetClickable(GUIObject *guio, int enabled) {
  const bool on = enabled != 0;
  if (on != guio->IsClickable())
  {
    guio->SetClickable(on);
  }
}

int GUIControl_GetEnabled(GUIObject *guio) {
  return guio->IsEnabled() ? 1 : 0;
}

void GUIControl_SetEnabled(GUIObject *guio, int enabled) {
  const bool on = enabled != 0;
  if (on != guio->IsEnabled())
  {
    guio->SetEnabled(on);
  }
}


int GUIControl_GetID(GUIObject *guio) {
  return guio->GetID();
}

const char *GUIControl_GetScriptName(GUIObject *guio)
{
    return CreateNewScriptString(guio->GetName());
}

ScriptGUI* GUIControl_GetOwningGUI(GUIObject *guio) {
  return &scrGui[guio->GetParentID()];
}

GUIButton* GUIControl_GetAsButton(GUIObject *guio) {
  if (guis[guio->GetParentID()].GetControlType(guio->GetID()) != kGUIButton)
    return nullptr;

  return (GUIButton*)guio;
}

GUIInvWindow* GUIControl_GetAsInvWindow(GUIObject *guio) {
  if (guis[guio->GetParentID()].GetControlType(guio->GetID()) != kGUIInvWindow)
    return nullptr;

  return (GUIInvWindow*)guio;
}

GUILabel* GUIControl_GetAsLabel(GUIObject *guio) {
  if (guis[guio->GetParentID()].GetControlType(guio->GetID()) != kGUILabel)
    return nullptr;

  return (GUILabel*)guio;
}

GUIListBox* GUIControl_GetAsListBox(GUIObject *guio) {
  if (guis[guio->GetParentID()].GetControlType(guio->GetID()) != kGUIListBox)
    return nullptr;

  return (GUIListBox*)guio;
}

GUISlider* GUIControl_GetAsSlider(GUIObject *guio) {
  if (guis[guio->GetParentID()].GetControlType(guio->GetID()) != kGUISlider)
    return nullptr;

  return (GUISlider*)guio;
}

GUITextBox* GUIControl_GetAsTextBox(GUIObject *guio) {
  if (guis[guio->GetParentID()].GetControlType(guio->GetID()) != kGUITextBox)
    return nullptr;

  return (GUITextBox*)guio;
}

int GUIControl_GetX(GUIObject *guio) {
  return guio->GetX();
}

void GUIControl_SetX(GUIObject *guio, int xx) {
  guio->SetX(xx);
}

int GUIControl_GetY(GUIObject *guio) {
  return guio->GetY();
}

void GUIControl_SetY(GUIObject *guio, int yy) {
  guio->SetY(yy);
}

int GUIControl_GetZOrder(GUIObject *guio)
{
    return guio->GetZOrder();
}

void GUIControl_SetZOrder(GUIObject *guio, int zorder)
{
    guis[guio->GetParentID()].SetControlZOrder(guio->GetID(), zorder);
}

void GUIControl_SetPosition(GUIObject *guio, int xx, int yy) {
  GUIControl_SetX(guio, xx);
  GUIControl_SetY(guio, yy);
}


int GUIControl_GetWidth(GUIObject *guio) {
  return guio->GetWidth();
}

void GUIControl_SetWidth(GUIObject *guio, int newwid) {
  guio->SetWidth(newwid);
}

int GUIControl_GetHeight(GUIObject *guio) {
  return guio->GetHeight();
}

void GUIControl_SetHeight(GUIObject *guio, int newhit) {
  guio->SetHeight(newhit);
}

void GUIControl_SetSize(GUIObject *guio, int newwid, int newhit) {
  if ((newwid < 2) || (newhit < 2))
    quit("!SetGUIObjectSize: new size is too small (must be at least 2x2)");

  debug_script_log("SetGUIObject %d,%d size %d,%d", guio->GetParentID(), guio->GetID(), newwid, newhit);
  GUIControl_SetWidth(guio, newwid);
  GUIControl_SetHeight(guio, newhit);
}

void GUIControl_SendToBack(GUIObject *guio) {
  guis[guio->GetParentID()].SendControlToBack(guio->GetID());
}

void GUIControl_BringToFront(GUIObject *guio) {
  guis[guio->GetParentID()].BringControlToFront(guio->GetID());
}

int GUIControl_GetTransparency(GUIObject *guio) {
    return GfxDef::LegacyTrans255ToTrans100(guio->GetTransparency());
}

void GUIControl_SetTransparency(GUIObject *guio, int trans) {
    if ((trans < 0) | (trans > 100))
        quit("!SetGUITransparency: transparency value must be between 0 and 100");
    guio->SetTransparency(GfxDef::Trans100ToLegacyTrans255(trans));
}

int GUIControl_GetBlendMode(GUIObject *guio) {
    return guio->GetBlendMode();
}

void GUIControl_SetBlendMode(GUIObject *guio, int blend_mode) {
    guio->SetBlendMode(ValidateBlendMode("GUIControl.BlendMode", blend_mode));
}

int GUIControl_GetProperty(GUIObject *guio, const char *property)
{
    int ctrl_type = guis[guio->GetParentID()].GetControlType(guio->GetID());
    int ctrl_id = guis[guio->GetParentID()].GetControlID(guio->GetID());
    return get_int_property(game.guicontrolProps[ctrl_type][ctrl_id], play.guicontrolProps[ctrl_type][ctrl_id], property);
}

const char* GUIControl_GetTextProperty(GUIObject *guio, const char *property)
{
    int ctrl_type = guis[guio->GetParentID()].GetControlType(guio->GetID());
    int ctrl_id = guis[guio->GetParentID()].GetControlID(guio->GetID());
    return get_text_property_dynamic_string(game.guicontrolProps[ctrl_type][ctrl_id], play.guicontrolProps[ctrl_type][ctrl_id], property);
}

bool GUIControl_SetProperty(GUIObject *guio, const char *property, int value)
{
    int ctrl_type = guis[guio->GetParentID()].GetControlType(guio->GetID());
    int ctrl_id = guis[guio->GetParentID()].GetControlID(guio->GetID());
    return set_int_property(play.guicontrolProps[ctrl_type][ctrl_id], property, value);
}

bool GUIControl_SetTextProperty(GUIObject *guio, const char *property, const char *value)
{
    int ctrl_type = guis[guio->GetParentID()].GetControlType(guio->GetID());
    int ctrl_id = guis[guio->GetParentID()].GetControlID(guio->GetID());
    return set_text_property(play.guicontrolProps[ctrl_type][ctrl_id], property, value);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "ac/dynobj/scriptstring.h"
#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"


GUIObject *GUIControl_GetByName(const char *name)
{
    // TODO: figure out if this may be simplified
    const static std::vector<String> typenames = {
        ccDynamicGUIButton.GetType(), ccDynamicGUIInvWindow.GetType(), ccDynamicGUILabel.GetType(),
        ccDynamicGUIListBox.GetType(), ccDynamicGUISlider.GetType(), ccDynamicGUITextBox.GetType()
    };
    return static_cast<GUIObject*>(ccGetScriptObjectAddress(name, typenames));
}


RuntimeScriptValue Sc_GUIControl_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(GUIObject, ccDynamicGUIControl, GUIControl_GetByName, const char);
}

// void (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_BringToFront(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIObject, GUIControl_BringToFront);
}

// GUIObject *(int xx, int yy)
RuntimeScriptValue Sc_GetGUIControlAtLocation(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(GUIObject, ccDynamicGUIControl, GetGUIControlAtLocation);
}

// void (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_SendToBack(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIObject, GUIControl_SendToBack);
}

// void (GUIObject *guio, int xx, int yy)
RuntimeScriptValue Sc_GUIControl_SetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(GUIObject, GUIControl_SetPosition);
}

// void (GUIObject *guio, int newwid, int newhit)
RuntimeScriptValue Sc_GUIControl_SetSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(GUIObject, GUIControl_SetSize);
}

// GUIButton* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsButton(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, GUIButton, ccDynamicGUIControl, GUIControl_GetAsButton);
}

// GUIInvWindow* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsInvWindow(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, GUIInvWindow, ccDynamicGUIControl, GUIControl_GetAsInvWindow);
}

// GUILabel* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsLabel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, GUILabel, ccDynamicGUIControl, GUIControl_GetAsLabel);
}

// GUIListBox* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsListBox(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, GUIListBox, ccDynamicGUIControl, GUIControl_GetAsListBox);
}

// GUISlider* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsSlider(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, GUISlider, ccDynamicGUIControl, GUIControl_GetAsSlider);
}

// GUITextBox* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetAsTextBox(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, GUITextBox, ccDynamicGUIControl, GUIControl_GetAsTextBox);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetClickable);
}

// void (GUIObject *guio, int enabled)
RuntimeScriptValue Sc_GUIControl_SetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetClickable);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetEnabled);
}

// void (GUIObject *guio, int enabled)
RuntimeScriptValue Sc_GUIControl_SetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetEnabled);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetHeight);
}

// void (GUIObject *guio, int newhit)
RuntimeScriptValue Sc_GUIControl_SetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetHeight);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetID);
}

RuntimeScriptValue Sc_GUIControl_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, const char, myScriptStringImpl, GUIControl_GetScriptName);
}

// ScriptGUI* (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetOwningGUI(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIObject, ScriptGUI, ccDynamicGUIControl, GUIControl_GetOwningGUI);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetVisible);
}

// void (GUIObject *guio, int visible)
RuntimeScriptValue Sc_GUIControl_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetVisible);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetWidth);
}

// void (GUIObject *guio, int newwid)
RuntimeScriptValue Sc_GUIControl_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetWidth);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetX);
}

// void (GUIObject *guio, int xx)
RuntimeScriptValue Sc_GUIControl_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetX);
}

// int (GUIObject *guio)
RuntimeScriptValue Sc_GUIControl_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetY);
}

// void (GUIObject *guio, int yy)
RuntimeScriptValue Sc_GUIControl_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetY);
}

RuntimeScriptValue Sc_GUIControl_GetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetZOrder);
}

RuntimeScriptValue Sc_GUIControl_SetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetZOrder);
}

RuntimeScriptValue Sc_GUIControl_GetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetTransparency);
}

RuntimeScriptValue Sc_GUIControl_SetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetTransparency);
}

RuntimeScriptValue Sc_GUIControl_GetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIObject, GUIControl_GetBlendMode);
}

RuntimeScriptValue Sc_GUIControl_SetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIObject, GUIControl_SetBlendMode);
}

RuntimeScriptValue Sc_GUIControl_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(GUIObject, GUIControl_GetProperty, const char);
}

RuntimeScriptValue Sc_GUIControl_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(GUIObject, const char, myScriptStringImpl, GUIControl_GetTextProperty, const char);
}

RuntimeScriptValue Sc_GUIControl_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(GUIObject, GUIControl_SetProperty, const char);
}

RuntimeScriptValue Sc_GUIControl_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(GUIObject, GUIControl_SetTextProperty, const char, const char);
}


void RegisterGUIControlAPI()
{
    ScFnRegister guicontrol_api[] = {
        { "GUIControl::GetAtScreenXY^2",  API_FN_PAIR(GetGUIControlAtLocation) },
        { "GUIControl::GetByName",        API_FN_PAIR(GUIControl_GetByName) },

        { "GUIControl::BringToFront^0",   API_FN_PAIR(GUIControl_BringToFront) },
        { "GUIControl::SendToBack^0",     API_FN_PAIR(GUIControl_SendToBack) },
        { "GUIControl::SetPosition^2",    API_FN_PAIR(GUIControl_SetPosition) },
        { "GUIControl::SetSize^2",        API_FN_PAIR(GUIControl_SetSize) },
        { "GUIControl::GetProperty^1",    API_FN_PAIR(GUIControl_GetProperty) },
        { "GUIControl::GetTextProperty^1", API_FN_PAIR(GUIControl_GetTextProperty) },
        { "GUIControl::SetProperty^2",    API_FN_PAIR(GUIControl_SetProperty) },
        { "GUIControl::SetTextProperty^2", API_FN_PAIR(GUIControl_SetTextProperty) },
        { "GUIControl::get_AsButton",     API_FN_PAIR(GUIControl_GetAsButton) },
        { "GUIControl::get_AsInvWindow",  API_FN_PAIR(GUIControl_GetAsInvWindow) },
        { "GUIControl::get_AsLabel",      API_FN_PAIR(GUIControl_GetAsLabel) },
        { "GUIControl::get_AsListBox",    API_FN_PAIR(GUIControl_GetAsListBox) },
        { "GUIControl::get_AsSlider",     API_FN_PAIR(GUIControl_GetAsSlider) },
        { "GUIControl::get_AsTextBox",    API_FN_PAIR(GUIControl_GetAsTextBox) },
        { "GUIControl::get_Clickable",    API_FN_PAIR(GUIControl_GetClickable) },
        { "GUIControl::set_Clickable",    API_FN_PAIR(GUIControl_SetClickable) },
        { "GUIControl::get_Enabled",      API_FN_PAIR(GUIControl_GetEnabled) },
        { "GUIControl::set_Enabled",      API_FN_PAIR(GUIControl_SetEnabled) },
        { "GUIControl::get_Height",       API_FN_PAIR(GUIControl_GetHeight) },
        { "GUIControl::set_Height",       API_FN_PAIR(GUIControl_SetHeight) },
        { "GUIControl::get_ID",           API_FN_PAIR(GUIControl_GetID) },
        { "GUIControl::get_OwningGUI",    API_FN_PAIR(GUIControl_GetOwningGUI) },
        { "GUIControl::get_ScriptName",   API_FN_PAIR(GUIControl_GetScriptName) },
        { "GUIControl::get_Visible",      API_FN_PAIR(GUIControl_GetVisible) },
        { "GUIControl::set_Visible",      API_FN_PAIR(GUIControl_SetVisible) },
        { "GUIControl::get_Width",        API_FN_PAIR(GUIControl_GetWidth) },
        { "GUIControl::set_Width",        API_FN_PAIR(GUIControl_SetWidth) },
        { "GUIControl::get_X",            API_FN_PAIR(GUIControl_GetX) },
        { "GUIControl::set_X",            API_FN_PAIR(GUIControl_SetX) },
        { "GUIControl::get_Y",            API_FN_PAIR(GUIControl_GetY) },
        { "GUIControl::set_Y",            API_FN_PAIR(GUIControl_SetY) },
        { "GUIControl::get_ZOrder",       API_FN_PAIR(GUIControl_GetZOrder) },
        { "GUIControl::set_ZOrder",       API_FN_PAIR(GUIControl_SetZOrder) },
        { "GUIControl::get_Transparency", API_FN_PAIR(GUIControl_GetTransparency) },
        { "GUIControl::set_Transparency", API_FN_PAIR(GUIControl_SetTransparency) },
        { "GUIControl::get_BlendMode",    API_FN_PAIR(GUIControl_GetBlendMode) },
        { "GUIControl::set_BlendMode",    API_FN_PAIR(GUIControl_SetBlendMode) },
    };

    ccAddExternalFunctions(guicontrol_api);
}
