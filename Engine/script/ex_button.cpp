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
	scAdd_External_Symbol("Button::Animate^4", (void *)Button_Animate);
	scAdd_External_Symbol("Button::GetText^1", (void *)Button_GetText);
	scAdd_External_Symbol("Button::SetText^1", (void *)Button_SetText);
	scAdd_External_Symbol("Button::get_ClipImage", (void *)Button_GetClipImage);
	scAdd_External_Symbol("Button::set_ClipImage", (void *)Button_SetClipImage);
	scAdd_External_Symbol("Button::get_Font", (void *)Button_GetFont);
	scAdd_External_Symbol("Button::set_Font", (void *)Button_SetFont);
	scAdd_External_Symbol("Button::get_Graphic", (void *)Button_GetGraphic);
	scAdd_External_Symbol("Button::get_MouseOverGraphic", (void *)Button_GetMouseOverGraphic);
	scAdd_External_Symbol("Button::set_MouseOverGraphic", (void *)Button_SetMouseOverGraphic);
	scAdd_External_Symbol("Button::get_NormalGraphic", (void *)Button_GetNormalGraphic);
	scAdd_External_Symbol("Button::set_NormalGraphic", (void *)Button_SetNormalGraphic);
	scAdd_External_Symbol("Button::get_PushedGraphic", (void *)Button_GetPushedGraphic);
	scAdd_External_Symbol("Button::set_PushedGraphic", (void *)Button_SetPushedGraphic);
	scAdd_External_Symbol("Button::get_Text", (void *)Button_GetText_New);
	scAdd_External_Symbol("Button::set_Text", (void *)Button_SetText);
	scAdd_External_Symbol("Button::get_TextColor", (void *)Button_GetTextColor);
	scAdd_External_Symbol("Button::set_TextColor", (void *)Button_SetTextColor);
}
