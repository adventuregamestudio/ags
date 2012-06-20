
//=============================================================================
//
// Exporting Hotspot script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_hotspot_script_functions()
{
	scAdd_External_Symbol("Hotspot::GetAtScreenXY^2",(void *)GetHotspotAtLocation);
	scAdd_External_Symbol("Hotspot::GetName^1", (void*)Hotspot_GetName);
	scAdd_External_Symbol("Hotspot::GetProperty^1", (void*)Hotspot_GetProperty);
	scAdd_External_Symbol("Hotspot::GetPropertyText^2", (void*)Hotspot_GetPropertyText);
	scAdd_External_Symbol("Hotspot::GetTextProperty^1",(void *)Hotspot_GetTextProperty);
	scAdd_External_Symbol("Hotspot::RunInteraction^1", (void*)Hotspot_RunInteraction);
	scAdd_External_Symbol("Hotspot::get_Enabled", (void*)Hotspot_GetEnabled);
	scAdd_External_Symbol("Hotspot::set_Enabled", (void*)Hotspot_SetEnabled);
	scAdd_External_Symbol("Hotspot::get_ID", (void*)Hotspot_GetID);
	scAdd_External_Symbol("Hotspot::get_Name", (void*)Hotspot_GetName_New);
	scAdd_External_Symbol("Hotspot::get_WalkToX", (void*)Hotspot_GetWalkToX);
	scAdd_External_Symbol("Hotspot::get_WalkToY", (void*)Hotspot_GetWalkToY);
}
