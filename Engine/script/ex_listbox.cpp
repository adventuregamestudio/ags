
//=============================================================================
//
// Exporting ListBox script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_listbox_script_functions()
{
	scAdd_External_Symbol("ListBox::AddItem^1", (void *)ListBox_AddItem);
	scAdd_External_Symbol("ListBox::Clear^0", (void *)ListBox_Clear);
	scAdd_External_Symbol("ListBox::FillDirList^1", (void *)ListBox_FillDirList);
	scAdd_External_Symbol("ListBox::FillSaveGameList^0", (void *)ListBox_FillSaveGameList);
	scAdd_External_Symbol("ListBox::GetItemAtLocation^2", (void *)ListBox_GetItemAtLocation);
	scAdd_External_Symbol("ListBox::GetItemText^2", (void *)ListBox_GetItemText);
	scAdd_External_Symbol("ListBox::InsertItemAt^2", (void *)ListBox_InsertItemAt);
	scAdd_External_Symbol("ListBox::RemoveItem^1", (void *)ListBox_RemoveItem);
	scAdd_External_Symbol("ListBox::ScrollDown^0", (void *)ListBox_ScrollDown);
	scAdd_External_Symbol("ListBox::ScrollUp^0", (void *)ListBox_ScrollUp);
	scAdd_External_Symbol("ListBox::SetItemText^2", (void *)ListBox_SetItemText);
	scAdd_External_Symbol("ListBox::get_Font", (void *)ListBox_GetFont);
	scAdd_External_Symbol("ListBox::set_Font", (void *)ListBox_SetFont);
	scAdd_External_Symbol("ListBox::get_HideBorder", (void *)ListBox_GetHideBorder);
	scAdd_External_Symbol("ListBox::set_HideBorder", (void *)ListBox_SetHideBorder);
	scAdd_External_Symbol("ListBox::get_HideScrollArrows", (void *)ListBox_GetHideScrollArrows);
	scAdd_External_Symbol("ListBox::set_HideScrollArrows", (void *)ListBox_SetHideScrollArrows);
	scAdd_External_Symbol("ListBox::get_ItemCount", (void *)ListBox_GetItemCount);
	scAdd_External_Symbol("ListBox::geti_Items", (void *)ListBox_GetItems);
	scAdd_External_Symbol("ListBox::seti_Items", (void *)ListBox_SetItemText);
	scAdd_External_Symbol("ListBox::get_RowCount", (void *)ListBox_GetRowCount);
	scAdd_External_Symbol("ListBox::geti_SaveGameSlots", (void *)ListBox_GetSaveGameSlots);
	scAdd_External_Symbol("ListBox::get_SelectedIndex", (void *)ListBox_GetSelectedIndex);
	scAdd_External_Symbol("ListBox::set_SelectedIndex", (void *)ListBox_SetSelectedIndex);
	scAdd_External_Symbol("ListBox::get_TopItem", (void *)ListBox_GetTopItem);
	scAdd_External_Symbol("ListBox::set_TopItem", (void *)ListBox_SetTopItem);
}
