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
//
//
//=============================================================================
#ifndef __AGS_EE_PLUGIN__PLUGINFUNC_H
#define __AGS_EE_PLUGIN__PLUGINFUNC_H

void PluginSimulateMouseClick(int pluginButtonID);
bool RegisterPluginStubs(const char* name);

//=============================================================================
// Stubs for plugin functions.
void ScriptStub_ShellExecute();
void srSetSnowDriftRange(int min_value, int max_value);
void srSetSnowDriftSpeed(int min_value, int max_value);
void srSetSnowFallSpeed(int min_value, int max_value);
void srChangeSnowAmount(int amount);
void srSetSnowBaseline(int top, int bottom);
void srSetSnowTransparency(int min_value, int max_value);
void srSetSnowDefaultView(int view, int loop);
void srSetSnowWindSpeed(int value);
void srSetSnowAmount(int amount);
void srSetSnowView(int kind_id, int event, int view, int loop);
void srChangeRainAmount(int amount);
void srSetRainView(int kind_id, int event, int view, int loop);
void srSetRainDefaultView(int view, int loop);
void srSetRainTransparency(int min_value, int max_value);
void srSetRainWindSpeed(int value);
void srSetRainBaseline(int top, int bottom);
void srSetRainAmount(int amount);
void srSetRainFallSpeed(int min_value, int max_value);
void srSetWindSpeed(int value);
void srSetBaseline(int top, int bottom);
int  JoystickCount();
int  Joystick_Open(int a);
int  Joystick_IsButtonDown(int a);
void Joystick_EnableEvents(int a);
void Joystick_DisableEvents();
void Joystick_Click(int a);
int  Joystick_Valid();
int  Joystick_Unplugged();
int  DrawAlpha(int destination, int sprite, int x, int y, int transparency);
int  GetAlpha(int sprite, int x, int y);
int  PutAlpha(int sprite, int x, int y, int alpha);
int  Blur(int sprite, int radius);
int  HighPass(int sprite, int threshold);
int  DrawAdd(int destination, int sprite, int x, int y, float scale);
int  GetFlashlightInt();
void SetFlashlightInt1(int Param1);
void SetFlashlightInt2(int Param1, int Param2);
void SetFlashlightInt3(int Param1, int Param2, int Param3);
void SetFlashlightInt5(int Param1, int Param2, int Param3, int Param4, int Param5);

#endif // __AGS_EE_PLUGIN__PLUGINFUNC_H
