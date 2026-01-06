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
#include "ac/listbox.h"
#include <algorithm>
#include <vector>
#include <allegro.h> // find files
#include "ac/common.h"
#include "ac/file.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/gui.h"
#include "ac/path_helper.h"
#include "ac/string.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "core/assetmanager.h"
#include "debug/debug_log.h"
#include "util/directory.h"
#include "util/path.h"

using namespace AGS::Common;

extern GameSetupStruct game;

// *** LIST BOX FUNCTIONS

int ListBox_AddItem(GUIListBox *lbb, const char *text) {
  if (lbb->AddItem(text) < 0)
    return 0;
  return 1;
}

int ListBox_InsertItemAt(GUIListBox *lbb, int index, const char *text) {
  if (lbb->InsertItem(index, text) < 0)
    return 0;
  return 1;
}

void ListBox_Clear(GUIListBox *listbox) {
  listbox->Clear();
}

void ListBox_FillDirList3(GUIListBox *listbox, const char *filemask, int file_sort, int sort_dir)
{
    file_sort = ValidateFileSort("ListBox.FillDirList", file_sort);
    sort_dir = ValidateSortDirection("ListBox.FillDirList", sort_dir);

    std::vector<String> files;
    FillDirList(files, filemask, (ScriptFileSortStyle)file_sort, (ScriptSortDirection)sort_dir);

    // TODO: method for adding item batch to speed up update
    listbox->Clear();
    for (auto it = files.cbegin(); it != files.cend(); ++it)
    {
        listbox->AddItem(*it);
    }
}

void ListBox_FillDirList(GUIListBox *listbox, const char *filemask)
{
    ListBox_FillDirList3(listbox, filemask, kScFileSort_Name, kScSortAscending);
}

int ListBox_GetSaveGameSlots(GUIListBox *listbox, int index) {
  if ((index < 0) || (static_cast<uint32_t>(index) >= listbox->GetItemCount()))
    quit("!ListBox.SaveGameSlot: index out of range");

  return listbox->GetSavedGameIndex(index);
}

// Fills ListBox with save game slots, inserts descriptions as textual items,
// and fills SavedGameIndex array with corresponding slot numbers
static void ListBox_FillSaveItems(GUIListBox *listbox, const std::vector<SaveListItem> &saves)
{
    listbox->Clear();
    listbox->SetSvgIndex(true);
    // TODO: method for adding item batch to speed up update and auto-set SvgIndex flag
    for (const auto &item : saves)
    {
        listbox->AddItem(item.Description, item.Slot);
    }

    // update the global savegameindex[] array for backward compatibilty
    for (size_t n = 0; n < LEGACY_MAXSAVEGAMES && n < saves.size(); ++n)
    {
        play.filenumbers[n] = saves[n].Slot;
    }
}

int ListBox_FillSaveGameList4(GUIListBox *listbox, int min_slot, int max_slot, int save_sort, int sort_dir)
{
    // Optionally override the max slot
    max_slot = usetup.Override.MaxSaveSlot > 0 ? usetup.Override.MaxSaveSlot : max_slot;

    if (!ValidateSaveSlotRange("ListBox.FillSaveGameList", min_slot, max_slot))
    {
        listbox->Clear();
        return 0;
    }

    save_sort = ValidateSaveGameSort("ListBox.FillSaveGameList", save_sort);
    sort_dir = ValidateSortDirection("ListBox.FillSaveGameList", sort_dir);

    std::vector<SaveListItem> saves;
    FillSaveList(saves, min_slot, max_slot, true, (ScriptSaveGameSortStyle)save_sort, (ScriptSortDirection)sort_dir);
    std::sort(saves.rbegin(), saves.rend(), SaveItemCmpByTime()); // sort by time in reverse

    // Fill in the list box
    ListBox_FillSaveItems(listbox, saves);

    // Returns TRUE if the whole range of slots is occupied
    return saves.size() > static_cast<uint32_t>(max_slot - min_slot);
}

int ListBox_FillSaveGameList(GUIListBox *listbox)
{
    return ListBox_FillSaveGameList4(listbox, 0, LEGACY_TOP_LISTEDSAVESLOT, kScFileSort_Time, kScSortDescending);
}

int ListBox_FillSaveGameList2(GUIListBox *listbox, int min_slot, int max_slot)
{
    return ListBox_FillSaveGameList4(listbox, min_slot, max_slot, kScFileSort_Time, kScSortDescending);
}

void ListBox_FillSaveGameSlots(GUIListBox *listbox, void *src_arr, int save_sort, int sort_dir)
{
    const auto &hdr = CCDynamicArray::GetHeader(src_arr);
    if (hdr.GetElemCount() == 0u)
    {
        debug_script_warn("ListBox.FillSaveGameSlots: empty array provided, skip execution");
        return;
    }

    std::vector<int> slots;
    const int *slots_arr = static_cast<const int*>(src_arr);
    slots.insert(slots.end(), slots_arr, slots_arr + hdr.GetElemCount());

    std::vector<SaveListItem> saves;
    FillSaveList(slots, saves, true, (ScriptSaveGameSortStyle)save_sort, (ScriptSortDirection)sort_dir);
    ListBox_FillSaveItems(listbox, saves);
}

int ListBox_GetItemAtLocation(GUIListBox *listbox, int x, int y) {

  if (!guis[listbox->GetParentID()].IsDisplayed())
    return -1;

  data_to_game_coords(&x, &y);
  x = (x - listbox->GetX()) - guis[listbox->GetParentID()].GetX();
  y = (y - listbox->GetY()) - guis[listbox->GetParentID()].GetY();

  if ((x < 0) || (y < 0) || (x >= listbox->GetWidth()) || (y >= listbox->GetHeight()))
    return -1;
  
  return listbox->GetItemAt(x, y);
}

char *ListBox_GetItemText(GUIListBox *listbox, int index, char *buffer) {
  if ((index < 0) || (static_cast<uint32_t>(index) >= listbox->GetItemCount()))
    quit("!ListBoxGetItemText: invalid item specified");
  snprintf(buffer, MAX_MAXSTRLEN, "%s", listbox->GetItem(index).GetCStr());
  return buffer;
}

const char* ListBox_GetItems(GUIListBox *listbox, int index) {
  if ((index < 0) || (static_cast<uint32_t>(index) >= listbox->GetItemCount()))
    quit("!ListBox.Items: invalid index specified");

  return CreateNewScriptString(listbox->GetItem(index));
}

void ListBox_SetItemText(GUIListBox *listbox, int index, const char *newtext) {
  if ((index < 0) || (static_cast<uint32_t>(index) >= listbox->GetItemCount()))
    quit("!ListBoxSetItemText: invalid item specified");

  if (listbox->GetItem(index) != newtext) {
    listbox->SetItemText(index, newtext);
  }
}

void ListBox_RemoveItem(GUIListBox *listbox, int index) {
  
  if ((index < 0) || (static_cast<uint32_t>(index) >= listbox->GetItemCount()))
    quit("!ListBoxRemove: invalid listindex specified");

  listbox->RemoveItem(index);
}

int ListBox_GetItemCount(GUIListBox *listbox) {
  return listbox->GetItemCount();
}

int ListBox_GetFont(GUIListBox *listbox) {
  return listbox->GetFont();
}

void ListBox_SetFont(GUIListBox *listbox, int newfont) {
  newfont = ValidateFontNumber("ListBox.Font", newfont);
  listbox->SetFont(newfont);
}

bool ListBox_GetShowBorder(GUIListBox *listbox) {
    return listbox->IsBorderShown();
}

void ListBox_SetShowBorder(GUIListBox *listbox, bool newValue) {
    if (listbox->IsBorderShown() != newValue)
    {
        listbox->SetShowBorder(newValue);
    }
}

bool ListBox_GetShowScrollArrows(GUIListBox *listbox) {
    return listbox->AreArrowsShown();
}

void ListBox_SetShowScrollArrows(GUIListBox *listbox, bool newValue) {
    if (listbox->AreArrowsShown() != newValue)
    {
        listbox->SetShowArrows(newValue);
    }
}

int ListBox_GetHideBorder(GUIListBox *listbox) {
    return !ListBox_GetShowBorder(listbox);
}

void ListBox_SetHideBorder(GUIListBox *listbox, int newValue) {
    ListBox_SetShowBorder(listbox, !newValue);
}

int ListBox_GetHideScrollArrows(GUIListBox *listbox) {
    return !ListBox_GetShowScrollArrows(listbox);
}

void ListBox_SetHideScrollArrows(GUIListBox *listbox, int newValue) {
    ListBox_SetShowScrollArrows(listbox, !newValue);
}

int ListBox_GetSelectedBackColor(GUIListBox *listbox) {
    return listbox->GetSelectedBgColor();
}

void ListBox_SetSelectedBackColor(GUIListBox *listbox, int colr) {
    listbox->SetSelectedBgColor(colr);
}

int ListBox_GetSelectedTextColor(GUIListBox *listbox) {
    return listbox->GetSelectedTextColor();
}

void ListBox_SetSelectedTextColor(GUIListBox *listbox, int colr) {
    listbox->SetSelectedTextColor(colr);
}

int ListBox_GetTextAlignment(GUIListBox *listbox) {
    return listbox->GetTextAlignment();
}

void ListBox_SetTextAlignment(GUIListBox *listbox, int align) {
    listbox->SetTextAlignment(static_cast<HorAlignment>(align));
}

int ListBox_GetTextColor(GUIListBox *listbox) {
    return listbox->GetTextColor();
}

void ListBox_SetTextColor(GUIListBox *listbox, int colr) {
    listbox->SetTextColor(colr);
}

int ListBox_GetSelectedIndex(GUIListBox *listbox) {
    return listbox->GetSelectedItem();
}

void ListBox_SetSelectedIndex(GUIListBox *guisl, int newsel) {
  guisl->SetSelectedItem(newsel);
}

int ListBox_GetTopItem(GUIListBox *listbox) {
  return listbox->GetTopItem();
}

void ListBox_SetTopItem(GUIListBox *guisl, int item) {
  if ((item >= guisl->GetItemCount()) || (item < 0))
  {
    item = Math::Clamp<uint32_t>(item, 0u, guisl->GetItemCount());
    debug_script_warn("ListBoxSetTopItem: tried to set top to beyond top or bottom of list");
  }
  guisl->SetTopItem(item);
}

int ListBox_GetRowCount(GUIListBox *listbox) {
  return listbox->GetVisibleItemCount();
}

void ListBox_ScrollDown(GUIListBox *listbox) {
  if (listbox->GetTopItem() + listbox->GetVisibleItemCount() < listbox->GetItemCount()) {
    listbox->SetTopItem(listbox->GetTopItem() + 1);
  }
}

void ListBox_ScrollUp(GUIListBox *listbox) {
  if (listbox->GetTopItem() > 0) {
    listbox->SetTopItem(listbox->GetTopItem() - 1);
  }
}


GUIListBox* is_valid_listbox (int guin, int objn) {
  if ((guin<0) | (guin>=game.numgui)) quit("!ListBox: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].GetControlCount())) quit("!ListBox: invalid object number");
  if (guis[guin].GetControlType(objn)!=kGUIListBox)
    quit("!ListBox: specified control is not a list box");
  return (GUIListBox*)guis[guin].GetControl(objn);
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

// int (GUIListBox *lbb, const char *text)
RuntimeScriptValue Sc_ListBox_AddItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(GUIListBox, ListBox_AddItem, const char);
}

// void (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_Clear(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIListBox, ListBox_Clear);
}

// void (GUIListBox *listbox, const char *filemask)
RuntimeScriptValue Sc_ListBox_FillDirList(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUIListBox, ListBox_FillDirList, const char);
}

RuntimeScriptValue Sc_ListBox_FillDirList3(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT2(GUIListBox, ListBox_FillDirList3, const char);
}

RuntimeScriptValue Sc_ListBox_FillSaveGameList(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_FillSaveGameList);
}

RuntimeScriptValue Sc_ListBox_FillSaveGameList2(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT2(GUIListBox, ListBox_FillSaveGameList2);
}

RuntimeScriptValue Sc_ListBox_FillSaveGameList4(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT4(GUIListBox, ListBox_FillSaveGameList4);
}

RuntimeScriptValue Sc_ListBox_FillSaveGameSlots(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT2(GUIListBox, ListBox_FillSaveGameSlots, void);
}

// int (GUIListBox *listbox, int x, int y)
RuntimeScriptValue Sc_ListBox_GetItemAtLocation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT2(GUIListBox, ListBox_GetItemAtLocation);
}

// char *(GUIListBox *listbox, int index, char *buffer)
RuntimeScriptValue Sc_ListBox_GetItemText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT_POBJ(GUIListBox, char, myScriptStringImpl, ListBox_GetItemText, char);
}

// int (GUIListBox *lbb, int index, const char *text)
RuntimeScriptValue Sc_ListBox_InsertItemAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT_POBJ(GUIListBox, ListBox_InsertItemAt, const char);
}

// void (GUIListBox *listbox, int itemIndex)
RuntimeScriptValue Sc_ListBox_RemoveItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_RemoveItem);
}

// void (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_ScrollDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIListBox, ListBox_ScrollDown);
}

// void (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_ScrollUp(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIListBox, ListBox_ScrollUp);
}

// void (GUIListBox *listbox, int index, const char *newtext)
RuntimeScriptValue Sc_ListBox_SetItemText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT_POBJ(GUIListBox, ListBox_SetItemText, const char);
}

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetFont);
}

// void (GUIListBox *listbox, int newfont)
RuntimeScriptValue Sc_ListBox_SetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetFont);
}

RuntimeScriptValue Sc_ListBox_GetShowBorder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(GUIListBox, ListBox_GetShowBorder);
}

RuntimeScriptValue Sc_ListBox_SetShowBorder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(GUIListBox, ListBox_SetShowBorder);
}

RuntimeScriptValue Sc_ListBox_GetShowScrollArrows(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(GUIListBox, ListBox_GetShowScrollArrows);
}

RuntimeScriptValue Sc_ListBox_SetShowScrollArrows(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(GUIListBox, ListBox_SetShowScrollArrows);
}

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetHideBorder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetHideBorder);
}

// void (GUIListBox *listbox, int newValue)
RuntimeScriptValue Sc_ListBox_SetHideBorder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetHideBorder);
}

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetHideScrollArrows(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetHideScrollArrows);
}

// void (GUIListBox *listbox, int newValue)
RuntimeScriptValue Sc_ListBox_SetHideScrollArrows(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetHideScrollArrows);
}

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetItemCount);
}

// const char* (GUIListBox *listbox, int index)
RuntimeScriptValue Sc_ListBox_GetItems(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(GUIListBox, const char, myScriptStringImpl, ListBox_GetItems);
}

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetRowCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetRowCount);
}

// int (GUIListBox *listbox, int index)
RuntimeScriptValue Sc_ListBox_GetSaveGameSlots(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(GUIListBox, ListBox_GetSaveGameSlots);
}

RuntimeScriptValue Sc_ListBox_GetSelectedBackColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetSelectedBackColor);
}

// void (GUIListBox *guisl, int newsel)
RuntimeScriptValue Sc_ListBox_SetSelectedBackColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetSelectedBackColor);
}

RuntimeScriptValue Sc_ListBox_GetSelectedTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetSelectedTextColor);
}

// void (GUIListBox *guisl, int newsel)
RuntimeScriptValue Sc_ListBox_SetSelectedTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetSelectedTextColor);
}

RuntimeScriptValue Sc_ListBox_GetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetTextAlignment);
}

// void (GUIListBox *guisl, int newsel)
RuntimeScriptValue Sc_ListBox_SetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetTextAlignment);
}

RuntimeScriptValue Sc_ListBox_GetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetTextColor);
}

// void (GUIListBox *guisl, int newsel)
RuntimeScriptValue Sc_ListBox_SetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetTextColor);
}

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetSelectedIndex(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetSelectedIndex);
}

// void (GUIListBox *guisl, int newsel)
RuntimeScriptValue Sc_ListBox_SetSelectedIndex(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetSelectedIndex);
}

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetTopItem);
}

// void (GUIListBox *guisl, int item)
RuntimeScriptValue Sc_ListBox_SetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetTopItem);
}



void RegisterListBoxAPI()
{
    ScFnRegister listbox_api[] = {
        { "ListBox::AddItem^1",           API_FN_PAIR(ListBox_AddItem) },
        { "ListBox::Clear^0",             API_FN_PAIR(ListBox_Clear) },
        { "ListBox::FillDirList^1",       API_FN_PAIR(ListBox_FillDirList) },
        { "ListBox::FillDirList^3",       API_FN_PAIR(ListBox_FillDirList3) },
        { "ListBox::FillSaveGameList^0",  API_FN_PAIR(ListBox_FillSaveGameList) },
        { "ListBox::FillSaveGameList^2",  API_FN_PAIR(ListBox_FillSaveGameList2) },
        { "ListBox::FillSaveGameList^4",  API_FN_PAIR(ListBox_FillSaveGameList4) },
        { "ListBox::FillSaveGameSlots^3", API_FN_PAIR(ListBox_FillSaveGameSlots) },
        { "ListBox::GetItemAtLocation^2", API_FN_PAIR(ListBox_GetItemAtLocation) },
        { "ListBox::GetItemText^2",       API_FN_PAIR(ListBox_GetItemText) },
        { "ListBox::InsertItemAt^2",      API_FN_PAIR(ListBox_InsertItemAt) },
        { "ListBox::RemoveItem^1",        API_FN_PAIR(ListBox_RemoveItem) },
        { "ListBox::ScrollDown^0",        API_FN_PAIR(ListBox_ScrollDown) },
        { "ListBox::ScrollUp^0",          API_FN_PAIR(ListBox_ScrollUp) },
        { "ListBox::SetItemText^2",       API_FN_PAIR(ListBox_SetItemText) },
        { "ListBox::get_Font",            API_FN_PAIR(ListBox_GetFont) },
        { "ListBox::set_Font",            API_FN_PAIR(ListBox_SetFont) },
        { "ListBox::get_ShowBorder",      API_FN_PAIR(ListBox_GetShowBorder) },
        { "ListBox::set_ShowBorder",      API_FN_PAIR(ListBox_SetShowBorder) },
        { "ListBox::get_ShowScrollArrows", API_FN_PAIR(ListBox_GetShowScrollArrows) },
        { "ListBox::set_ShowScrollArrows", API_FN_PAIR(ListBox_SetShowScrollArrows) },
        // old { "inverted" properties
        { "ListBox::get_HideBorder",      API_FN_PAIR(ListBox_GetHideBorder) },
        { "ListBox::set_HideBorder",      API_FN_PAIR(ListBox_SetHideBorder) },
        { "ListBox::get_HideScrollArrows", API_FN_PAIR(ListBox_GetHideScrollArrows) },
        { "ListBox::set_HideScrollArrows", API_FN_PAIR(ListBox_SetHideScrollArrows) },
        //
        { "ListBox::get_ItemCount",       API_FN_PAIR(ListBox_GetItemCount) },
        { "ListBox::geti_Items",          API_FN_PAIR(ListBox_GetItems) },
        { "ListBox::seti_Items",          API_FN_PAIR(ListBox_SetItemText) },
        { "ListBox::get_RowCount",        API_FN_PAIR(ListBox_GetRowCount) },
        { "ListBox::geti_SaveGameSlots",  API_FN_PAIR(ListBox_GetSaveGameSlots) },
        { "ListBox::get_SelectedBackColor", API_FN_PAIR(ListBox_GetSelectedBackColor) },
        { "ListBox::set_SelectedBackColor", API_FN_PAIR(ListBox_SetSelectedBackColor) },
        { "ListBox::get_SelectedIndex",   API_FN_PAIR(ListBox_GetSelectedIndex) },
        { "ListBox::set_SelectedIndex",   API_FN_PAIR(ListBox_SetSelectedIndex) },
        { "ListBox::get_SelectedTextColor", API_FN_PAIR(ListBox_GetSelectedTextColor) },
        { "ListBox::set_SelectedTextColor", API_FN_PAIR(ListBox_SetSelectedTextColor) },
        { "ListBox::get_TextAlignment",   API_FN_PAIR(ListBox_GetTextAlignment) },
        { "ListBox::set_TextAlignment",   API_FN_PAIR(ListBox_SetTextAlignment) },
        { "ListBox::get_TextColor",       API_FN_PAIR(ListBox_GetTextColor) },
        { "ListBox::set_TextColor",       API_FN_PAIR(ListBox_SetTextColor) },
        { "ListBox::get_TopItem",         API_FN_PAIR(ListBox_GetTopItem) },
        { "ListBox::set_TopItem",         API_FN_PAIR(ListBox_SetTopItem) },
    };

    ccAddExternalFunctions(listbox_api);
}
