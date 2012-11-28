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
	ccAddExternalObjectFunction("ListBox::AddItem^1", (void *)ListBox_AddItem);
	ccAddExternalObjectFunction("ListBox::Clear^0", (void *)ListBox_Clear);
	ccAddExternalObjectFunction("ListBox::FillDirList^1", (void *)ListBox_FillDirList);
	ccAddExternalObjectFunction("ListBox::FillSaveGameList^0", (void *)ListBox_FillSaveGameList);
	ccAddExternalObjectFunction("ListBox::GetItemAtLocation^2", (void *)ListBox_GetItemAtLocation);
	ccAddExternalObjectFunction("ListBox::GetItemText^2", (void *)ListBox_GetItemText);
	ccAddExternalObjectFunction("ListBox::InsertItemAt^2", (void *)ListBox_InsertItemAt);
	ccAddExternalObjectFunction("ListBox::RemoveItem^1", (void *)ListBox_RemoveItem);
	ccAddExternalObjectFunction("ListBox::ScrollDown^0", (void *)ListBox_ScrollDown);
	ccAddExternalObjectFunction("ListBox::ScrollUp^0", (void *)ListBox_ScrollUp);
	ccAddExternalObjectFunction("ListBox::SetItemText^2", (void *)ListBox_SetItemText);
	ccAddExternalObjectFunction("ListBox::get_Font", (void *)ListBox_GetFont);
	ccAddExternalObjectFunction("ListBox::set_Font", (void *)ListBox_SetFont);
	ccAddExternalObjectFunction("ListBox::get_HideBorder", (void *)ListBox_GetHideBorder);
	ccAddExternalObjectFunction("ListBox::set_HideBorder", (void *)ListBox_SetHideBorder);
	ccAddExternalObjectFunction("ListBox::get_HideScrollArrows", (void *)ListBox_GetHideScrollArrows);
	ccAddExternalObjectFunction("ListBox::set_HideScrollArrows", (void *)ListBox_SetHideScrollArrows);
	ccAddExternalObjectFunction("ListBox::get_ItemCount", (void *)ListBox_GetItemCount);
	ccAddExternalObjectFunction("ListBox::geti_Items", (void *)ListBox_GetItems);
	ccAddExternalObjectFunction("ListBox::seti_Items", (void *)ListBox_SetItemText);
	ccAddExternalObjectFunction("ListBox::get_RowCount", (void *)ListBox_GetRowCount);
	ccAddExternalObjectFunction("ListBox::geti_SaveGameSlots", (void *)ListBox_GetSaveGameSlots);
	ccAddExternalObjectFunction("ListBox::get_SelectedIndex", (void *)ListBox_GetSelectedIndex);
	ccAddExternalObjectFunction("ListBox::set_SelectedIndex", (void *)ListBox_SetSelectedIndex);
	ccAddExternalObjectFunction("ListBox::get_TopItem", (void *)ListBox_GetTopItem);
	ccAddExternalObjectFunction("ListBox::set_TopItem", (void *)ListBox_SetTopItem);
}
