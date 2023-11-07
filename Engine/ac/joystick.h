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
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__JOYSTICK_H
#define __AGS_EE_AC__JOYSTICK_H

#include "ac/dynobj/scriptjoystick.h"
#include "SDL_gamecontroller.h"
#include "SDL_events.h"

enum eAGSJoystick_Hat
{
    eAGSJoystick_HatInvalid = -1,
    eAGSJoystick_HatCentered = 0,
    eAGSJoystick_HatUp = 0x01,
    eAGSJoystick_HatRight = 0x02,
    eAGSJoystick_HatDown = 0x04,
    eAGSJoystick_HatLeft = 0x08,
    eAGSJoystick_HatRightUp = 0x03,
    eAGSJoystick_HatRightDown = 0x06,
    eAGSJoystick_HatLeftUp = 0x09,
    eAGSJoystick_HatLeftDown = 0x0C,
};

enum eAGSGamepad_Axis
{
    eAGSGamepad_AxisInvalid,
    eAGSGamepad_AxisLeftX,
    eAGSGamepad_AxisLeftY,
    eAGSGamepad_AxisRightX,
    eAGSGamepad_AxisRightY,
    eAGSGamepad_AxisTriggerLeft,
    eAGSGamepad_AxisTriggerRight,
    eAGSGamepad_AxisCount
};

enum eAGSGamepad_Button
{
    eAGSGamepad_ButtonInvalid,
    eAGSGamepad_ButtonA,
    eAGSGamepad_ButtonB,
    eAGSGamepad_ButtonX,
    eAGSGamepad_ButtonY,
    eAGSGamepad_ButtonBack,
    eAGSGamepad_ButtonGuide,
    eAGSGamepad_ButtonStart,
    eAGSGamepad_ButtonLeftStick,
    eAGSGamepad_ButtonRightStick,
    eAGSGamepad_ButtonLeftShoulder,
    eAGSGamepad_ButtonRightShoulder,
    eAGSGamepad_ButtonDpadUp,
    eAGSGamepad_ButtonDpadDown,
    eAGSGamepad_ButtonDpadLeft,
    eAGSGamepad_ButtonDpadRight,
    eAGSGamepad_ButtonCount
};

enum eAGSGamepad_InputType
{
    eAGSGamepad_InputTypeNone,
    eAGSGamepad_InputTypeAxis,
    eAGSGamepad_InputTypeButton,
    eAGSGamepad_InputTypeHat,
    eAGSGamepad_InputTypeCount,
};

struct GamepadInput
{
    SDL_JoystickID JoystickID = -1;
    eAGSGamepad_Button Button = eAGSGamepad_Button::eAGSGamepad_ButtonInvalid;
    eAGSGamepad_Axis Axis = eAGSGamepad_Axis::eAGSGamepad_AxisInvalid;
    eAGSGamepad_InputType Type = eAGSGamepad_InputType::eAGSGamepad_InputTypeNone;
};

void JoystickConnectionEvent(const SDL_JoyDeviceEvent &event);
bool is_default_gamepad_skip_button_pressed(eAGSGamepad_Button gbn);
void init_joystick();

SDL_GameControllerAxis Gamepad_Axis_AGStoSDL(eAGSGamepad_Axis ags_axis);
SDL_GameControllerButton Gamepad_Button_AGStoSDL(eAGSGamepad_Button ags_button);
eAGSGamepad_Button Gamepad_Button_SDLtoAGS(SDL_GameControllerButton sdl_button);

#endif // __AGS_EE_AC__JOYSTICK_H