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
#include "ac/inventoryitem.h"
#include "ac/characterinfo.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/gui.h"
#include "ac/mouse.h"
#include "ac/properties.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "ac/dynobj/cc_inventory.h"
#include "gui/guidefines.h"
#include "script/runtimescriptvalue.h"
#include "script/script.h"

using namespace AGS::Common;
using namespace AGS::Engine;


extern GameSetupStruct game;
extern ScriptInvItem scrInv[MAX_INV];
extern int cur_cursor;
extern CharacterInfo*playerchar;
extern CCInventory ccDynamicInv;


void InventoryItem_SetCursorGraphic(ScriptInvItem *iitem, int newSprite) 
{
    set_inv_item_cursorpic(iitem->id, newSprite);
}

int InventoryItem_GetCursorGraphic(ScriptInvItem *iitem) 
{
    return game.invinfo[iitem->id].cursorPic;
}

void set_inv_item_pic(int invi, int piccy) {
    if ((invi < 1) || (invi > game.numinvitems))
        quit("!SetInvItemPic: invalid inventory item specified");

    if (game.invinfo[invi].pic == piccy)
        return;

    game.invinfo[invi].pic = piccy;
    GUIE::MarkInventoryForUpdate(-1, false);
}

void InventoryItem_SetGraphic(ScriptInvItem *iitem, int piccy) {
    set_inv_item_pic(iitem->id, piccy);
}

void SetInvItemName(int invi, const char *newName) {
    if ((invi < 1) || (invi > game.numinvitems))
        quit("!SetInvName: invalid inventory item specified");

    game.invinfo[invi].name = newName;
    // might need to redraw the GUI if it has the inv item name on it
    GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
}

void InventoryItem_SetName(ScriptInvItem *scii, const char *newname) {
    SetInvItemName(scii->id, newname);
}

int InventoryItem_GetID(ScriptInvItem *scii) {
    return scii->id;
}

const char *InventoryItem_GetScriptName(ScriptInvItem *scii)
{
    return CreateNewScriptString(game.invScriptNames[scii->id]);
}

int offset_over_inv(GUIInvWindow *inv) {
    if (inv->GetItemWidth() <= 0 || inv->GetItemHeight() <= 0)
        return -1;
    int mover = mouse_ifacebut_xoffs / inv->GetItemWidth();
    // if it's off the edge of the visible items, ignore
    if (mover >= inv->GetColCount())
        return -1;
    mover += (mouse_ifacebut_yoffs / inv->GetItemHeight()) * inv->GetColCount();
    if (mover >= inv->GetColCount() * inv->GetRowCount())
        return -1;

    mover += inv->GetTopItem();
    if ((mover < 0) || (mover >= charextra[inv->GetCharacterID()].invorder_count))
        return -1;

    return charextra[inv->GetCharacterID()].invorder[mover];
}

int GetInvAt(int scrx, int scry) {
    int ongui = GetGUIAt(scrx, scry);
    if (ongui >= 0) {
        GUIMain &gui = guis[ongui];
        int onobj = gui.FindControlAt(scrx, scry);
        GUIControl *guio = gui.GetControl(onobj);
        if (guio) {
            Point guipt = gui.GetGraphicSpace().WorldToLocal(scrx, scry);
            mouse_ifacebut_xoffs = guipt.X - guio->GetX();
            mouse_ifacebut_yoffs = guipt.Y - guio->GetY();
        }
        if (guio && (gui.GetControlType(onobj) == kGUIInvWindow))
            return offset_over_inv((GUIInvWindow *)guio);
    }
    return -1;
}

ScriptInvItem *InventoryItem_GetAtScreenXY(int xx, int yy) {
  int hsnum = GetInvAt(xx, yy);
  if (hsnum <= 0)
    return nullptr;
  return &scrInv[hsnum];
}

const char* InventoryItem_GetName_New(ScriptInvItem *invitem) {
  return CreateNewScriptString(game.invinfo[invitem->id].name);
}

int InventoryItem_GetGraphic(ScriptInvItem *iitem) {
  return game.invinfo[iitem->id].pic;
}

void RunInventoryInteraction(int iit, int mood) {
    if ((iit < 0) || (iit >= game.numinvitems))
        quit("!RunInventoryInteraction: invalid inventory number");

    // convert cursor mode to event index (in inventoryitem event table)
    // TODO: probably move this conversion table elsewhere? should be a global info
    int evnt;
    switch (mood)
    {
    case MODE_LOOK: evnt = 0; break;
    case MODE_HAND: evnt = 1; break;
    case MODE_TALK: evnt = 2; break;
    case MODE_USE: evnt = 3; break;
    default: evnt = -1; break;
    }
    const int otherclick_evt = 4; // TODO: make global constant (inventory other-click evt)

    // For USE verb: remember active inventory
    if (mood == MODE_USE)
    {
        play.usedinv = playerchar->activeinv;
    }

    if (evnt < 0) // on any non-supported mode - use "other-click"
        evnt = otherclick_evt;

    const auto obj_evt = ObjectEvent(kScTypeGame, "inventory%d", iit,
                                     RuntimeScriptValue().SetScriptObject(&scrInv[iit], &ccDynamicInv), mood);
    run_interaction_script(obj_evt, game.invScripts[iit].get(), evnt);
}

int IsInventoryInteractionAvailable(int item, int mood) {
    if ((item < 0) || (item >= MAX_INV))
        quit("!IsInventoryInteractionAvailable: invalid inventory number");

    play.check_interaction_only = 1;

    RunInventoryInteraction(item, mood);

    int ciwas = play.check_interaction_only;
    play.check_interaction_only = 0;

    if (ciwas == 2)
        return 1;

    return 0;
}

void InventoryItem_RunInteraction(ScriptInvItem *iitem, int mood) {
    RunInventoryInteraction(iitem->id, mood);
}

int InventoryItem_CheckInteractionAvailable(ScriptInvItem *iitem, int mood) {
  return IsInventoryInteractionAvailable(iitem->id, mood);
}

int InventoryItem_GetProperty(ScriptInvItem *scii, const char *property) {
    return get_int_property (game.invProps[scii->id], play.invProps[scii->id], property);
}

void InventoryItem_GetPropertyText(ScriptInvItem *scii, const char *property, char *bufer) {
    get_text_property(game.invProps[scii->id], play.invProps[scii->id], property, bufer);
}

const char* InventoryItem_GetTextProperty(ScriptInvItem *scii, const char *property) {
    return get_text_property_dynamic_string(game.invProps[scii->id], play.invProps[scii->id], property);
}

bool InventoryItem_SetProperty(ScriptInvItem *scii, const char *property, int value)
{
    return set_int_property(play.invProps[scii->id], property, value);
}

bool InventoryItem_SetTextProperty(ScriptInvItem *scii, const char *property, const char *value)
{
    return set_text_property(play.invProps[scii->id], property, value);
}

//=============================================================================

void set_inv_item_cursorpic(int invItemId, int piccy) 
{
    game.invinfo[invItemId].cursorPic = piccy;

    if ((cur_cursor == MODE_USE) && (playerchar->activeinv == invItemId)) 
    {
        update_inv_cursor(invItemId);
        set_mouse_cursor(cur_cursor);
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


ScriptInvItem *InventoryItem_GetByName(const char *name)
{
    return static_cast<ScriptInvItem*>(ccGetScriptObjectAddress(name, ccDynamicInv.GetType()));
}


RuntimeScriptValue Sc_InventoryItem_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptInvItem, ccDynamicInv, InventoryItem_GetByName, const char);
}

// ScriptInvItem *(int xx, int yy)
RuntimeScriptValue Sc_InventoryItem_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptInvItem, ccDynamicInv, InventoryItem_GetAtScreenXY);
}

// int (ScriptInvItem *iitem, int mood)
RuntimeScriptValue Sc_InventoryItem_CheckInteractionAvailable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptInvItem, InventoryItem_CheckInteractionAvailable);
}

// int (ScriptInvItem *scii, const char *property)
RuntimeScriptValue Sc_InventoryItem_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptInvItem, InventoryItem_GetProperty, const char);
}

// void (ScriptInvItem *scii, const char *property, char *bufer)
RuntimeScriptValue Sc_InventoryItem_GetPropertyText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ2(ScriptInvItem, InventoryItem_GetPropertyText, const char, char);
}

// const char* (ScriptInvItem *scii, const char *property)
RuntimeScriptValue Sc_InventoryItem_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptInvItem, const char, myScriptStringImpl, InventoryItem_GetTextProperty, const char);
}

RuntimeScriptValue Sc_InventoryItem_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptInvItem, InventoryItem_SetProperty, const char);
}

RuntimeScriptValue Sc_InventoryItem_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptInvItem, InventoryItem_SetTextProperty, const char, const char);
}

// void (ScriptInvItem *iitem, int mood)
RuntimeScriptValue Sc_InventoryItem_RunInteraction(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptInvItem, InventoryItem_RunInteraction);
}

// void (ScriptInvItem *scii, const char *newname)
RuntimeScriptValue Sc_InventoryItem_SetName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptInvItem, InventoryItem_SetName, const char);
}

// int (ScriptInvItem *iitem) 
RuntimeScriptValue Sc_InventoryItem_GetCursorGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptInvItem, InventoryItem_GetCursorGraphic);
}

// void (ScriptInvItem *iitem, int newSprite) 
RuntimeScriptValue Sc_InventoryItem_SetCursorGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptInvItem, InventoryItem_SetCursorGraphic);
}

// int (ScriptInvItem *iitem)
RuntimeScriptValue Sc_InventoryItem_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptInvItem, InventoryItem_GetGraphic);
}

// void (ScriptInvItem *iitem, int piccy)
RuntimeScriptValue Sc_InventoryItem_SetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptInvItem, InventoryItem_SetGraphic);
}

// int (ScriptInvItem *scii)
RuntimeScriptValue Sc_InventoryItem_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptInvItem, InventoryItem_GetID);
}

RuntimeScriptValue Sc_InventoryItem_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptInvItem, const char, myScriptStringImpl, InventoryItem_GetScriptName);
}

// const char* (ScriptInvItem *invitem)
RuntimeScriptValue Sc_InventoryItem_GetName_New(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptInvItem, const char, myScriptStringImpl, InventoryItem_GetName_New);
}



void RegisterInventoryItemAPI()
{
    ScFnRegister invitem_api[] = {
        { "InventoryItem::GetAtScreenXY^2",           API_FN_PAIR(InventoryItem_GetAtScreenXY) },
        { "InventoryItem::GetByName",                 API_FN_PAIR(InventoryItem_GetByName) },

        { "InventoryItem::IsInteractionAvailable^1",  API_FN_PAIR(InventoryItem_CheckInteractionAvailable) },
        { "InventoryItem::GetProperty^1",             API_FN_PAIR(InventoryItem_GetProperty) },
        { "InventoryItem::GetPropertyText^2",         API_FN_PAIR(InventoryItem_GetPropertyText) },
        { "InventoryItem::GetTextProperty^1",         API_FN_PAIR(InventoryItem_GetTextProperty) },
        { "InventoryItem::SetProperty^2",             API_FN_PAIR(InventoryItem_SetProperty) },
        { "InventoryItem::SetTextProperty^2",         API_FN_PAIR(InventoryItem_SetTextProperty) },
        { "InventoryItem::RunInteraction^1",          API_FN_PAIR(InventoryItem_RunInteraction) },
        { "InventoryItem::SetName^1",                 API_FN_PAIR(InventoryItem_SetName) },
        { "InventoryItem::get_CursorGraphic",         API_FN_PAIR(InventoryItem_GetCursorGraphic) },
        { "InventoryItem::set_CursorGraphic",         API_FN_PAIR(InventoryItem_SetCursorGraphic) },
        { "InventoryItem::get_Graphic",               API_FN_PAIR(InventoryItem_GetGraphic) },
        { "InventoryItem::set_Graphic",               API_FN_PAIR(InventoryItem_SetGraphic) },
        { "InventoryItem::get_ID",                    API_FN_PAIR(InventoryItem_GetID) },
        { "InventoryItem::get_Name",                  API_FN_PAIR(InventoryItem_GetName_New) },
        { "InventoryItem::set_Name",                  API_FN_PAIR(InventoryItem_SetName) },
        { "InventoryItem::get_ScriptName",            API_FN_PAIR(InventoryItem_GetScriptName) },
    };

    ccAddExternalFunctions(invitem_api);
}
