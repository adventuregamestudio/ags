
//=============================================================================
//
// Exporting TextBox script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_textbox_script_functions()
{
	scAdd_External_Symbol("TextBox::GetText^1", (void *)TextBox_GetText);
	scAdd_External_Symbol("TextBox::SetText^1", (void *)TextBox_SetText);
	scAdd_External_Symbol("TextBox::get_Font", (void *)TextBox_GetFont);
	scAdd_External_Symbol("TextBox::set_Font", (void *)TextBox_SetFont);
	scAdd_External_Symbol("TextBox::get_Text", (void *)TextBox_GetText_New);
	scAdd_External_Symbol("TextBox::set_Text", (void *)TextBox_SetText);
	scAdd_External_Symbol("TextBox::get_TextColor", (void *)TextBox_GetTextColor);
	scAdd_External_Symbol("TextBox::set_TextColor", (void *)TextBox_SetTextColor);
}
