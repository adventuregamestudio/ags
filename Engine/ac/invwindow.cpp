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
#include "ac/invwindow.h"
#include "ac/common.h"
#include "ac/characterextras.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_character.h"
#include "ac/global_display.h"
#include "ac/global_room.h"
#include "ac/mouse.h"
#include "ac/sys_events.h"
#include "debug/debug_log.h"
#include "gui/guidialog.h"
#include "gui/guimain.h"
#include "main/game_run.h"
#include "platform/base/agsplatformdriver.h"
#include "ac/spritecache.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_character.h"
#include "ac/dynobj/cc_inventory.h"
#include "media/audio/audio_system.h"
#include "ac/timer.h"
#include "util/wgt2allg.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern GameState play;
extern ScriptInvItem scrInv[MAX_INV];
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;
extern SpriteCache spriteset;
extern int evblocknum;
extern CharacterInfo*playerchar;
extern AGSPlatformDriver *platform;
extern CCCharacter ccDynamicCharacter;
extern CCInventory ccDynamicInv;

// *** INV WINDOW FUNCTIONS

void InvWindow_SetCharacterToUse(GUIInvWindow *guii, CharacterInfo *chaa) {
  if (chaa == nullptr)
    guii->CharId = -1;
  else
    guii->CharId = chaa->index_id;
  // reset to top of list
  guii->TopItem = 0;

  guii->MarkChanged();
}

CharacterInfo* InvWindow_GetCharacterToUse(GUIInvWindow *guii) {
  if (guii->CharId < 0)
    return nullptr;

  return &game.chars[guii->CharId];
}

void InvWindow_SetItemWidth(GUIInvWindow *guii, int newwidth) {
  guii->ItemWidth = newwidth;
  guii->OnResized();
}

int InvWindow_GetItemWidth(GUIInvWindow *guii) {
  return guii->ItemWidth;
}

void InvWindow_SetItemHeight(GUIInvWindow *guii, int newhit) {
  guii->ItemHeight = newhit;
  guii->OnResized();
}

int InvWindow_GetItemHeight(GUIInvWindow *guii) {
  return guii->ItemHeight;
}

void InvWindow_SetTopItem(GUIInvWindow *guii, int topitem) {
  if (guii->TopItem != topitem) {
    guii->TopItem = topitem;
    guii->MarkChanged();
  }
}

int InvWindow_GetTopItem(GUIInvWindow *guii) {
  return guii->TopItem;
}

int InvWindow_GetItemsPerRow(GUIInvWindow *guii) {
  return guii->ColCount;
}

int InvWindow_GetItemCount(GUIInvWindow *guii) {
  return charextra[guii->GetCharacterId()].invorder_count;
}

int InvWindow_GetRowCount(GUIInvWindow *guii) {
  return guii->RowCount;
}

void InvWindow_ScrollDown(GUIInvWindow *guii) {
  if ((charextra[guii->GetCharacterId()].invorder_count) >
      (guii->TopItem + (guii->ColCount * guii->RowCount))) { 
    guii->TopItem += guii->ColCount;
    guii->MarkChanged();
  }
}

void InvWindow_ScrollUp(GUIInvWindow *guii) {
  if (guii->TopItem > 0) {
    guii->TopItem -= guii->ColCount;
    if (guii->TopItem < 0)
      guii->TopItem = 0;

    guii->MarkChanged();
  }
}

ScriptInvItem* InvWindow_GetItemAtIndex(GUIInvWindow *guii, int index) {
  if ((index < 0) || (index >= charextra[guii->GetCharacterId()].invorder_count))
    return nullptr;
  return &scrInv[charextra[guii->GetCharacterId()].invorder[index]];
}

//=============================================================================

int offset_over_inv(GUIInvWindow *inv) {
    if (inv->ItemWidth <= 0 || inv->ItemHeight <= 0)
        return -1;
    int mover = mouse_ifacebut_xoffs / inv->ItemWidth;
    // if it's off the edge of the visible items, ignore
    if (mover >= inv->ColCount)
        return -1;
    mover += (mouse_ifacebut_yoffs / inv->ItemHeight) * inv->ColCount;
    if (mover >= inv->ColCount * inv->RowCount)
        return -1;

    mover += inv->TopItem;
    if ((mover < 0) || (mover >= charextra[inv->GetCharacterId()].invorder_count))
        return -1;

    return charextra[inv->GetCharacterId()].invorder[mover];
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// void (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_ScrollDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIInvWindow, InvWindow_ScrollDown);
}

// void (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_ScrollUp(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIInvWindow, InvWindow_ScrollUp);
}

// CharacterInfo* (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetCharacterToUse(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIInvWindow, CharacterInfo, ccDynamicCharacter, InvWindow_GetCharacterToUse);
}

// void (GUIInvWindow *guii, CharacterInfo *chaa)
RuntimeScriptValue Sc_InvWindow_SetCharacterToUse(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUIInvWindow, InvWindow_SetCharacterToUse, CharacterInfo);
}

// ScriptInvItem* (GUIInvWindow *guii, int index)
RuntimeScriptValue Sc_InvWindow_GetItemAtIndex(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(GUIInvWindow, ScriptInvItem, ccDynamicInv, InvWindow_GetItemAtIndex);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetItemCount);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetItemHeight);
}

// void (GUIInvWindow *guii, int newhit)
RuntimeScriptValue Sc_InvWindow_SetItemHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIInvWindow, InvWindow_SetItemHeight);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetItemWidth);
}

// void (GUIInvWindow *guii, int newwidth)
RuntimeScriptValue Sc_InvWindow_SetItemWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIInvWindow, InvWindow_SetItemWidth);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemsPerRow(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetItemsPerRow);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetRowCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetRowCount);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetTopItem);
}

// void (GUIInvWindow *guii, int topitem)
RuntimeScriptValue Sc_InvWindow_SetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIInvWindow, InvWindow_SetTopItem);
}



void RegisterInventoryWindowAPI()
{
    ccAddExternalObjectFunction("InvWindow::ScrollDown^0",          Sc_InvWindow_ScrollDown);
    ccAddExternalObjectFunction("InvWindow::ScrollUp^0",            Sc_InvWindow_ScrollUp);
    ccAddExternalObjectFunction("InvWindow::get_CharacterToUse",    Sc_InvWindow_GetCharacterToUse);
    ccAddExternalObjectFunction("InvWindow::set_CharacterToUse",    Sc_InvWindow_SetCharacterToUse);
    ccAddExternalObjectFunction("InvWindow::geti_ItemAtIndex",      Sc_InvWindow_GetItemAtIndex);
    ccAddExternalObjectFunction("InvWindow::get_ItemCount",         Sc_InvWindow_GetItemCount);
    ccAddExternalObjectFunction("InvWindow::get_ItemHeight",        Sc_InvWindow_GetItemHeight);
    ccAddExternalObjectFunction("InvWindow::set_ItemHeight",        Sc_InvWindow_SetItemHeight);
    ccAddExternalObjectFunction("InvWindow::get_ItemWidth",         Sc_InvWindow_GetItemWidth);
    ccAddExternalObjectFunction("InvWindow::set_ItemWidth",         Sc_InvWindow_SetItemWidth);
    ccAddExternalObjectFunction("InvWindow::get_ItemsPerRow",       Sc_InvWindow_GetItemsPerRow);
    ccAddExternalObjectFunction("InvWindow::get_RowCount",          Sc_InvWindow_GetRowCount);
    ccAddExternalObjectFunction("InvWindow::get_TopItem",           Sc_InvWindow_GetTopItem);
    ccAddExternalObjectFunction("InvWindow::set_TopItem",           Sc_InvWindow_SetTopItem);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("InvWindow::ScrollDown^0",          (void*)InvWindow_ScrollDown);
    ccAddExternalFunctionForPlugin("InvWindow::ScrollUp^0",            (void*)InvWindow_ScrollUp);
    ccAddExternalFunctionForPlugin("InvWindow::get_CharacterToUse",    (void*)InvWindow_GetCharacterToUse);
    ccAddExternalFunctionForPlugin("InvWindow::set_CharacterToUse",    (void*)InvWindow_SetCharacterToUse);
    ccAddExternalFunctionForPlugin("InvWindow::geti_ItemAtIndex",      (void*)InvWindow_GetItemAtIndex);
    ccAddExternalFunctionForPlugin("InvWindow::get_ItemCount",         (void*)InvWindow_GetItemCount);
    ccAddExternalFunctionForPlugin("InvWindow::get_ItemHeight",        (void*)InvWindow_GetItemHeight);
    ccAddExternalFunctionForPlugin("InvWindow::set_ItemHeight",        (void*)InvWindow_SetItemHeight);
    ccAddExternalFunctionForPlugin("InvWindow::get_ItemWidth",         (void*)InvWindow_GetItemWidth);
    ccAddExternalFunctionForPlugin("InvWindow::set_ItemWidth",         (void*)InvWindow_SetItemWidth);
    ccAddExternalFunctionForPlugin("InvWindow::get_ItemsPerRow",       (void*)InvWindow_GetItemsPerRow);
    ccAddExternalFunctionForPlugin("InvWindow::get_RowCount",          (void*)InvWindow_GetRowCount);
    ccAddExternalFunctionForPlugin("InvWindow::get_TopItem",           (void*)InvWindow_GetTopItem);
    ccAddExternalFunctionForPlugin("InvWindow::set_TopItem",           (void*)InvWindow_SetTopItem);
}
