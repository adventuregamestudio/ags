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
	ccAddExternalObjectFunction("ViewFrame::get_Flipped", (void *)ViewFrame_GetFlipped);
	ccAddExternalObjectFunction("ViewFrame::get_Frame", (void *)ViewFrame_GetFrame);
	ccAddExternalObjectFunction("ViewFrame::get_Graphic", (void *)ViewFrame_GetGraphic);
	ccAddExternalObjectFunction("ViewFrame::set_Graphic", (void *)ViewFrame_SetGraphic);
	ccAddExternalObjectFunction("ViewFrame::get_LinkedAudio", (void *)ViewFrame_GetLinkedAudio);
	ccAddExternalObjectFunction("ViewFrame::set_LinkedAudio", (void *)ViewFrame_SetLinkedAudio);
	ccAddExternalObjectFunction("ViewFrame::get_Loop", (void *)ViewFrame_GetLoop);
	ccAddExternalObjectFunction("ViewFrame::get_Sound", (void *)ViewFrame_GetSound);
	ccAddExternalObjectFunction("ViewFrame::set_Sound", (void *)ViewFrame_SetSound);
	ccAddExternalObjectFunction("ViewFrame::get_Speed", (void *)ViewFrame_GetSpeed);
	ccAddExternalObjectFunction("ViewFrame::get_View", (void *)ViewFrame_GetView);
}
