
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
	scAdd_External_Symbol("Region::GetAtRoomXY^2",(void *)GetRegionAtLocation);
	scAdd_External_Symbol("Region::Tint^4", (void*)Region_Tint);
	scAdd_External_Symbol("Region::RunInteraction^1", (void*)Region_RunInteraction);
	scAdd_External_Symbol("Region::get_Enabled", (void*)Region_GetEnabled);
	scAdd_External_Symbol("Region::set_Enabled", (void*)Region_SetEnabled);
	scAdd_External_Symbol("Region::get_ID", (void*)Region_GetID);
	scAdd_External_Symbol("Region::get_LightLevel", (void*)Region_GetLightLevel);
	scAdd_External_Symbol("Region::set_LightLevel", (void*)Region_SetLightLevel);
	scAdd_External_Symbol("Region::get_TintEnabled", (void*)Region_GetTintEnabled);
	scAdd_External_Symbol("Region::get_TintBlue", (void*)Region_GetTintBlue);
	scAdd_External_Symbol("Region::get_TintGreen", (void*)Region_GetTintGreen);
	scAdd_External_Symbol("Region::get_TintRed", (void*)Region_GetTintRed);
	scAdd_External_Symbol("Region::get_TintSaturation", (void*)Region_GetTintSaturation);
}
