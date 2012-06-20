
//=============================================================================
//
// Exporting System script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_system_script_functions()
{
	scAdd_External_Symbol("System::get_CapsLock", (void *)System_GetCapsLock);
	scAdd_External_Symbol("System::get_ColorDepth", (void *)System_GetColorDepth);
	scAdd_External_Symbol("System::get_Gamma", (void *)System_GetGamma);
	scAdd_External_Symbol("System::set_Gamma", (void *)System_SetGamma);
	scAdd_External_Symbol("System::get_HardwareAcceleration", (void *)System_GetHardwareAcceleration);
	scAdd_External_Symbol("System::get_NumLock", (void *)System_GetNumLock);
	scAdd_External_Symbol("System::set_NumLock", (void *)System_SetNumLock);
	scAdd_External_Symbol("System::get_OperatingSystem", (void *)System_GetOS);
	scAdd_External_Symbol("System::get_ScreenHeight", (void *)System_GetScreenHeight);
	scAdd_External_Symbol("System::get_ScreenWidth", (void *)System_GetScreenWidth);
	scAdd_External_Symbol("System::get_ScrollLock", (void *)System_GetScrollLock);
	scAdd_External_Symbol("System::get_SupportsGammaControl", (void *)System_GetSupportsGammaControl);
	scAdd_External_Symbol("System::get_Version", (void *)System_GetVersion);
	scAdd_External_Symbol("SystemInfo::get_Version", (void *)System_GetVersion);
	scAdd_External_Symbol("System::get_ViewportHeight", (void *)System_GetViewportHeight);
	scAdd_External_Symbol("System::get_ViewportWidth", (void *)System_GetViewportWidth);
	scAdd_External_Symbol("System::get_Volume",(void *)System_GetVolume);
	scAdd_External_Symbol("System::set_Volume",(void *)System_SetVolume);
	scAdd_External_Symbol("System::get_VSync", (void *)System_GetVsync);
	scAdd_External_Symbol("System::set_VSync", (void *)System_SetVsync);
	scAdd_External_Symbol("System::get_Windowed", (void *)System_GetWindowed);
}
