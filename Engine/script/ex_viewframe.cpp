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
// Exporting ViewFrame script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_viewframe_script_functions()
{
	scAdd_External_Symbol("ViewFrame::get_Flipped", (void *)ViewFrame_GetFlipped);
	scAdd_External_Symbol("ViewFrame::get_Frame", (void *)ViewFrame_GetFrame);
	scAdd_External_Symbol("ViewFrame::get_Graphic", (void *)ViewFrame_GetGraphic);
	scAdd_External_Symbol("ViewFrame::set_Graphic", (void *)ViewFrame_SetGraphic);
	scAdd_External_Symbol("ViewFrame::get_LinkedAudio", (void *)ViewFrame_GetLinkedAudio);
	scAdd_External_Symbol("ViewFrame::set_LinkedAudio", (void *)ViewFrame_SetLinkedAudio);
	scAdd_External_Symbol("ViewFrame::get_Loop", (void *)ViewFrame_GetLoop);
	scAdd_External_Symbol("ViewFrame::get_Sound", (void *)ViewFrame_GetSound);
	scAdd_External_Symbol("ViewFrame::set_Sound", (void *)ViewFrame_SetSound);
	scAdd_External_Symbol("ViewFrame::get_Speed", (void *)ViewFrame_GetSpeed);
	scAdd_External_Symbol("ViewFrame::get_View", (void *)ViewFrame_GetView);
}
