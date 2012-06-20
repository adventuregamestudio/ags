
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
	scAdd_External_Symbol("Label::GetText^1", (void *)Label_GetText);
	scAdd_External_Symbol("Label::SetText^1", (void *)Label_SetText);
	scAdd_External_Symbol("Label::get_Font", (void *)Label_GetFont);
	scAdd_External_Symbol("Label::set_Font", (void *)Label_SetFont);
	scAdd_External_Symbol("Label::get_Text", (void *)Label_GetText_New);
	scAdd_External_Symbol("Label::set_Text", (void *)Label_SetText);
	scAdd_External_Symbol("Label::get_TextColor", (void *)Label_GetColor);
	scAdd_External_Symbol("Label::set_TextColor", (void *)Label_SetColor);
}
