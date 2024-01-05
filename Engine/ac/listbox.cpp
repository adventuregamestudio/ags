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
#include "ac/listbox.h"
#include <algorithm>
#include <vector>
#include <allegro.h> // find files
#include "ac/common.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/path_helper.h"
#include "ac/string.h"
#include "core/assetmanager.h"
#include "gui/guimain.h"
#include "debug/debug_log.h"
#include "util/directory.h"
#include "util/path.h"

using namespace AGS::Common;

extern GameState play;
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

void FillDirList(std::vector<String> &files, const FSLocation &loc, const String &pattern)
{
    // Do ci search for the location, as parts of the path may have case mismatch
    String path = File::FindFileCI(loc.BaseDir, loc.SubDir, true);
    if (path.IsEmpty())
        return;
    for (FindFile ff = FindFile::OpenFiles(path, pattern); !ff.AtEnd(); ff.Next())
        files.push_back(ff.Current());
}

void ListBox_FillDirList(GUIListBox *listbox, const char *filemask) {
  listbox->Clear();

  ResolvedPath rp, alt_rp;
  if (!ResolveScriptPath(filemask, true, rp, alt_rp))
    return;

  std::vector<String> files;
  if (rp.AssetMgr)
  {
    AssetMgr->FindAssets(files, rp.FullPath, "*");
  }
  else
  {
    FillDirList(files, rp.Loc, Path::GetFilename(rp.FullPath));
    if (alt_rp)
      FillDirList(files, alt_rp.Loc, Path::GetFilename(alt_rp.FullPath));
    // Sort and remove duplicates
    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end(), StrEqNoCase()), files.end());
  }

  // TODO: method for adding item batch to speed up update
  for (auto it = files.cbegin(); it != files.cend(); ++it)
  {
    listbox->AddItem(*it);
  }
}

int ListBox_GetSaveGameSlots(GUIListBox *listbox, int index) {
  if ((index < 0) || (index >= listbox->ItemCount))
    quit("!ListBox.SaveGameSlot: index out of range");

  return listbox->SavedGameIndex[index];
}

int ListBox_FillSaveGameList(GUIListBox *listbox) {
  // TODO: find out if limiting to MAXSAVEGAMES is still necessary here
  std::vector<SaveListItem> saves;
  FillSaveList(saves, TOP_LISTEDSAVESLOT, MAXSAVEGAMES);
  std::sort(saves.rbegin(), saves.rend());

  // fill in the list box
  listbox->Clear();
  // TODO: method for adding item batch to speed up update
  for (const auto &item : saves)
  {
    listbox->AddItem(item.Description);
    listbox->SavedGameIndex[listbox->ItemCount - 1] = item.Slot;
  }

  // update the global savegameindex[] array for backward compatibilty
  for (size_t n = 0; n < saves.size(); ++n)
  {
    play.filenumbers[n] = saves[n].Slot;
  }

  listbox->SetSvgIndex(true);

  if (saves.size() >= MAXSAVEGAMES)
    return 1;
  return 0;
}

int ListBox_GetItemAtLocation(GUIListBox *listbox, int x, int y) {

  if (!guis[listbox->ParentId].IsDisplayed())
    return -1;

  data_to_game_coords(&x, &y);
  x = (x - listbox->X) - guis[listbox->ParentId].X;
  y = (y - listbox->Y) - guis[listbox->ParentId].Y;

  if ((x < 0) || (y < 0) || (x >= listbox->GetWidth()) || (y >= listbox->GetHeight()))
    return -1;
  
  return listbox->GetItemAt(x, y);
}

char *ListBox_GetItemText(GUIListBox *listbox, int index, char *buffer) {
  if ((index < 0) || (index >= listbox->ItemCount))
    quit("!ListBoxGetItemText: invalid item specified");
  snprintf(buffer, MAX_MAXSTRLEN, "%s", listbox->Items[index].GetCStr());
  return buffer;
}

const char* ListBox_GetItems(GUIListBox *listbox, int index) {
  if ((index < 0) || (index >= listbox->ItemCount))
    quit("!ListBox.Items: invalid index specified");

  return CreateNewScriptString(listbox->Items[index]);
}

void ListBox_SetItemText(GUIListBox *listbox, int index, const char *newtext) {
  if ((index < 0) || (index >= listbox->ItemCount))
    quit("!ListBoxSetItemText: invalid item specified");

  if (listbox->Items[index] != newtext) {
    listbox->SetItemText(index, newtext);
  }
}

void ListBox_RemoveItem(GUIListBox *listbox, int itemIndex) {
  
  if ((itemIndex < 0) || (itemIndex >= listbox->ItemCount))
    quit("!ListBoxRemove: invalid listindex specified");

  listbox->RemoveItem(itemIndex);
}

int ListBox_GetItemCount(GUIListBox *listbox) {
  return listbox->ItemCount;
}

int ListBox_GetFont(GUIListBox *listbox) {
  return listbox->Font;
}

void ListBox_SetFont(GUIListBox *listbox, int newfont) {

  if ((newfont < 0) || (newfont >= game.numfonts))
    quit("!ListBox.Font: invalid font number.");

  if (newfont != listbox->Font) {
    listbox->SetFont(newfont);
  }

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
    return listbox->SelectedBgColor;
}

void ListBox_SetSelectedBackColor(GUIListBox *listbox, int colr) {
    if (listbox->SelectedBgColor != colr) {
        listbox->SelectedBgColor = colr;
        listbox->MarkChanged();
    }
}

int ListBox_GetSelectedTextColor(GUIListBox *listbox) {
    return listbox->SelectedTextColor;
}

void ListBox_SetSelectedTextColor(GUIListBox *listbox, int colr) {
    if (listbox->SelectedTextColor != colr) {
        listbox->SelectedTextColor = colr;
        listbox->MarkChanged();
    }
}

int ListBox_GetTextAlignment(GUIListBox *listbox) {
    return listbox->TextAlignment;
}

void ListBox_SetTextAlignment(GUIListBox *listbox, int align) {
    if (listbox->TextAlignment != align) {
        listbox->TextAlignment = (HorAlignment)align;
        listbox->MarkChanged();
    }
}

int ListBox_GetTextColor(GUIListBox *listbox) {
    return listbox->TextColor;
}

void ListBox_SetTextColor(GUIListBox *listbox, int colr) {
    if (listbox->TextColor != colr) {
        listbox->TextColor = colr;
        listbox->MarkChanged();
    }
}

int ListBox_GetSelectedIndex(GUIListBox *listbox) {
  if ((listbox->SelectedItem < 0) || (listbox->SelectedItem >= listbox->ItemCount))
    return -1;
  return listbox->SelectedItem;
}

void ListBox_SetSelectedIndex(GUIListBox *guisl, int newsel) {

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
    guisl->MarkChanged();
  }

}

int ListBox_GetTopItem(GUIListBox *listbox) {
  return listbox->TopItem;
}

void ListBox_SetTopItem(GUIListBox *guisl, int item) {
  if ((item >= guisl->ItemCount) || (item < 0))
  {
    item = Math::Clamp(item, 0, guisl->ItemCount);
    debug_script_warn("ListBoxSetTopItem: tried to set top to beyond top or bottom of list");
  }
  if (guisl->TopItem != item)
  {
    guisl->TopItem = item;
    guisl->MarkChanged();
  }
}

int ListBox_GetRowCount(GUIListBox *listbox) {
  return listbox->VisibleItemCount;
}

void ListBox_ScrollDown(GUIListBox *listbox) {
  if (listbox->TopItem + listbox->VisibleItemCount < listbox->ItemCount) {
    listbox->TopItem++;
    listbox->MarkChanged();
  }
}

void ListBox_ScrollUp(GUIListBox *listbox) {
  if (listbox->TopItem > 0) {
    listbox->TopItem--;
    listbox->MarkChanged();
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

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_FillSaveGameList(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_FillSaveGameList);
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
        { "ListBox::FillSaveGameList^0",  API_FN_PAIR(ListBox_FillSaveGameList) },
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
