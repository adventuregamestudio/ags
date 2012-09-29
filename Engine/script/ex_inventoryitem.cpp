
//=============================================================================
//
// Exporting InventoryItem script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_inventoryitem_script_functions()
{
	ccAddExternalStaticFunction("InventoryItem::GetAtScreenXY^2", (void *)GetInvAtLocation);
	ccAddExternalObjectFunction("InventoryItem::IsInteractionAvailable^1", (void *)InventoryItem_CheckInteractionAvailable);
	ccAddExternalObjectFunction("InventoryItem::GetName^1", (void *)InventoryItem_GetName);
	ccAddExternalObjectFunction("InventoryItem::GetProperty^1", (void *)InventoryItem_GetProperty);
	ccAddExternalObjectFunction("InventoryItem::GetPropertyText^2", (void *)InventoryItem_GetPropertyText);
	ccAddExternalObjectFunction("InventoryItem::GetTextProperty^1",(void *)InventoryItem_GetTextProperty);
	ccAddExternalObjectFunction("InventoryItem::RunInteraction^1", (void *)InventoryItem_RunInteraction);
	ccAddExternalObjectFunction("InventoryItem::SetName^1", (void *)InventoryItem_SetName);
	ccAddExternalObjectFunction("InventoryItem::get_CursorGraphic", (void *)InventoryItem_GetCursorGraphic);
	ccAddExternalObjectFunction("InventoryItem::set_CursorGraphic", (void *)InventoryItem_SetCursorGraphic);
	ccAddExternalObjectFunction("InventoryItem::get_Graphic", (void *)InventoryItem_GetGraphic);
	ccAddExternalObjectFunction("InventoryItem::set_Graphic", (void *)InventoryItem_SetGraphic);
	ccAddExternalObjectFunction("InventoryItem::get_ID", (void *)InventoryItem_GetID);
	ccAddExternalObjectFunction("InventoryItem::get_Name", (void *)InventoryItem_GetName_New);
	ccAddExternalObjectFunction("InventoryItem::set_Name", (void *)InventoryItem_SetName);
}
