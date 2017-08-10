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
// Stubs for plugin functions.
//
//=============================================================================

#include <string.h>
#include "ac/global_plugin.h"
#include "ac/mouse.h"
#include "util/string_utils.h"

int pluginSimulatedClick = NONE;

void PluginSimulateMouseClick(int pluginButtonID) {
    pluginSimulatedClick = pluginButtonID - 1;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "script/script_runtime.h"

RuntimeScriptValue Sc_PluginStub_Void(const RuntimeScriptValue *params, int32_t param_count)
{
    return RuntimeScriptValue();
}

RuntimeScriptValue Sc_PluginStub_Int0(const RuntimeScriptValue *params, int32_t param_count)
{
    return RuntimeScriptValue().SetInt32(0);
}

RuntimeScriptValue Sc_PluginStub_IntNeg1(const RuntimeScriptValue *params, int32_t param_count)
{
    return RuntimeScriptValue().SetInt32(-1);
}

bool RegisterPluginStubs(const char* name)
{
  // Stubs for plugin functions.

  if (stricmp(name, "ags_shell") == 0)
  {
    // ags_shell.dll
    ccAddExternalStaticFunction("ShellExecute",                 Sc_PluginStub_Void);
    return true;
  }
  else if (stricmp(name, "ags_snowrain") == 0)
  {
    // ags_snowrain.dll
    ccAddExternalStaticFunction("srSetSnowDriftRange",          Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowDriftSpeed",          Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowFallSpeed",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srChangeSnowAmount",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowBaseline",            Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowTransparency",        Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowDefaultView",         Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowWindSpeed",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowAmount",              Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetSnowView",                Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srChangeRainAmount",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainView",                Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainDefaultView",         Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainTransparency",        Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainWindSpeed",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainBaseline",            Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainAmount",              Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetRainFallSpeed",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetWindSpeed",               Sc_PluginStub_Void);
    ccAddExternalStaticFunction("srSetBaseline",                Sc_PluginStub_Void);
    return true;
  }
  else if (stricmp(name, "agsjoy") == 0)
  {
    // agsjoy.dll
    ccAddExternalStaticFunction("JoystickCount",                Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("JoystickName",                 Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("JoystickRescan",               Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Joystick::Open^1",             Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Joystick::IsOpen^1",           Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Joystick::Click^1",            Sc_PluginStub_Void);
    ccAddExternalStaticFunction("Joystick::Close^0",            Sc_PluginStub_Void);
    ccAddExternalStaticFunction("Joystick::Valid^0",            Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Joystick::Unplugged^0",        Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Joystick::GetName^0",          Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Joystick::GetAxis^1",          Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Joystick::IsButtonDown^1",     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Joystick::Update^0",           Sc_PluginStub_Void);
    ccAddExternalStaticFunction("Joystick::DisableEvents^0",    Sc_PluginStub_Void);
    ccAddExternalStaticFunction("Joystick::EnableEvents^1",     Sc_PluginStub_Void);
    return true;
  }
  else if (stricmp(name, "agsblend") == 0)
  {
    // agsblend.dll
    ccAddExternalStaticFunction("DrawAlpha",                    Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetAlpha",                     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("PutAlpha",                     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("Blur",                         Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("HighPass",                     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("DrawAdd",                      Sc_PluginStub_Int0);
    return true;
  }
  else if (stricmp(name, "agsflashlight") == 0)
  {
    // agsflashlight.dll
    ccAddExternalStaticFunction("SetFlashlightTint",            Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightTintRed",         Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightTintGreen",       Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightTintBlue",        Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightMinLightLevel",   Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightMaxLightLevel",   Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightDarkness",        Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightDarkness",        Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightDarknessSize",    Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightDarknessSize",    Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightBrightness",      Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightBrightness",      Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightBrightnessSize",  Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightBrightnessSize",  Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightPosition",        Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightPositionX",       Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightPositionY",       Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightFollowMouse",     Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightFollowMouse",     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightFollowCharacter", Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightFollowCharacter", Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightCharacterDX",     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightCharacterDY",     Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightCharacterHorz",   Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("GetFlashlightCharacterVert",   Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("SetFlashlightMask",            Sc_PluginStub_Void);
    ccAddExternalStaticFunction("GetFlashlightMask",            Sc_PluginStub_Int0);
    return true;
  }
  else if (stricmp(name, "agswadjetutil") == 0)
  {
    // agswadjetutil.dll
    ccAddExternalStaticFunction("IsOnPhone",                    Sc_PluginStub_Int0);
    ccAddExternalStaticFunction("FakeKeypress",                 Sc_PluginStub_Void);
    ccAddExternalStaticFunction("IosSetAchievementValue",       Sc_PluginStub_Void);
    ccAddExternalStaticFunction("IosGetAchievementValue",       Sc_PluginStub_IntNeg1);
    ccAddExternalStaticFunction("IosShowAchievements",          Sc_PluginStub_Void);
    ccAddExternalStaticFunction("IosResetAchievements",         Sc_PluginStub_Void);
    return true;
  }

  return false;
}
