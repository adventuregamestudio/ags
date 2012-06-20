
//=============================================================================
//
// Exporting Overlay script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_overlay_script_functions()
{
	scAdd_External_Symbol("Overlay::CreateGraphical^4", (void *)Overlay_CreateGraphical);
	scAdd_External_Symbol("Overlay::CreateTextual^106", (void *)Overlay_CreateTextual);
	scAdd_External_Symbol("Overlay::SetText^104", (void *)Overlay_SetText);
	scAdd_External_Symbol("Overlay::Remove^0", (void *)Overlay_Remove);
	scAdd_External_Symbol("Overlay::get_Valid", (void *)Overlay_GetValid);
	scAdd_External_Symbol("Overlay::get_X", (void *)Overlay_GetX);
	scAdd_External_Symbol("Overlay::set_X", (void *)Overlay_SetX);
	scAdd_External_Symbol("Overlay::get_Y", (void *)Overlay_GetY);
	scAdd_External_Symbol("Overlay::set_Y", (void *)Overlay_SetY);
}
