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

#include "ac/global_plugin.h"
#include "ac/mouse.h"

int pluginSimulatedClick = NONE;

void PluginSimulateMouseClick(int pluginButtonID) {
    pluginSimulatedClick = pluginButtonID - 1;
}

//=============================================================================
// Stubs for plugin functions.

void ScriptStub_ShellExecute()
{
}
void srSetSnowDriftRange(int min_value, int max_value)
{
}
void srSetSnowDriftSpeed(int min_value, int max_value)
{
}
void srSetSnowFallSpeed(int min_value, int max_value)
{
}
void srChangeSnowAmount(int amount)
{
}
void srSetSnowBaseline(int top, int bottom)
{
}
void srSetSnowTransparency(int min_value, int max_value)
{
}
void srSetSnowDefaultView(int view, int loop)
{
}
void srSetSnowWindSpeed(int value)
{
}
void srSetSnowAmount(int amount)
{
}
void srSetSnowView(int kind_id, int event, int view, int loop)
{
}
void srChangeRainAmount(int amount)
{
}
void srSetRainView(int kind_id, int event, int view, int loop)
{
}
void srSetRainDefaultView(int view, int loop)
{
}
void srSetRainTransparency(int min_value, int max_value)
{
}
void srSetRainWindSpeed(int value)
{
}
void srSetRainBaseline(int top, int bottom)
{
}
void srSetRainAmount(int amount)
{
}
void srSetRainFallSpeed(int min_value, int max_value)
{
}
void srSetWindSpeed(int value)
{
}
void srSetBaseline(int top, int bottom)
{
}
int JoystickCount()
{
    return 0;
}
int Joystick_Open(int a)
{
    return 0;
}
int Joystick_IsButtonDown(int a)
{
    return 0;
}
void Joystick_EnableEvents(int a)
{
}
void Joystick_DisableEvents()
{
}
void Joystick_Click(int a)
{
}
int Joystick_Valid()
{
    return 0;
}
int Joystick_Unplugged()
{
    return 0;
}
int DrawAlpha(int destination, int sprite, int x, int y, int transparency)
{
    return 0;
}
int GetAlpha(int sprite, int x, int y)
{
    return 0;
}
int PutAlpha(int sprite, int x, int y, int alpha)
{
    return 0;
}
int Blur(int sprite, int radius)
{
    return 0;
}
int HighPass(int sprite, int threshold)
{
    return 0;
}
int DrawAdd(int destination, int sprite, int x, int y, float scale)
{
    return 0;
}

int GetFlashlightInt()
{
    return 0;
}
void SetFlashlightInt1(int Param1)
{
}
void SetFlashlightInt2(int Param1, int Param2)
{
}
void SetFlashlightInt3(int Param1, int Param2, int Param3)
{
}
void SetFlashlightInt5(int Param1, int Param2, int Param3, int Param4, int Param5)
{
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// void ()
RuntimeScriptValue Sc_ScriptStub_ShellExecute(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(ScriptStub_ShellExecute)
}

// void (int min_value, int max_value)
RuntimeScriptValue Sc_srSetSnowDriftRange(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetSnowDriftRange)
}

// void (int min_value, int max_value)
RuntimeScriptValue Sc_srSetSnowDriftSpeed(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetSnowDriftSpeed)
}

// void (int min_value, int max_value)
RuntimeScriptValue Sc_srSetSnowFallSpeed(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetSnowFallSpeed)
}

// void (int amount)
RuntimeScriptValue Sc_srChangeSnowAmount(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srChangeSnowAmount)
}

// void (int top, int bottom)
RuntimeScriptValue Sc_srSetSnowBaseline(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetSnowBaseline)
}

// void (int min_value, int max_value)
RuntimeScriptValue Sc_srSetSnowTransparency(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetSnowTransparency)
}

// void (int view, int loop)
RuntimeScriptValue Sc_srSetSnowDefaultView(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetSnowDefaultView)
}

// void (int value)
RuntimeScriptValue Sc_srSetSnowWindSpeed(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srSetSnowWindSpeed)
}

// void (int amount)
RuntimeScriptValue Sc_srSetSnowAmount(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srSetSnowAmount)
}

// void (int kind_id, int event, int view, int loop)
RuntimeScriptValue Sc_srSetSnowView(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(srSetSnowView)
}

// void (int amount)
RuntimeScriptValue Sc_srChangeRainAmount(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srChangeRainAmount)
}

// void (int kind_id, int event, int view, int loop)
RuntimeScriptValue Sc_srSetRainView(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(srSetRainView)
}

// void (int view, int loop)
RuntimeScriptValue Sc_srSetRainDefaultView(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetRainDefaultView)
}

// void (int min_value, int max_value)
RuntimeScriptValue Sc_srSetRainTransparency(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetRainTransparency)
}

// void (int value)
RuntimeScriptValue Sc_srSetRainWindSpeed(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srSetRainWindSpeed)
}

// void (int top, int bottom)
RuntimeScriptValue Sc_srSetRainBaseline(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetRainBaseline)
}

// void (int amount)
RuntimeScriptValue Sc_srSetRainAmount(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srSetRainAmount)
}

// void (int min_value, int max_value)
RuntimeScriptValue Sc_srSetRainFallSpeed(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetRainFallSpeed)
}

// void (int value)
RuntimeScriptValue Sc_srSetWindSpeed(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(srSetWindSpeed)
}

// void (int top, int bottom)
RuntimeScriptValue Sc_srSetBaseline(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(srSetBaseline)
}

// int ()
RuntimeScriptValue Sc_JoystickCount(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(JoystickCount)
}

// int (int a)
RuntimeScriptValue Sc_Joystick_Open(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Joystick_Open)
}

// int (int a)
RuntimeScriptValue Sc_Joystick_IsButtonDown(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Joystick_IsButtonDown)
}

// void (int a)
RuntimeScriptValue Sc_Joystick_EnableEvents(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Joystick_EnableEvents)
}

// void ()
RuntimeScriptValue Sc_Joystick_DisableEvents(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Joystick_DisableEvents)
}

// void (int a)
RuntimeScriptValue Sc_Joystick_Click(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Joystick_Click)
}

// int ()
RuntimeScriptValue Sc_Joystick_Valid(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Joystick_Valid)
}

// int ()
RuntimeScriptValue Sc_Joystick_Unplugged(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Joystick_Unplugged)
}

// int (int destination, int sprite, int x, int y, int transparency)
RuntimeScriptValue Sc_DrawAlpha(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT5(DrawAlpha)
}

// int (int sprite, int x, int y)
RuntimeScriptValue Sc_GetAlpha(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT3(GetAlpha)
}

// int (int sprite, int x, int y, int alpha)
RuntimeScriptValue Sc_PutAlpha(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT4(PutAlpha)
}

// int (int sprite, int radius)
RuntimeScriptValue Sc_Blur(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(Blur)
}

// int (int sprite, int threshold)
RuntimeScriptValue Sc_HighPass(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(HighPass)
}

// int (int destination, int sprite, int x, int y, float scale)
RuntimeScriptValue Sc_DrawAdd(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT4_PFLOAT(DrawAdd)
}

// int ()
RuntimeScriptValue Sc_GetFlashlightInt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetFlashlightInt)
}

// void (int Param1)
RuntimeScriptValue Sc_SetFlashlightInt1(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetFlashlightInt1)
}

// void (int Param1, int Param2)
RuntimeScriptValue Sc_SetFlashlightInt2(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetFlashlightInt2)
}

// void (int Param1, int Param2, int Param3)
RuntimeScriptValue Sc_SetFlashlightInt3(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetFlashlightInt3)
}

// void (int Param1, int Param2, int Param3, int Param4, int Param5)
RuntimeScriptValue Sc_SetFlashlightInt5(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT5(SetFlashlightInt5)
}


void RegisterBuiltInPluginAPI()
{
    // Stubs for plugin functions.

	// ags_shell.dll
	ccAddExternalStaticFunction("ShellExecute",                 Sc_ScriptStub_ShellExecute);

	// ags_snowrain.dll
	ccAddExternalStaticFunction("srSetSnowDriftRange",          Sc_srSetSnowDriftRange);
	ccAddExternalStaticFunction("srSetSnowDriftSpeed",          Sc_srSetSnowDriftSpeed);
	ccAddExternalStaticFunction("srSetSnowFallSpeed",           Sc_srSetSnowFallSpeed);
	ccAddExternalStaticFunction("srChangeSnowAmount",           Sc_srChangeSnowAmount);
	ccAddExternalStaticFunction("srSetSnowBaseline",            Sc_srSetSnowBaseline);
	ccAddExternalStaticFunction("srSetSnowTransparency",        Sc_srSetSnowTransparency);
	ccAddExternalStaticFunction("srSetSnowDefaultView",         Sc_srSetSnowDefaultView);
	ccAddExternalStaticFunction("srSetSnowWindSpeed",           Sc_srSetSnowWindSpeed);
	ccAddExternalStaticFunction("srSetSnowAmount",              Sc_srSetSnowAmount);
	ccAddExternalStaticFunction("srSetSnowView",                Sc_srSetSnowView);
	ccAddExternalStaticFunction("srChangeRainAmount",           Sc_srChangeRainAmount);
	ccAddExternalStaticFunction("srSetRainView",                Sc_srSetRainView);
	ccAddExternalStaticFunction("srSetRainDefaultView",         Sc_srSetRainDefaultView);
	ccAddExternalStaticFunction("srSetRainTransparency",        Sc_srSetRainTransparency);
	ccAddExternalStaticFunction("srSetRainWindSpeed",           Sc_srSetRainWindSpeed);
	ccAddExternalStaticFunction("srSetRainBaseline",            Sc_srSetRainBaseline);
	ccAddExternalStaticFunction("srSetRainAmount",              Sc_srSetRainAmount);
	ccAddExternalStaticFunction("srSetRainFallSpeed",           Sc_srSetRainFallSpeed);
	ccAddExternalStaticFunction("srSetWindSpeed",               Sc_srSetWindSpeed);
	ccAddExternalStaticFunction("srSetBaseline",                Sc_srSetBaseline);

	// agsjoy.dll
	ccAddExternalStaticFunction("JoystickCount",                Sc_JoystickCount);
	ccAddExternalStaticFunction("Joystick::Open^1",             Sc_Joystick_Open);
	ccAddExternalStaticFunction("Joystick::IsButtonDown^1",     Sc_Joystick_IsButtonDown);
	ccAddExternalStaticFunction("Joystick::EnableEvents^1",     Sc_Joystick_EnableEvents);
	ccAddExternalStaticFunction("Joystick::DisableEvents^0",    Sc_Joystick_DisableEvents);
	ccAddExternalStaticFunction("Joystick::Click^1",            Sc_Joystick_Click);
	ccAddExternalStaticFunction("Joystick::Valid^0",            Sc_Joystick_Valid);
	ccAddExternalStaticFunction("Joystick::Unplugged^0",        Sc_Joystick_Unplugged);

	// agsblend.dll
	ccAddExternalStaticFunction("DrawAlpha",                    Sc_DrawAlpha);
	ccAddExternalStaticFunction("GetAlpha",                     Sc_GetAlpha);
	ccAddExternalStaticFunction("PutAlpha",                     Sc_PutAlpha);
	ccAddExternalStaticFunction("Blur",                         Sc_Blur);
	ccAddExternalStaticFunction("HighPass",                     Sc_HighPass);
	ccAddExternalStaticFunction("DrawAdd",                      Sc_DrawAdd);

	// agsflashlight.dll
	ccAddExternalStaticFunction("SetFlashlightTint",            Sc_SetFlashlightInt3);
	ccAddExternalStaticFunction("GetFlashlightTintRed",         Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightTintGreen",       Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightTintBlue",        Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightMinLightLevel",   Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightMaxLightLevel",   Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightDarkness",        Sc_SetFlashlightInt1);
	ccAddExternalStaticFunction("GetFlashlightDarkness",        Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightDarknessSize",    Sc_SetFlashlightInt1);
	ccAddExternalStaticFunction("GetFlashlightDarknessSize",    Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightBrightness",      Sc_SetFlashlightInt1);
	ccAddExternalStaticFunction("GetFlashlightBrightness",      Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightBrightnessSize",  Sc_SetFlashlightInt1);
	ccAddExternalStaticFunction("GetFlashlightBrightnessSize",  Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightPosition",        Sc_SetFlashlightInt2);
	ccAddExternalStaticFunction("GetFlashlightPositionX",       Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightPositionY",       Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightFollowMouse",     Sc_SetFlashlightInt1);
	ccAddExternalStaticFunction("GetFlashlightFollowMouse",     Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightFollowCharacter", Sc_SetFlashlightInt5);
	ccAddExternalStaticFunction("GetFlashlightFollowCharacter", Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightCharacterDX",     Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightCharacterDY",     Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightCharacterHorz",   Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("GetFlashlightCharacterVert",   Sc_GetFlashlightInt);
	ccAddExternalStaticFunction("SetFlashlightMask",            Sc_SetFlashlightInt1);
	ccAddExternalStaticFunction("GetFlashlightMask",            Sc_GetFlashlightInt);
}
