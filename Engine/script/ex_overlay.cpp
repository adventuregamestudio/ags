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
// Exporting Overlay script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_overlay_script_functions()
{
	scAdd_External_Symbol("Overlay::CreateGraphical^4", (void *)Overlay_CreateGraphical);
	scAdd_External_Symbol("Overlay::CreateTextual^106", (void *)Overlay_CreateTextual);
	scAdd_External_Symbol("Overlay::SetText^104", (void *)Overlay_SetText);
	scAdd_External_Symbol("Overlay::Remove^0", (void *)Overlay_Remove);
	scAdd_External_Symbol("Overlay::get_Valid", (void *)Overlay_GetValid);
	scAdd_External_Symbol("Overlay::get_X", (void *)Overlay_GetX);
	scAdd_External_Symbol("Overlay::set_X", (void *)Overlay_SetX);
	scAdd_External_Symbol("Overlay::get_Y", (void *)Overlay_GetY);
	scAdd_External_Symbol("Overlay::set_Y", (void *)Overlay_SetY);
}
