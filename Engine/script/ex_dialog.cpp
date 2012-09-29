
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
	ccAddExternalObjectFunction("Dialog::get_ID", (void *)Dialog_GetID);
	ccAddExternalObjectFunction("Dialog::get_OptionCount", (void *)Dialog_GetOptionCount);
	ccAddExternalObjectFunction("Dialog::get_ShowTextParser", (void *)Dialog_GetShowTextParser);
	ccAddExternalObjectFunction("Dialog::DisplayOptions^1", (void *)Dialog_DisplayOptions);
	ccAddExternalObjectFunction("Dialog::GetOptionState^1", (void *)Dialog_GetOptionState);
	ccAddExternalObjectFunction("Dialog::GetOptionText^1", (void *)Dialog_GetOptionText);
	ccAddExternalObjectFunction("Dialog::HasOptionBeenChosen^1", (void *)Dialog_HasOptionBeenChosen);
	ccAddExternalObjectFunction("Dialog::SetOptionState^2", (void *)Dialog_SetOptionState);
	ccAddExternalObjectFunction("Dialog::Start^0", (void *)Dialog_Start);

	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_ActiveOptionID", (void *)DialogOptionsRendering_GetActiveOptionID);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_ActiveOptionID", (void *)DialogOptionsRendering_SetActiveOptionID);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_DialogToRender", (void *)DialogOptionsRendering_GetDialogToRender);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_Height", (void *)DialogOptionsRendering_GetHeight);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_Height", (void *)DialogOptionsRendering_SetHeight);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_ParserTextBoxX", (void *)DialogOptionsRendering_GetParserTextboxX);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_ParserTextBoxX", (void *)DialogOptionsRendering_SetParserTextboxX);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_ParserTextBoxY", (void *)DialogOptionsRendering_GetParserTextboxY);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_ParserTextBoxY", (void *)DialogOptionsRendering_SetParserTextboxY);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_ParserTextBoxWidth", (void *)DialogOptionsRendering_GetParserTextboxWidth);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_ParserTextBoxWidth", (void *)DialogOptionsRendering_SetParserTextboxWidth);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_Surface", (void *)DialogOptionsRendering_GetSurface);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_Width", (void *)DialogOptionsRendering_GetWidth);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_Width", (void *)DialogOptionsRendering_SetWidth);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_X", (void *)DialogOptionsRendering_GetX);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_X", (void *)DialogOptionsRendering_SetX);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_Y", (void *)DialogOptionsRendering_GetY);
	ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_Y", (void *)DialogOptionsRendering_SetY);
}
