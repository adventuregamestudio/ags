
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
	scAdd_External_Symbol("InventoryItem::GetAtScreenXY^2", (void *)GetInvAtLocation);
	scAdd_External_Symbol("InventoryItem::IsInteractionAvailable^1", (void *)InventoryItem_CheckInteractionAvailable);
	scAdd_External_Symbol("InventoryItem::GetName^1", (void *)InventoryItem_GetName);
	scAdd_External_Symbol("InventoryItem::GetProperty^1", (void *)InventoryItem_GetProperty);
	scAdd_External_Symbol("InventoryItem::GetPropertyText^2", (void *)InventoryItem_GetPropertyText);
	scAdd_External_Symbol("InventoryItem::GetTextProperty^1",(void *)InventoryItem_GetTextProperty);
	scAdd_External_Symbol("InventoryItem::RunInteraction^1", (void *)InventoryItem_RunInteraction);
	scAdd_External_Symbol("InventoryItem::SetName^1", (void *)InventoryItem_SetName);
	scAdd_External_Symbol("InventoryItem::get_CursorGraphic", (void *)InventoryItem_GetCursorGraphic);
	scAdd_External_Symbol("InventoryItem::set_CursorGraphic", (void *)InventoryItem_SetCursorGraphic);
	scAdd_External_Symbol("InventoryItem::get_Graphic", (void *)InventoryItem_GetGraphic);
	scAdd_External_Symbol("InventoryItem::set_Graphic", (void *)InventoryItem_SetGraphic);
	scAdd_External_Symbol("InventoryItem::get_ID", (void *)InventoryItem_GetID);
	scAdd_External_Symbol("InventoryItem::get_Name", (void *)InventoryItem_GetName_New);
	scAdd_External_Symbol("InventoryItem::set_Name", (void *)InventoryItem_SetName);
}
