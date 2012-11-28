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
// Exporting Label script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_label_script_functions()
{
	ccAddExternalObjectFunction("Label::GetText^1", (void *)Label_GetText);
	ccAddExternalObjectFunction("Label::SetText^1", (void *)Label_SetText);
	ccAddExternalObjectFunction("Label::get_Font", (void *)Label_GetFont);
	ccAddExternalObjectFunction("Label::set_Font", (void *)Label_SetFont);
	ccAddExternalObjectFunction("Label::get_Text", (void *)Label_GetText_New);
	ccAddExternalObjectFunction("Label::set_Text", (void *)Label_SetText);
	ccAddExternalObjectFunction("Label::get_TextColor", (void *)Label_GetColor);
	ccAddExternalObjectFunction("Label::set_TextColor", (void *)Label_SetColor);
}
