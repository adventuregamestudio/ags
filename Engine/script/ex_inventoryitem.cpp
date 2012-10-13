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
