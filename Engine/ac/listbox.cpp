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
#include "ac/listbox.h"
#include "ac/common.h"
#include "ac/file.h"
#include "ac/global_game.h"
#include "ac/string.h"
#include "game/game_objects.h"
#include "gui/guimain.h"

using AGS::Common::String;

extern int guis_need_update;
extern char saveGameDirectory[260];

// *** LIST BOX FUNCTIONS

int ListBox_AddItem(GuiListBox *lbb, const char *text) {
  if (lbb->AddItem(text) < 0)
    return 0;

  guis_need_update = 1;
  return 1;
}

int ListBox_InsertItemAt(GuiListBox *lbb, int index, const char *text) {
  if (lbb->InsertItem(index, text) < 0)
    return 0;

  guis_need_update = 1;
  return 1;
}

void ListBox_Clear(GuiListBox *listbox) {
  listbox->Clear();
  guis_need_update = 1;
}

void ListBox_FillDirList(GuiListBox *listbox, const char *filemask) {
  char searchPath[MAX_PATH];
  validate_user_file_path(filemask, searchPath, false);

  listbox->Clear();
  al_ffblk dfb;
  int	dun = al_findfirst(searchPath, &dfb, FA_SEARCH);
  while (!dun) {
    listbox->AddItem(dfb.name);
    dun = al_findnext(&dfb);
  }
  al_findclose(&dfb);
  guis_need_update = 1;
}

int ListBox_GetSaveGameSlots(GuiListBox *listbox, int index) {
  if ((index < 0) || (index >= listbox->ItemCount))
    quit("!ListBox.SaveGameSlot: index out of range");

  return listbox->SavedGameIndex[index];
}

int ListBox_FillSaveGameList(GuiListBox *listbox) {
  listbox->Clear();

  int numsaves=0;
  int bufix=0;
  al_ffblk ffb; 
  long filedates[MAXSAVEGAMES];
  char buff[200];

  char searchPath[260];
  sprintf(searchPath, "%s""agssave.*", saveGameDirectory);

  int don = al_findfirst(searchPath, &ffb, FA_SEARCH);
  while (!don) {
    bufix=0;
    if (numsaves >= MAXSAVEGAMES)
      break;
    // only list games .000 to .099 (to allow higher slots for other perposes)
    if (strstr(ffb.name,".0")==NULL) {
      don = al_findnext(&ffb);
      continue;
    }
    const char *numberExtension = strstr(ffb.name, ".0") + 1;
    int saveGameSlot = atoi(numberExtension);
    GetSaveSlotDescription(saveGameSlot, buff);
    listbox->AddItem(buff);
    listbox->SavedGameIndex[numsaves] = saveGameSlot;
    filedates[numsaves]=(long int)ffb.time;
    numsaves++;
    don = al_findnext(&ffb);
  }
  al_findclose(&ffb);

  int nn;
  for (nn=0;nn<numsaves-1;nn++) {
    for (int kk=0;kk<numsaves-1;kk++) {  // Date order the games

      if (filedates[kk] < filedates[kk+1]) {   // swap them round
        String temp_str = listbox->Items[kk];
        listbox->Items[kk] = listbox->Items[kk+1];
        listbox->Items[kk+1] = temp_str;
        int numtem = listbox->SavedGameIndex[kk];
        listbox->SavedGameIndex[kk] = listbox->SavedGameIndex[kk+1];
        listbox->SavedGameIndex[kk+1] = numtem;
        long numted=filedates[kk]; filedates[kk]=filedates[kk+1];
        filedates[kk+1]=numted;
      }
    }
  }

  // update the global savegameindex[] array for backward compatibilty
  for (nn = 0; nn < numsaves; nn++) {
    play.SavedGameFileNumbers[nn] = listbox->SavedGameIndex[nn];
  }

  guis_need_update = 1;
  listbox->ListBoxFlags |= Common::kGuiListBox_SavedGameIndexValid;

  if (numsaves >= MAXSAVEGAMES)
    return 1;
  return 0;
}

int ListBox_GetItemAtLocation(GuiListBox *listbox, int x, int y) {

  if (!guis[listbox->ParentId].IsVisible())
    return -1;

  multiply_up_coordinates(&x, &y);
  x = (x - listbox->GetX()) - guis[listbox->ParentId].GetX();
  y = (y - listbox->GetY()) - guis[listbox->ParentId].GetY();

  if ((x < 0) || (y < 0) || (x >= listbox->GetWidth()) || (y >= listbox->GetHeight()))
    return -1;
  
  return listbox->GetItemAt(x, y);
}

char *ListBox_GetItemText(GuiListBox *listbox, int index, char *buffer) {
  if ((index < 0) || (index >= listbox->ItemCount))
    quit("!ListBoxGetItemText: invalid item specified");
  strncpy(buffer, listbox->Items[index],198);
  buffer[199] = 0;
  return buffer;
}

const char* ListBox_GetItems(GuiListBox *listbox, int index) {
  if ((index < 0) || (index >= listbox->ItemCount))
    quit("!ListBox.Items: invalid index specified");

  return CreateNewScriptString(listbox->Items[index]);
}

void ListBox_SetItemText(GuiListBox *listbox, int index, const char *newtext) {
  if ((index < 0) || (index >= listbox->ItemCount))
    quit("!ListBoxSetItemText: invalid item specified");

  if (strcmp(listbox->Items[index], newtext)) {
    listbox->SetItemText(index, newtext);
    guis_need_update = 1;
  }
}

void ListBox_RemoveItem(GuiListBox *listbox, int itemIndex) {
  
  if ((itemIndex < 0) || (itemIndex >= listbox->ItemCount))
    quit("!ListBoxRemove: invalid listindex specified");

  listbox->RemoveItem(itemIndex);
  guis_need_update = 1;
}

int ListBox_GetItemCount(GuiListBox *listbox) {
  return listbox->ItemCount;
}

int ListBox_GetFont(GuiListBox *listbox) {
  return listbox->TextFont;
}

void ListBox_SetFont(GuiListBox *listbox, int newfont) {

  if ((newfont < 0) || (newfont >= game.FontCount))
    quit("!ListBox.Font: invalid TextFont number.");

  if (newfont != listbox->TextFont) {
    listbox->SetFont(newfont);
    guis_need_update = 1;
  }

}

int ListBox_GetHideBorder(GuiListBox *listbox) {
    return (listbox->ListBoxFlags & Common::kGuiListBox_NoBorder) ? 1 : 0;
}

void ListBox_SetHideBorder(GuiListBox *listbox, int newValue) {
  listbox->ListBoxFlags &= ~Common::kGuiListBox_NoBorder;
  if (newValue)
    listbox->ListBoxFlags |= Common::kGuiListBox_NoBorder;
  guis_need_update = 1;
}

int ListBox_GetHideScrollArrows(GuiListBox *listbox) {
    return (listbox->ListBoxFlags & Common::kGuiListBox_NoArrows) ? 1 : 0;
}

void ListBox_SetHideScrollArrows(GuiListBox *listbox, int newValue) {
  listbox->ListBoxFlags &= ~Common::kGuiListBox_NoArrows;
  if (newValue)
    listbox->ListBoxFlags |= Common::kGuiListBox_NoArrows;
  guis_need_update = 1;
}

int ListBox_GetSelectedIndex(GuiListBox *listbox) {
  if ((listbox->SelectedItem < 0) || (listbox->SelectedItem >= listbox->ItemCount))
    return -1;
  return listbox->SelectedItem;
}

void ListBox_SetSelectedIndex(GuiListBox *guisl, int newsel) {

  if (newsel >= guisl->ItemCount)
    newsel = -1;

  if (guisl->SelectedItem != newsel) {
    guisl->SelectedItem = newsel;
    if (newsel >= 0) {
      if (newsel < guisl->TopItem)
        guisl->TopItem = newsel;
      if (newsel >= guisl->TopItem + guisl->VisibleItemCount)
        guisl->TopItem = (newsel - guisl->VisibleItemCount) + 1;
    }
    guis_need_update = 1;
  }

}

int ListBox_GetTopItem(GuiListBox *listbox) {
  return listbox->TopItem;
}

void ListBox_SetTopItem(GuiListBox *guisl, int item) {
  if ((guisl->ItemCount == 0) && (item == 0))
    ;  // allow resetting an empty box to the top
  else if ((item >= guisl->ItemCount) || (item < 0))
    quit("!ListBoxSetTopItem: tried to set top to beyond top or bottom of list");

  guisl->TopItem = item;
  guis_need_update = 1;
}

int ListBox_GetRowCount(GuiListBox *listbox) {
  return listbox->VisibleItemCount;
}

void ListBox_ScrollDown(GuiListBox *listbox) {
  if (listbox->TopItem + listbox->VisibleItemCount < listbox->ItemCount) {
    listbox->TopItem++;
    guis_need_update = 1;
  }
}

void ListBox_ScrollUp(GuiListBox *listbox) {
  if (listbox->TopItem > 0) {
    listbox->TopItem--;
    guis_need_update = 1;
  }
}


GuiListBox* is_valid_listbox (int guin, int objn) {
  if ((guin<0) | (guin>=game.GuiCount)) quit("!ListBox: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].ControlCount)) quit("!ListBox: invalid object number");
  if (guis[guin].GetControlType(objn)!=Common::kGuiListBox)
    quit("!ListBox: specified control is not a list box");
  guis_need_update = 1;
  return (GuiListBox*)guis[guin].Controls[objn];
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

// int (GuiListBox *lbb, const char *text)
RuntimeScriptValue Sc_ListBox_AddItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(GuiListBox, ListBox_AddItem, const char);
}

// void (GuiListBox *listbox)
RuntimeScriptValue Sc_ListBox_Clear(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GuiListBox, ListBox_Clear);
}

// void (GuiListBox *listbox, const char *filemask)
RuntimeScriptValue Sc_ListBox_FillDirList(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GuiListBox, ListBox_FillDirList, const char);
}

// int (GuiListBox *listbox)
RuntimeScriptValue Sc_ListBox_FillSaveGameList(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiListBox, ListBox_FillSaveGameList);
}

// int (GuiListBox *listbox, int x, int y)
RuntimeScriptValue Sc_ListBox_GetItemAtLocation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT2(GuiListBox, ListBox_GetItemAtLocation);
}

// char *(GuiListBox *listbox, int index, char *buffer)
RuntimeScriptValue Sc_ListBox_GetItemText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT_POBJ(GuiListBox, char, myScriptStringImpl, ListBox_GetItemText, char);
}

// int (GuiListBox *lbb, int index, const char *text)
RuntimeScriptValue Sc_ListBox_InsertItemAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT_POBJ(GuiListBox, ListBox_InsertItemAt, const char);
}

// void (GuiListBox *listbox, int itemIndex)
RuntimeScriptValue Sc_ListBox_RemoveItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiListBox, ListBox_RemoveItem);
}

// void (GuiListBox *listbox)
RuntimeScriptValue Sc_ListBox_ScrollDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GuiListBox, ListBox_ScrollDown);
}

// void (GuiListBox *listbox)
RuntimeScriptValue Sc_ListBox_ScrollUp(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GuiListBox, ListBox_ScrollUp);
}

// void (GuiListBox *listbox, int index, const char *newtext)
RuntimeScriptValue Sc_ListBox_SetItemText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT_POBJ(GuiListBox, ListBox_SetItemText, const char);
}

// int (GuiListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiListBox, ListBox_GetFont);
}

// void (GuiListBox *listbox, int newfont)
RuntimeScriptValue Sc_ListBox_SetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiListBox, ListBox_SetFont);
}

// int (GuiListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetHideBorder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiListBox, ListBox_GetHideBorder);
}

// void (GuiListBox *listbox, int newValue)
RuntimeScriptValue Sc_ListBox_SetHideBorder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiListBox, ListBox_SetHideBorder);
}

// int (GuiListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetHideScrollArrows(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiListBox, ListBox_GetHideScrollArrows);
}

// void (GuiListBox *listbox, int newValue)
RuntimeScriptValue Sc_ListBox_SetHideScrollArrows(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiListBox, ListBox_SetHideScrollArrows);
}

// int (GuiListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiListBox, ListBox_GetItemCount);
}

// const char* (GuiListBox *listbox, int index)
RuntimeScriptValue Sc_ListBox_GetItems(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(GuiListBox, const char, myScriptStringImpl, ListBox_GetItems);
}

// int (GuiListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetRowCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiListBox, ListBox_GetRowCount);
}

// int (GuiListBox *listbox, int index)
RuntimeScriptValue Sc_ListBox_GetSaveGameSlots(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(GuiListBox, ListBox_GetSaveGameSlots);
}

// int (GuiListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetSelectedIndex(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiListBox, ListBox_GetSelectedIndex);
}

// void (GuiListBox *guisl, int newsel)
RuntimeScriptValue Sc_ListBox_SetSelectedIndex(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiListBox, ListBox_SetSelectedIndex);
}

// int (GuiListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiListBox, ListBox_GetTopItem);
}

// void (GuiListBox *guisl, int item)
RuntimeScriptValue Sc_ListBox_SetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiListBox, ListBox_SetTopItem);
}



void RegisterListBoxAPI()
{
    ccAddExternalObjectFunction("ListBox::AddItem^1",           Sc_ListBox_AddItem);
    ccAddExternalObjectFunction("ListBox::Clear^0",             Sc_ListBox_Clear);
    ccAddExternalObjectFunction("ListBox::FillDirList^1",       Sc_ListBox_FillDirList);
    ccAddExternalObjectFunction("ListBox::FillSaveGameList^0",  Sc_ListBox_FillSaveGameList);
    ccAddExternalObjectFunction("ListBox::GetItemAtLocation^2", Sc_ListBox_GetItemAtLocation);
    ccAddExternalObjectFunction("ListBox::GetItemText^2",       Sc_ListBox_GetItemText);
    ccAddExternalObjectFunction("ListBox::InsertItemAt^2",      Sc_ListBox_InsertItemAt);
    ccAddExternalObjectFunction("ListBox::RemoveItem^1",        Sc_ListBox_RemoveItem);
    ccAddExternalObjectFunction("ListBox::ScrollDown^0",        Sc_ListBox_ScrollDown);
    ccAddExternalObjectFunction("ListBox::ScrollUp^0",          Sc_ListBox_ScrollUp);
    ccAddExternalObjectFunction("ListBox::SetItemText^2",       Sc_ListBox_SetItemText);
    ccAddExternalObjectFunction("ListBox::get_Font",            Sc_ListBox_GetFont);
    ccAddExternalObjectFunction("ListBox::set_Font",            Sc_ListBox_SetFont);
    ccAddExternalObjectFunction("ListBox::get_HideBorder",      Sc_ListBox_GetHideBorder);
    ccAddExternalObjectFunction("ListBox::set_HideBorder",      Sc_ListBox_SetHideBorder);
    ccAddExternalObjectFunction("ListBox::get_HideScrollArrows", Sc_ListBox_GetHideScrollArrows);
    ccAddExternalObjectFunction("ListBox::set_HideScrollArrows", Sc_ListBox_SetHideScrollArrows);
    ccAddExternalObjectFunction("ListBox::get_ItemCount",       Sc_ListBox_GetItemCount);
    ccAddExternalObjectFunction("ListBox::geti_Items",          Sc_ListBox_GetItems);
    ccAddExternalObjectFunction("ListBox::seti_Items",          Sc_ListBox_SetItemText);
    ccAddExternalObjectFunction("ListBox::get_RowCount",        Sc_ListBox_GetRowCount);
    ccAddExternalObjectFunction("ListBox::geti_SaveGameSlots",  Sc_ListBox_GetSaveGameSlots);
    ccAddExternalObjectFunction("ListBox::get_SelectedIndex",   Sc_ListBox_GetSelectedIndex);
    ccAddExternalObjectFunction("ListBox::set_SelectedIndex",   Sc_ListBox_SetSelectedIndex);
    ccAddExternalObjectFunction("ListBox::get_TopItem",         Sc_ListBox_GetTopItem);
    ccAddExternalObjectFunction("ListBox::set_TopItem",         Sc_ListBox_SetTopItem);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("ListBox::AddItem^1",           (void*)ListBox_AddItem);
    ccAddExternalFunctionForPlugin("ListBox::Clear^0",             (void*)ListBox_Clear);
    ccAddExternalFunctionForPlugin("ListBox::FillDirList^1",       (void*)ListBox_FillDirList);
    ccAddExternalFunctionForPlugin("ListBox::FillSaveGameList^0",  (void*)ListBox_FillSaveGameList);
    ccAddExternalFunctionForPlugin("ListBox::GetItemAtLocation^2", (void*)ListBox_GetItemAtLocation);
    ccAddExternalFunctionForPlugin("ListBox::GetItemText^2",       (void*)ListBox_GetItemText);
    ccAddExternalFunctionForPlugin("ListBox::InsertItemAt^2",      (void*)ListBox_InsertItemAt);
    ccAddExternalFunctionForPlugin("ListBox::RemoveItem^1",        (void*)ListBox_RemoveItem);
    ccAddExternalFunctionForPlugin("ListBox::ScrollDown^0",        (void*)ListBox_ScrollDown);
    ccAddExternalFunctionForPlugin("ListBox::ScrollUp^0",          (void*)ListBox_ScrollUp);
    ccAddExternalFunctionForPlugin("ListBox::SetItemText^2",       (void*)ListBox_SetItemText);
    ccAddExternalFunctionForPlugin("ListBox::get_Font",            (void*)ListBox_GetFont);
    ccAddExternalFunctionForPlugin("ListBox::set_Font",            (void*)ListBox_SetFont);
    ccAddExternalFunctionForPlugin("ListBox::get_HideBorder",      (void*)ListBox_GetHideBorder);
    ccAddExternalFunctionForPlugin("ListBox::set_HideBorder",      (void*)ListBox_SetHideBorder);
    ccAddExternalFunctionForPlugin("ListBox::get_HideScrollArrows", (void*)ListBox_GetHideScrollArrows);
    ccAddExternalFunctionForPlugin("ListBox::set_HideScrollArrows", (void*)ListBox_SetHideScrollArrows);
    ccAddExternalFunctionForPlugin("ListBox::get_ItemCount",       (void*)ListBox_GetItemCount);
    ccAddExternalFunctionForPlugin("ListBox::geti_Items",          (void*)ListBox_GetItems);
    ccAddExternalFunctionForPlugin("ListBox::seti_Items",          (void*)ListBox_SetItemText);
    ccAddExternalFunctionForPlugin("ListBox::get_RowCount",        (void*)ListBox_GetRowCount);
    ccAddExternalFunctionForPlugin("ListBox::geti_SaveGameSlots",  (void*)ListBox_GetSaveGameSlots);
    ccAddExternalFunctionForPlugin("ListBox::get_SelectedIndex",   (void*)ListBox_GetSelectedIndex);
    ccAddExternalFunctionForPlugin("ListBox::set_SelectedIndex",   (void*)ListBox_SetSelectedIndex);
    ccAddExternalFunctionForPlugin("ListBox::get_TopItem",         (void*)ListBox_GetTopItem);
    ccAddExternalFunctionForPlugin("ListBox::set_TopItem",         (void*)ListBox_SetTopItem);
}
