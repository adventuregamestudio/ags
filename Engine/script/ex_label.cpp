
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
