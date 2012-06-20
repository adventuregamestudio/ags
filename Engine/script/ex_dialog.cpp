
//=============================================================================
//
// Exporting Dialog script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_dialog_script_functions()
{
	scAdd_External_Symbol("Dialog::get_ID", (void *)Dialog_GetID);
	scAdd_External_Symbol("Dialog::get_OptionCount", (void *)Dialog_GetOptionCount);
	scAdd_External_Symbol("Dialog::get_ShowTextParser", (void *)Dialog_GetShowTextParser);
	scAdd_External_Symbol("Dialog::DisplayOptions^1", (void *)Dialog_DisplayOptions);
	scAdd_External_Symbol("Dialog::GetOptionState^1", (void *)Dialog_GetOptionState);
	scAdd_External_Symbol("Dialog::GetOptionText^1", (void *)Dialog_GetOptionText);
	scAdd_External_Symbol("Dialog::HasOptionBeenChosen^1", (void *)Dialog_HasOptionBeenChosen);
	scAdd_External_Symbol("Dialog::SetOptionState^2", (void *)Dialog_SetOptionState);
	scAdd_External_Symbol("Dialog::Start^0", (void *)Dialog_Start);

	scAdd_External_Symbol("DialogOptionsRenderingInfo::get_ActiveOptionID", (void *)DialogOptionsRendering_GetActiveOptionID);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::set_ActiveOptionID", (void *)DialogOptionsRendering_SetActiveOptionID);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::get_DialogToRender", (void *)DialogOptionsRendering_GetDialogToRender);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::get_Height", (void *)DialogOptionsRendering_GetHeight);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::set_Height", (void *)DialogOptionsRendering_SetHeight);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::get_ParserTextBoxX", (void *)DialogOptionsRendering_GetParserTextboxX);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::set_ParserTextBoxX", (void *)DialogOptionsRendering_SetParserTextboxX);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::get_ParserTextBoxY", (void *)DialogOptionsRendering_GetParserTextboxY);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::set_ParserTextBoxY", (void *)DialogOptionsRendering_SetParserTextboxY);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::get_ParserTextBoxWidth", (void *)DialogOptionsRendering_GetParserTextboxWidth);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::set_ParserTextBoxWidth", (void *)DialogOptionsRendering_SetParserTextboxWidth);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::get_Surface", (void *)DialogOptionsRendering_GetSurface);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::get_Width", (void *)DialogOptionsRendering_GetWidth);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::set_Width", (void *)DialogOptionsRendering_SetWidth);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::get_X", (void *)DialogOptionsRendering_GetX);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::set_X", (void *)DialogOptionsRendering_SetX);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::get_Y", (void *)DialogOptionsRendering_GetY);
	scAdd_External_Symbol("DialogOptionsRenderingInfo::set_Y", (void *)DialogOptionsRendering_SetY);
}
