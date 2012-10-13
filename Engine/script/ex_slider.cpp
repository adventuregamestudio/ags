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
	ccAddExternalObjectFunction("Slider::get_BackgroundGraphic", (void *)Slider_GetBackgroundGraphic);
	ccAddExternalObjectFunction("Slider::set_BackgroundGraphic", (void *)Slider_SetBackgroundGraphic);
	ccAddExternalObjectFunction("Slider::get_HandleGraphic", (void *)Slider_GetHandleGraphic);
	ccAddExternalObjectFunction("Slider::set_HandleGraphic", (void *)Slider_SetHandleGraphic);
	ccAddExternalObjectFunction("Slider::get_HandleOffset", (void *)Slider_GetHandleOffset);
	ccAddExternalObjectFunction("Slider::set_HandleOffset", (void *)Slider_SetHandleOffset);
	ccAddExternalObjectFunction("Slider::get_Max", (void *)Slider_GetMax);
	ccAddExternalObjectFunction("Slider::set_Max", (void *)Slider_SetMax);
	ccAddExternalObjectFunction("Slider::get_Min", (void *)Slider_GetMin);
	ccAddExternalObjectFunction("Slider::set_Min", (void *)Slider_SetMin);
	ccAddExternalObjectFunction("Slider::get_Value", (void *)Slider_GetValue);
	ccAddExternalObjectFunction("Slider::set_Value", (void *)Slider_SetValue);
}
