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
// Exporting built-in plugins script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_builtin_plugins_script_functions()
{
	// Stubs for plugin functions.

	// ags_shell.dll
	scAdd_External_Symbol("ShellExecute", (void*)ScriptStub_ShellExecute);

	// ags_snowrain.dll
	scAdd_External_Symbol("srSetSnowDriftRange",(void *)srSetSnowDriftRange);
	scAdd_External_Symbol("srSetSnowDriftSpeed",(void *)srSetSnowDriftSpeed);
	scAdd_External_Symbol("srSetSnowFallSpeed",(void *)srSetSnowFallSpeed);
	scAdd_External_Symbol("srChangeSnowAmount",(void *)srChangeSnowAmount);
	scAdd_External_Symbol("srSetSnowBaseline",(void *)srSetSnowBaseline);
	scAdd_External_Symbol("srSetSnowTransparency",(void *)srSetSnowTransparency);
	scAdd_External_Symbol("srSetSnowDefaultView",(void *)srSetSnowDefaultView);
	scAdd_External_Symbol("srSetSnowWindSpeed",(void *)srSetSnowWindSpeed);
	scAdd_External_Symbol("srSetSnowAmount",(void *)srSetSnowAmount);
	scAdd_External_Symbol("srSetSnowView",(void *)srSetSnowView);
	scAdd_External_Symbol("srChangeRainAmount",(void *)srChangeRainAmount);
	scAdd_External_Symbol("srSetRainView",(void *)srSetRainView);
	scAdd_External_Symbol("srSetRainDefaultView",(void *)srSetRainDefaultView);
	scAdd_External_Symbol("srSetRainTransparency",(void *)srSetRainTransparency);
	scAdd_External_Symbol("srSetRainWindSpeed",(void *)srSetRainWindSpeed);
	scAdd_External_Symbol("srSetRainBaseline",(void *)srSetRainBaseline);
	scAdd_External_Symbol("srSetRainAmount",(void *)srSetRainAmount);
	scAdd_External_Symbol("srSetRainFallSpeed",(void *)srSetRainFallSpeed);
	scAdd_External_Symbol("srSetWindSpeed",(void *)srSetWindSpeed);
	scAdd_External_Symbol("srSetBaseline",(void *)srSetBaseline);

	// agsjoy.dll
	scAdd_External_Symbol("JoystickCount",(void *)JoystickCount);
	scAdd_External_Symbol("Joystick::Open^1",(void *)Joystick_Open);
	scAdd_External_Symbol("Joystick::IsButtonDown^1",(void *)Joystick_IsButtonDown);
	scAdd_External_Symbol("Joystick::EnableEvents^1",(void *)Joystick_EnableEvents);
	scAdd_External_Symbol("Joystick::DisableEvents^0",(void *)Joystick_DisableEvents);
	scAdd_External_Symbol("Joystick::Click^1",(void *)Joystick_Click);
	scAdd_External_Symbol("Joystick::Valid^0",(void *)Joystick_Valid);
	scAdd_External_Symbol("Joystick::Unplugged^0",(void *)Joystick_Unplugged);

	// agsblend.dll
	scAdd_External_Symbol("DrawAlpha",(void *)DrawAlpha);
	scAdd_External_Symbol("GetAlpha",(void *)GetAlpha);
	scAdd_External_Symbol("PutAlpha",(void *)PutAlpha);
	scAdd_External_Symbol("Blur",(void *)Blur);
	scAdd_External_Symbol("HighPass",(void *)HighPass);
	scAdd_External_Symbol("DrawAdd",(void *)DrawAdd);

	// agsflashlight.dll
	scAdd_External_Symbol("SetFlashlightTint",(void *)SetFlashlightInt3);
	scAdd_External_Symbol("GetFlashlightTintRed",(void *)GetFlashlightInt);
	scAdd_External_Symbol("GetFlashlightTintGreen",(void *)GetFlashlightInt);
	scAdd_External_Symbol("GetFlashlightTintBlue",(void *)GetFlashlightInt);
	scAdd_External_Symbol("GetFlashlightMinLightLevel",(void *)GetFlashlightInt);
	scAdd_External_Symbol("GetFlashlightMaxLightLevel",(void *)GetFlashlightInt);
	scAdd_External_Symbol("SetFlashlightDarkness",(void *)SetFlashlightInt1);
	scAdd_External_Symbol("GetFlashlightDarkness",(void *)GetFlashlightInt);
	scAdd_External_Symbol("SetFlashlightDarknessSize",(void *)SetFlashlightInt1);
	scAdd_External_Symbol("GetFlashlightDarknessSize",(void *)GetFlashlightInt);
	scAdd_External_Symbol("SetFlashlightBrightness",(void *)SetFlashlightInt1);
	scAdd_External_Symbol("GetFlashlightBrightness",(void *)GetFlashlightInt);
	scAdd_External_Symbol("SetFlashlightBrightnessSize",(void *)SetFlashlightInt1);
	scAdd_External_Symbol("GetFlashlightBrightnessSize",(void *)GetFlashlightInt);
	scAdd_External_Symbol("SetFlashlightPosition",(void *)SetFlashlightInt2);
	scAdd_External_Symbol("GetFlashlightPositionX",(void *)GetFlashlightInt);
	scAdd_External_Symbol("GetFlashlightPositionY",(void *)GetFlashlightInt);
	scAdd_External_Symbol("SetFlashlightFollowMouse",(void *)SetFlashlightInt1);
	scAdd_External_Symbol("GetFlashlightFollowMouse",(void *)GetFlashlightInt);
	scAdd_External_Symbol("SetFlashlightFollowCharacter",(void *)SetFlashlightInt5);
	scAdd_External_Symbol("GetFlashlightFollowCharacter",(void *)GetFlashlightInt);
	scAdd_External_Symbol("GetFlashlightCharacterDX",(void *)GetFlashlightInt);
	scAdd_External_Symbol("GetFlashlightCharacterDY",(void *)GetFlashlightInt);
	scAdd_External_Symbol("GetFlashlightCharacterHorz",(void *)GetFlashlightInt);
	scAdd_External_Symbol("GetFlashlightCharacterVert",(void *)GetFlashlightInt);
	scAdd_External_Symbol("SetFlashlightMask",(void *)SetFlashlightInt1);
	scAdd_External_Symbol("GetFlashlightMask",(void *)GetFlashlightInt);
}
