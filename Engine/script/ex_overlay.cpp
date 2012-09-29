
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
	ccAddExternalStaticFunction("Overlay::CreateGraphical^4", (void *)Overlay_CreateGraphical);
	ccAddExternalStaticFunction("Overlay::CreateTextual^106", (void *)Overlay_CreateTextual);
	ccAddExternalObjectFunction("Overlay::SetText^104", (void *)Overlay_SetText);
	ccAddExternalObjectFunction("Overlay::Remove^0", (void *)Overlay_Remove);
	ccAddExternalObjectFunction("Overlay::get_Valid", (void *)Overlay_GetValid);
	ccAddExternalObjectFunction("Overlay::get_X", (void *)Overlay_GetX);
	ccAddExternalObjectFunction("Overlay::set_X", (void *)Overlay_SetX);
	ccAddExternalObjectFunction("Overlay::get_Y", (void *)Overlay_GetY);
	ccAddExternalObjectFunction("Overlay::set_Y", (void *)Overlay_SetY);
}
