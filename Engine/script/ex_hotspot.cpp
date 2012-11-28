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
	ccAddExternalStaticFunction("Hotspot::GetAtScreenXY^2",(void *)GetHotspotAtLocation);
	ccAddExternalObjectFunction("Hotspot::GetName^1", (void*)Hotspot_GetName);
	ccAddExternalObjectFunction("Hotspot::GetProperty^1", (void*)Hotspot_GetProperty);
	ccAddExternalObjectFunction("Hotspot::GetPropertyText^2", (void*)Hotspot_GetPropertyText);
	ccAddExternalObjectFunction("Hotspot::GetTextProperty^1",(void *)Hotspot_GetTextProperty);
	ccAddExternalObjectFunction("Hotspot::RunInteraction^1", (void*)Hotspot_RunInteraction);
	ccAddExternalObjectFunction("Hotspot::get_Enabled", (void*)Hotspot_GetEnabled);
	ccAddExternalObjectFunction("Hotspot::set_Enabled", (void*)Hotspot_SetEnabled);
	ccAddExternalObjectFunction("Hotspot::get_ID", (void*)Hotspot_GetID);
	ccAddExternalObjectFunction("Hotspot::get_Name", (void*)Hotspot_GetName_New);
	ccAddExternalObjectFunction("Hotspot::get_WalkToX", (void*)Hotspot_GetWalkToX);
	ccAddExternalObjectFunction("Hotspot::get_WalkToY", (void*)Hotspot_GetWalkToY);
}
