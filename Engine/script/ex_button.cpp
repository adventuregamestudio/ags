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
// Exporting Button script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_button_script_functions()
{
	ccAddExternalObjectFunction("Button::Animate^4", (void *)Button_Animate);
	ccAddExternalObjectFunction("Button::GetText^1", (void *)Button_GetText);
	ccAddExternalObjectFunction("Button::SetText^1", (void *)Button_SetText);
	ccAddExternalObjectFunction("Button::get_ClipImage", (void *)Button_GetClipImage);
	ccAddExternalObjectFunction("Button::set_ClipImage", (void *)Button_SetClipImage);
	ccAddExternalObjectFunction("Button::get_Font", (void *)Button_GetFont);
	ccAddExternalObjectFunction("Button::set_Font", (void *)Button_SetFont);
	ccAddExternalObjectFunction("Button::get_Graphic", (void *)Button_GetGraphic);
	ccAddExternalObjectFunction("Button::get_MouseOverGraphic", (void *)Button_GetMouseOverGraphic);
	ccAddExternalObjectFunction("Button::set_MouseOverGraphic", (void *)Button_SetMouseOverGraphic);
	ccAddExternalObjectFunction("Button::get_NormalGraphic", (void *)Button_GetNormalGraphic);
	ccAddExternalObjectFunction("Button::set_NormalGraphic", (void *)Button_SetNormalGraphic);
	ccAddExternalObjectFunction("Button::get_PushedGraphic", (void *)Button_GetPushedGraphic);
	ccAddExternalObjectFunction("Button::set_PushedGraphic", (void *)Button_SetPushedGraphic);
	ccAddExternalObjectFunction("Button::get_Text", (void *)Button_GetText_New);
	ccAddExternalObjectFunction("Button::set_Text", (void *)Button_SetText);
	ccAddExternalObjectFunction("Button::get_TextColor", (void *)Button_GetTextColor);
	ccAddExternalObjectFunction("Button::set_TextColor", (void *)Button_SetTextColor);
}
