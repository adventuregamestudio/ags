//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <algorithm>
#include <vector>
#include "ac/string.h"
#include "ac/gamestructdefines.h"
#include "ac/joystick.h"
#include "SDL_joystick.h"
#include "SDL_gamecontroller.h"
#include "debug/debug_log.h"
#include "script/script_api.h"
#include "ac/dynobj/dynobj_manager.h"

bool is_default_gamepad_skip_button_pressed(eAGSGamepad_Button gbn)
{
    return (gbn == eAGSGamepad_ButtonA) ||
    (gbn == eAGSGamepad_ButtonB) ||
    (gbn == eAGSGamepad_ButtonX) ||
    (gbn == eAGSGamepad_ButtonY);
}

// clamps to -1.0 to 1.0, taking into account a dead-zone from 0.0
float gamepad_clamp_val(float val, float dead_zone)
{
    dead_zone = std::max(dead_zone, 0.01f); // ignore negative values, but force minimal deadzone

    if (val < 0.0f) {
        val = val < -dead_zone ? val : 0.0f;
    } else {
        val = val > dead_zone ? val : 0.0f;
    }

    if (val < -0.99f) return -1.0f;
    if (val > 0.99f) return 1.0f;

    return val;
}

float axis_to_float_with_deadzone(int axis_val_int, float dead_zone) {
    float axis_val = axis_val_int < 0 ? static_cast<float>(axis_val_int) / 32768.0f : static_cast<float>(axis_val_int) / 32767.0f;
    return gamepad_clamp_val(axis_val, dead_zone);
}

SDL_GameControllerAxis Gamepad_Axis_AGStoSDL( eAGSGamepad_Axis ags_axis) {
    switch (ags_axis)
    {
        case eAGSGamepad_AxisInvalid: return SDL_CONTROLLER_AXIS_INVALID;
        case eAGSGamepad_AxisLeftX: return SDL_CONTROLLER_AXIS_LEFTX;
        case eAGSGamepad_AxisLeftY: return SDL_CONTROLLER_AXIS_LEFTY;
        case eAGSGamepad_AxisRightX: return SDL_CONTROLLER_AXIS_RIGHTX;
        case eAGSGamepad_AxisRightY: return SDL_CONTROLLER_AXIS_RIGHTY;
        case eAGSGamepad_AxisTriggerLeft: return SDL_CONTROLLER_AXIS_TRIGGERLEFT;
        case eAGSGamepad_AxisTriggerRight: return SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    }
    return SDL_CONTROLLER_AXIS_INVALID;
}

SDL_GameControllerButton Gamepad_Button_AGStoSDL(eAGSGamepad_Button ags_button) {
    switch (ags_button)
    {
        case eAGSGamepad_ButtonInvalid: return SDL_CONTROLLER_BUTTON_INVALID;
        case eAGSGamepad_ButtonA: return SDL_CONTROLLER_BUTTON_A;
        case eAGSGamepad_ButtonB: return SDL_CONTROLLER_BUTTON_B;
        case eAGSGamepad_ButtonX: return SDL_CONTROLLER_BUTTON_X;
        case eAGSGamepad_ButtonY: return SDL_CONTROLLER_BUTTON_Y;
        case eAGSGamepad_ButtonBack: return SDL_CONTROLLER_BUTTON_BACK;
        case eAGSGamepad_ButtonGuide: return SDL_CONTROLLER_BUTTON_GUIDE;
        case eAGSGamepad_ButtonStart: return SDL_CONTROLLER_BUTTON_START;
        case eAGSGamepad_ButtonLeftStick: return  SDL_CONTROLLER_BUTTON_LEFTSTICK;
        case eAGSGamepad_ButtonRightStick: return  SDL_CONTROLLER_BUTTON_RIGHTSTICK;
        case eAGSGamepad_ButtonLeftShoulder: return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
        case eAGSGamepad_ButtonRightShoulder: return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
        case eAGSGamepad_ButtonDpadUp: return  SDL_CONTROLLER_BUTTON_DPAD_UP;
        case eAGSGamepad_ButtonDpadDown: return  SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        case eAGSGamepad_ButtonDpadLeft: return  SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        case eAGSGamepad_ButtonDpadRight: return  SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    }
    return SDL_CONTROLLER_BUTTON_INVALID;
}

eAGSJoystick_Hat Joystick_Hat_SDLtoAGS(uint8_t sdl_hat) {
    switch (sdl_hat)
    {
        case SDL_HAT_CENTERED: return eAGSJoystick_HatCentered;
        case SDL_HAT_UP: return eAGSJoystick_HatUp;
        case SDL_HAT_RIGHT: return eAGSJoystick_HatRight;
        case SDL_HAT_DOWN: return eAGSJoystick_HatDown;
        case SDL_HAT_LEFT: return eAGSJoystick_HatLeft;
        case SDL_HAT_RIGHTUP: return eAGSJoystick_HatRightUp;
        case SDL_HAT_RIGHTDOWN: return eAGSJoystick_HatRightDown;
        case SDL_HAT_LEFTUP: return eAGSJoystick_HatLeftUp;
        case SDL_HAT_LEFTDOWN: return eAGSJoystick_HatLeftDown;
    }
    return eAGSJoystick_HatCentered;
}

uint8_t Joystick_Hat_AGStoSDL(eAGSJoystick_Hat ags_hat)
{
    switch (ags_hat)
    {
        case eAGSJoystick_HatCentered: return SDL_HAT_CENTERED;
        case eAGSJoystick_HatUp: return SDL_HAT_UP;
        case eAGSJoystick_HatRight: return SDL_HAT_RIGHT;
        case eAGSJoystick_HatDown: return SDL_HAT_DOWN;
        case eAGSJoystick_HatLeft: return  SDL_HAT_LEFT;
        case eAGSJoystick_HatRightUp: return  SDL_HAT_RIGHTUP;
        case eAGSJoystick_HatRightDown: return SDL_HAT_RIGHTDOWN;
        case eAGSJoystick_HatLeftUp: return SDL_HAT_LEFTUP;
        case eAGSJoystick_HatLeftDown: return SDL_HAT_LEFTDOWN;
    }
    return SDL_HAT_CENTERED;
}

eAGSGamepad_Button Gamepad_Button_SDLtoAGS(SDL_GameControllerButton sdl_button) {
    switch (sdl_button)
    {
        case SDL_CONTROLLER_BUTTON_INVALID: return eAGSGamepad_ButtonInvalid;
        case SDL_CONTROLLER_BUTTON_A: return eAGSGamepad_ButtonA;
        case SDL_CONTROLLER_BUTTON_B: return eAGSGamepad_ButtonB;
        case SDL_CONTROLLER_BUTTON_X: return eAGSGamepad_ButtonX;
        case SDL_CONTROLLER_BUTTON_Y: return eAGSGamepad_ButtonY;
        case SDL_CONTROLLER_BUTTON_BACK: return eAGSGamepad_ButtonBack;
        case SDL_CONTROLLER_BUTTON_GUIDE: return eAGSGamepad_ButtonGuide;
        case SDL_CONTROLLER_BUTTON_START: return eAGSGamepad_ButtonStart;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: return eAGSGamepad_ButtonLeftStick;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return eAGSGamepad_ButtonRightStick;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return eAGSGamepad_ButtonLeftShoulder;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return eAGSGamepad_ButtonRightShoulder;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return eAGSGamepad_ButtonDpadUp;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return eAGSGamepad_ButtonDpadDown;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return eAGSGamepad_ButtonDpadLeft;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return eAGSGamepad_ButtonDpadRight;
    }
    return eAGSGamepad_ButtonInvalid;
}

struct JoystickImpl {
    SDL_GameController* sdlGameController = nullptr;
    SDL_Joystick* sdlJoystick = nullptr;
    SDL_JoystickID sdlJoystickID = -1;
    int scriptJoystickHandle;
};

std::vector<JoystickImpl> _joysticks;

void add_joystick(int device_index)
{
    // in case the platform also generates connection events at start, see note in init_joystick()
    if(_joysticks.size() != device_index) return;

    SDL_GameController* sdl_gamepad = nullptr;
    SDL_Joystick* sdl_joy = nullptr;

    if (SDL_IsGameController(device_index))
    {
        sdl_gamepad = SDL_GameControllerOpen(device_index);
        sdl_joy = SDL_GameControllerGetJoystick(sdl_gamepad);
    }
    else
    {
        sdl_joy = SDL_JoystickOpen(device_index);
    }

    if (!sdl_joy) return;

    SDL_JoystickID sdl_id = SDL_JoystickInstanceID(sdl_joy);

    JoystickImpl joy;
    joy.sdlJoystick = sdl_joy;
    joy.sdlGameController = sdl_gamepad;
    joy.sdlJoystickID = sdl_id;
    joy.scriptJoystickHandle = -1;
    _joysticks.push_back(joy);
}

void remove_joystick(int instance_id)
{
    SDL_JoystickID sdl_id = instance_id;
    auto it = std::find_if(_joysticks.begin(), _joysticks.end(),
                           [sdl_id](const JoystickImpl& joystick) {
                               return joystick.sdlJoystickID == sdl_id;
                           });

    // remove the joystick from list if it was found
    if (it != _joysticks.end()) {
        // need to do something with the script reference here!
        if(it->scriptJoystickHandle != -1) {
            auto* scriptJoystick = (ScriptJoystick*) ccGetObjectAddressFromHandle(it->scriptJoystickHandle);
            scriptJoystick->Invalidate();
            ccReleaseObjectReference(it->scriptJoystickHandle);
        }

        _joysticks.erase(it);
    }

    for(int i=0; i<_joysticks.size(); i++)
    {
        auto const& joy = _joysticks[i];
        if(joy.scriptJoystickHandle != -1) {
            auto* scriptJoystick = (ScriptJoystick*) ccGetObjectAddressFromHandle(joy.scriptJoystickHandle);
            scriptJoystick->SetID(i);
        }
    }
}

// it looks like on desktop platforms a connection event is always generated and thus this is not needed
void init_joystick()
{
    if(!_joysticks.empty()) return;
    int joy_count = SDL_NumJoysticks();
    for(int i=0; i<joy_count; i++) {
        add_joystick(i);
    }
}

void JoystickConnectionEvent(const SDL_JoyDeviceEvent &event)
{
    if (event.type == SDL_JOYDEVICEADDED) add_joystick(event.which);
    else if (event.type == SDL_JOYDEVICEREMOVED) remove_joystick(event.which);
}

int32_t Joystick_GetJoystickCount()
{
    return static_cast<int32_t>(_joysticks.size());
}

ScriptJoystick* Joystick_GetiJoysticks(int i)
{
    if(i<0 || i>=_joysticks.size()) return nullptr;

    if(_joysticks[i].scriptJoystickHandle != -1) {
        return static_cast<ScriptJoystick *>(ccGetObjectAddressFromHandle(_joysticks[i].scriptJoystickHandle));
    }

    ScriptJoystick *newJoy = new ScriptJoystick(i);
    int handle = ccRegisterManagedObject(newJoy, newJoy);
    // make sure an internal reference is kept
    ccAddObjectReference(handle);
    _joysticks[i].scriptJoystickHandle = handle;
    return newJoy;
}

int Joystick_IsConnected(ScriptJoystick* joy)
{
    if(joy->IsInvalid()) return 0;
    return SDL_JoystickGetAttached(_joysticks[joy->GetID()].sdlJoystick) ? 1 : 0;
}

int Joystick_IsGamepad(ScriptJoystick* joy)
{
    return _joysticks[joy->GetID()].sdlGameController != nullptr;
}

const char* Joystick_GetName(ScriptJoystick* joy) {
    if (joy->IsInvalid()) return nullptr;
    auto const& joy_impl = _joysticks[joy->GetID()];
    const char* name_str = SDL_JoystickName(joy_impl.sdlJoystick);
    if (!name_str) return nullptr;
    return CreateNewScriptString(name_str);
}

int Joystick_IsGamepadButtonDown(ScriptJoystick* joy, int butt)
{
    if (joy->IsInvalid()) return 0;
    auto const& joy_impl = _joysticks[joy->GetID()];
    return SDL_GameControllerGetButton(joy_impl.sdlGameController, Gamepad_Button_AGStoSDL(static_cast<eAGSGamepad_Button>(butt)));
}

float Joystick_GetGamepadAxis(ScriptJoystick* joy, int axis, float dead_zone)
{
    if (joy->IsInvalid()) return 0;
    auto const& joy_impl = _joysticks[joy->GetID()];
    int axis_val_int = SDL_GameControllerGetAxis(joy_impl.sdlGameController, Gamepad_Axis_AGStoSDL(static_cast<eAGSGamepad_Axis>(axis)));
    return axis_to_float_with_deadzone(axis_val_int, dead_zone);
}

float Joystick_GetAxis(ScriptJoystick* joy, int axis, float dead_zone)
{
    if (joy->IsInvalid()) return 0;
    auto const& joy_impl = _joysticks[joy->GetID()];
    int axis_count = SDL_JoystickNumAxes(joy_impl.sdlJoystick);
    if (axis < 0 || axis >= axis_count) {
        debug_script_warn("Warning: joystick's (id %d) axis %d is not in range (0:%d), returned 0",
                          joy_impl.sdlJoystickID, axis, axis_count);
        return 0;
    }
    int axis_val_int = SDL_JoystickGetAxis(joy_impl.sdlJoystick, axis);
    return axis_to_float_with_deadzone(axis_val_int, dead_zone);

}

int Joystick_IsButtonDown(ScriptJoystick* joy, int butt)
{
    if (joy->IsInvalid()) return 0;
    auto const& joy_impl = _joysticks[joy->GetID()];
    int button_count = SDL_JoystickNumButtons(joy_impl.sdlJoystick);
    if (butt < 0 || butt >= button_count) {
        debug_script_warn("Warning: joystick's (id %d) button %d is not in range (0:%d), returned false",
                          joy_impl.sdlJoystickID, butt, button_count);
        return 0;
    }
    return SDL_JoystickGetButton(joy_impl.sdlJoystick, butt);
}


int Joystick_GetHat(ScriptJoystick* joy, int hat)
{
    if (joy->IsInvalid()) return eAGSJoystick_Hat::eAGSJoystick_HatCentered;
    auto const& joy_impl = _joysticks[joy->GetID()];
    int hat_count = SDL_JoystickNumHats(joy_impl.sdlJoystick);
    if (hat < 0 || hat >= hat_count) {
        debug_script_warn("Warning: joystick's (id %d) hat %d is not in range (0:%d), returned HatCentered",
                          joy_impl.sdlJoystickID, hat, hat_count);
        return eAGSJoystick_Hat::eAGSJoystick_HatCentered;
    }
    return SDL_JoystickGetHat(joy_impl.sdlJoystick, Joystick_Hat_AGStoSDL(static_cast<eAGSJoystick_Hat>(hat)));
}

int Joystick_GetAxisCount(ScriptJoystick* joy)
{
    if (joy->IsInvalid()) return 0;
    auto const& joy_impl = _joysticks[joy->GetID()];
    return SDL_JoystickNumAxes(joy_impl.sdlJoystick);
}

int Joystick_GetButtonCount(ScriptJoystick* joy)
{
    if (joy->IsInvalid()) return 0;
    auto const& joy_impl = _joysticks[joy->GetID()];
    return SDL_JoystickNumButtons(joy_impl.sdlJoystick);
}

int Joystick_GetHatCount(ScriptJoystick* joy)
{
    if (joy->IsInvalid()) return 0;
    auto const& joy_impl = _joysticks[joy->GetID()];
    return SDL_JoystickNumHats(joy_impl.sdlJoystick);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/dynobj/scriptstring.h"

extern ScriptString myScriptStringImpl;

// int ScriptJoystick::()
RuntimeScriptValue Sc_Joystick_GetJoystickCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Joystick_GetJoystickCount);
}

// ScriptJoystick* ScriptJoystick::(int index)
RuntimeScriptValue Sc_Joystick_GetiJoysticks(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptJoystick, Joystick_GetiJoysticks);
}

// String* (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_GetName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptJoystick, const char, myScriptStringImpl, Joystick_GetName);
}

// int (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_IsConnected(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptJoystick, Joystick_IsConnected);
}

// int (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_IsGamepad(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptJoystick, Joystick_IsGamepad);
}

// int (ScriptJoystick *joy, int button)
RuntimeScriptValue Sc_Joystick_IsGamepadButtonDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptJoystick, Joystick_IsGamepadButtonDown);
}

// float (ScriptJoystick *joy, int axis)
RuntimeScriptValue Sc_Joystick_GetGamepadAxis(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT_PINT_PFLOAT(ScriptJoystick, Joystick_GetGamepadAxis);
}

RuntimeScriptValue Sc_Joystick_GetAxis(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT_PINT_PFLOAT(ScriptJoystick, Joystick_GetAxis);
}

RuntimeScriptValue Sc_Joystick_IsButtonDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptJoystick, Joystick_IsButtonDown);
}

//int (ScriptJoystick *joy, int hat)
RuntimeScriptValue Sc_Joystick_GetHat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptJoystick, Joystick_GetHat);
}

// int (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_GetAxisCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptJoystick, Joystick_GetAxisCount);
}

// int (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_GetButtonCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptJoystick, Joystick_GetButtonCount);
}

// int (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_GetHatCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptJoystick, Joystick_GetHatCount);
}

void RegisterJoystickAPI()
{
    ScFnRegister joystick_api[] = {
            {"Joystick::get_JoystickCount",     API_FN_PAIR(Joystick_GetJoystickCount)},
            {"Joystick::geti_Joysticks",         API_FN_PAIR(Joystick_GetiJoysticks)},
            {"Joystick::get_Name",              API_FN_PAIR(Joystick_GetName)},
            {"Joystick::get_IsConnected",       API_FN_PAIR(Joystick_IsConnected)},
            {"Joystick::get_IsGamepad",         API_FN_PAIR(Joystick_IsGamepad)},
            {"Joystick::IsGamepadButtonDown^1", API_FN_PAIR(Joystick_IsGamepadButtonDown)},
            {"Joystick::GetGamepadAxis^2",      API_FN_PAIR(Joystick_GetGamepadAxis)},
            {"Joystick::GetAxis^2",             API_FN_PAIR(Joystick_GetAxis)},
            {"Joystick::IsButtonDown^1",        API_FN_PAIR(Joystick_IsButtonDown)},
            {"Joystick::GetHat^1",              API_FN_PAIR(Joystick_GetHat)},
            {"Joystick::get_AxisCount",         API_FN_PAIR(Joystick_GetAxisCount)},
            {"Joystick::get_ButtonCount",       API_FN_PAIR(Joystick_GetButtonCount)},
            {"Joystick::get_HatCount",          API_FN_PAIR(Joystick_GetHatCount)}
    };

    ccAddExternalFunctions(joystick_api);
}