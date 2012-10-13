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
// Exporting Region script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_region_script_functions()
{
	ccAddExternalStaticFunction("Region::GetAtRoomXY^2",(void *)GetRegionAtLocation);
	ccAddExternalObjectFunction("Region::Tint^4", (void*)Region_Tint);
	ccAddExternalObjectFunction("Region::RunInteraction^1", (void*)Region_RunInteraction);
	ccAddExternalObjectFunction("Region::get_Enabled", (void*)Region_GetEnabled);
	ccAddExternalObjectFunction("Region::set_Enabled", (void*)Region_SetEnabled);
	ccAddExternalObjectFunction("Region::get_ID", (void*)Region_GetID);
	ccAddExternalObjectFunction("Region::get_LightLevel", (void*)Region_GetLightLevel);
	ccAddExternalObjectFunction("Region::set_LightLevel", (void*)Region_SetLightLevel);
	ccAddExternalObjectFunction("Region::get_TintEnabled", (void*)Region_GetTintEnabled);
	ccAddExternalObjectFunction("Region::get_TintBlue", (void*)Region_GetTintBlue);
	ccAddExternalObjectFunction("Region::get_TintGreen", (void*)Region_GetTintGreen);
	ccAddExternalObjectFunction("Region::get_TintRed", (void*)Region_GetTintRed);
	ccAddExternalObjectFunction("Region::get_TintSaturation", (void*)Region_GetTintSaturation);
}
