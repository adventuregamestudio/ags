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
// Exporting Slider script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_slider_script_functions()
{
	scAdd_External_Symbol("Slider::get_BackgroundGraphic", (void *)Slider_GetBackgroundGraphic);
	scAdd_External_Symbol("Slider::set_BackgroundGraphic", (void *)Slider_SetBackgroundGraphic);
	scAdd_External_Symbol("Slider::get_HandleGraphic", (void *)Slider_GetHandleGraphic);
	scAdd_External_Symbol("Slider::set_HandleGraphic", (void *)Slider_SetHandleGraphic);
	scAdd_External_Symbol("Slider::get_HandleOffset", (void *)Slider_GetHandleOffset);
	scAdd_External_Symbol("Slider::set_HandleOffset", (void *)Slider_SetHandleOffset);
	scAdd_External_Symbol("Slider::get_Max", (void *)Slider_GetMax);
	scAdd_External_Symbol("Slider::set_Max", (void *)Slider_SetMax);
	scAdd_External_Symbol("Slider::get_Min", (void *)Slider_GetMin);
	scAdd_External_Symbol("Slider::set_Min", (void *)Slider_SetMin);
	scAdd_External_Symbol("Slider::get_Value", (void *)Slider_GetValue);
	scAdd_External_Symbol("Slider::set_Value", (void *)Slider_SetValue);
}
