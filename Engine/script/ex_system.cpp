
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
	ccAddExternalStaticFunction("System::get_CapsLock", (void *)System_GetCapsLock);
	ccAddExternalStaticFunction("System::get_ColorDepth", (void *)System_GetColorDepth);
	ccAddExternalStaticFunction("System::get_Gamma", (void *)System_GetGamma);
	ccAddExternalStaticFunction("System::set_Gamma", (void *)System_SetGamma);
	ccAddExternalStaticFunction("System::get_HardwareAcceleration", (void *)System_GetHardwareAcceleration);
	ccAddExternalStaticFunction("System::get_NumLock", (void *)System_GetNumLock);
	ccAddExternalStaticFunction("System::set_NumLock", (void *)System_SetNumLock);
	ccAddExternalStaticFunction("System::get_OperatingSystem", (void *)System_GetOS);
	ccAddExternalStaticFunction("System::get_ScreenHeight", (void *)System_GetScreenHeight);
	ccAddExternalStaticFunction("System::get_ScreenWidth", (void *)System_GetScreenWidth);
	ccAddExternalStaticFunction("System::get_ScrollLock", (void *)System_GetScrollLock);
	ccAddExternalStaticFunction("System::get_SupportsGammaControl", (void *)System_GetSupportsGammaControl);
	ccAddExternalStaticFunction("System::get_Version", (void *)System_GetVersion);
	ccAddExternalStaticFunction("SystemInfo::get_Version", (void *)System_GetVersion);
	ccAddExternalStaticFunction("System::get_ViewportHeight", (void *)System_GetViewportHeight);
	ccAddExternalStaticFunction("System::get_ViewportWidth", (void *)System_GetViewportWidth);
	ccAddExternalStaticFunction("System::get_Volume",(void *)System_GetVolume);
	ccAddExternalStaticFunction("System::set_Volume",(void *)System_SetVolume);
	ccAddExternalStaticFunction("System::get_VSync", (void *)System_GetVsync);
	ccAddExternalStaticFunction("System::set_VSync", (void *)System_SetVsync);
	ccAddExternalStaticFunction("System::get_Windowed", (void *)System_GetWindowed);
}
