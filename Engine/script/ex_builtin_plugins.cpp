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
	ccAddExternalStaticFunction("ShellExecute", (void*)ScriptStub_ShellExecute);

	// ags_snowrain.dll
	ccAddExternalStaticFunction("srSetSnowDriftRange",(void *)srSetSnowDriftRange);
	ccAddExternalStaticFunction("srSetSnowDriftSpeed",(void *)srSetSnowDriftSpeed);
	ccAddExternalStaticFunction("srSetSnowFallSpeed",(void *)srSetSnowFallSpeed);
	ccAddExternalStaticFunction("srChangeSnowAmount",(void *)srChangeSnowAmount);
	ccAddExternalStaticFunction("srSetSnowBaseline",(void *)srSetSnowBaseline);
	ccAddExternalStaticFunction("srSetSnowTransparency",(void *)srSetSnowTransparency);
	ccAddExternalStaticFunction("srSetSnowDefaultView",(void *)srSetSnowDefaultView);
	ccAddExternalStaticFunction("srSetSnowWindSpeed",(void *)srSetSnowWindSpeed);
	ccAddExternalStaticFunction("srSetSnowAmount",(void *)srSetSnowAmount);
	ccAddExternalStaticFunction("srSetSnowView",(void *)srSetSnowView);
	ccAddExternalStaticFunction("srChangeRainAmount",(void *)srChangeRainAmount);
	ccAddExternalStaticFunction("srSetRainView",(void *)srSetRainView);
	ccAddExternalStaticFunction("srSetRainDefaultView",(void *)srSetRainDefaultView);
	ccAddExternalStaticFunction("srSetRainTransparency",(void *)srSetRainTransparency);
	ccAddExternalStaticFunction("srSetRainWindSpeed",(void *)srSetRainWindSpeed);
	ccAddExternalStaticFunction("srSetRainBaseline",(void *)srSetRainBaseline);
	ccAddExternalStaticFunction("srSetRainAmount",(void *)srSetRainAmount);
	ccAddExternalStaticFunction("srSetRainFallSpeed",(void *)srSetRainFallSpeed);
	ccAddExternalStaticFunction("srSetWindSpeed",(void *)srSetWindSpeed);
	ccAddExternalStaticFunction("srSetBaseline",(void *)srSetBaseline);

	// agsjoy.dll
	ccAddExternalStaticFunction("JoystickCount",(void *)JoystickCount);
	ccAddExternalStaticFunction("Joystick::Open^1",(void *)Joystick_Open);
	ccAddExternalStaticFunction("Joystick::IsButtonDown^1",(void *)Joystick_IsButtonDown);
	ccAddExternalStaticFunction("Joystick::EnableEvents^1",(void *)Joystick_EnableEvents);
	ccAddExternalStaticFunction("Joystick::DisableEvents^0",(void *)Joystick_DisableEvents);
	ccAddExternalStaticFunction("Joystick::Click^1",(void *)Joystick_Click);
	ccAddExternalStaticFunction("Joystick::Valid^0",(void *)Joystick_Valid);
	ccAddExternalStaticFunction("Joystick::Unplugged^0",(void *)Joystick_Unplugged);

	// agsblend.dll
	ccAddExternalStaticFunction("DrawAlpha",(void *)DrawAlpha);
	ccAddExternalStaticFunction("GetAlpha",(void *)GetAlpha);
	ccAddExternalStaticFunction("PutAlpha",(void *)PutAlpha);
	ccAddExternalStaticFunction("Blur",(void *)Blur);
	ccAddExternalStaticFunction("HighPass",(void *)HighPass);
	ccAddExternalStaticFunction("DrawAdd",(void *)DrawAdd);

	// agsflashlight.dll
	ccAddExternalStaticFunction("SetFlashlightTint",(void *)SetFlashlightInt3);
	ccAddExternalStaticFunction("GetFlashlightTintRed",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightTintGreen",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightTintBlue",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightMinLightLevel",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightMaxLightLevel",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightDarkness",(void *)SetFlashlightInt1);
	ccAddExternalStaticFunction("GetFlashlightDarkness",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightDarknessSize",(void *)SetFlashlightInt1);
	ccAddExternalStaticFunction("GetFlashlightDarknessSize",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightBrightness",(void *)SetFlashlightInt1);
	ccAddExternalStaticFunction("GetFlashlightBrightness",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightBrightnessSize",(void *)SetFlashlightInt1);
	ccAddExternalStaticFunction("GetFlashlightBrightnessSize",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightPosition",(void *)SetFlashlightInt2);
	ccAddExternalStaticFunction("GetFlashlightPositionX",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightPositionY",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightFollowMouse",(void *)SetFlashlightInt1);
	ccAddExternalStaticFunction("GetFlashlightFollowMouse",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightFollowCharacter",(void *)SetFlashlightInt5);
	ccAddExternalStaticFunction("GetFlashlightFollowCharacter",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightCharacterDX",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightCharacterDY",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightCharacterHorz",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightCharacterVert",(void *)GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightMask",(void *)SetFlashlightInt1);
	ccAddExternalStaticFunction("GetFlashlightMask",(void *)GetFlashlightInt);
}
