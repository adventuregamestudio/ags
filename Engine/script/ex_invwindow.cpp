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
// Exporting InvWindow script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_invwindow_script_functions()
{
	ccAddExternalObjectFunction("InvWindow::ScrollDown^0", (void *)InvWindow_ScrollDown);
	ccAddExternalObjectFunction("InvWindow::ScrollUp^0", (void *)InvWindow_ScrollUp);
	ccAddExternalObjectFunction("InvWindow::get_CharacterToUse", (void *)InvWindow_GetCharacterToUse);
	ccAddExternalObjectFunction("InvWindow::set_CharacterToUse", (void *)InvWindow_SetCharacterToUse);
	ccAddExternalObjectFunction("InvWindow::geti_ItemAtIndex", (void *)InvWindow_GetItemAtIndex);
	ccAddExternalObjectFunction("InvWindow::get_ItemCount", (void *)InvWindow_GetItemCount);
	ccAddExternalObjectFunction("InvWindow::get_ItemHeight", (void *)InvWindow_GetItemHeight);
	ccAddExternalObjectFunction("InvWindow::set_ItemHeight", (void *)InvWindow_SetItemHeight);
	ccAddExternalObjectFunction("InvWindow::get_ItemWidth", (void *)InvWindow_GetItemWidth);
	ccAddExternalObjectFunction("InvWindow::set_ItemWidth", (void *)InvWindow_SetItemWidth);
	ccAddExternalObjectFunction("InvWindow::get_ItemsPerRow", (void *)InvWindow_GetItemsPerRow);
	ccAddExternalObjectFunction("InvWindow::get_RowCount", (void *)InvWindow_GetRowCount);
	ccAddExternalObjectFunction("InvWindow::get_TopItem", (void *)InvWindow_GetTopItem);
	ccAddExternalObjectFunction("InvWindow::set_TopItem", (void *)InvWindow_SetTopItem);
}
