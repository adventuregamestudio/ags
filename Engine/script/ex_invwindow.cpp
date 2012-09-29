
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
