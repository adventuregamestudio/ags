
//=============================================================================
//
// Exporting GuiControl script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_guicontrol_script_functions()
{
	ccAddExternalObjectFunction("GUIControl::BringToFront^0", (void *)GUIControl_BringToFront);
	ccAddExternalStaticFunction("GUIControl::GetAtScreenXY^2", (void *)GetGUIControlAtLocation);
	ccAddExternalObjectFunction("GUIControl::SendToBack^0", (void *)GUIControl_SendToBack);
	ccAddExternalObjectFunction("GUIControl::SetPosition^2", (void *)GUIControl_SetPosition);
	ccAddExternalObjectFunction("GUIControl::SetSize^2", (void *)GUIControl_SetSize);
	ccAddExternalObjectFunction("GUIControl::get_AsButton", (void *)GUIControl_GetAsButton);
	ccAddExternalObjectFunction("GUIControl::get_AsInvWindow", (void *)GUIControl_GetAsInvWindow);
	ccAddExternalObjectFunction("GUIControl::get_AsLabel", (void *)GUIControl_GetAsLabel);
	ccAddExternalObjectFunction("GUIControl::get_AsListBox", (void *)GUIControl_GetAsListBox);
	ccAddExternalObjectFunction("GUIControl::get_AsSlider", (void *)GUIControl_GetAsSlider);
	ccAddExternalObjectFunction("GUIControl::get_AsTextBox", (void *)GUIControl_GetAsTextBox);
	ccAddExternalObjectFunction("GUIControl::get_Clickable", (void *)GUIControl_GetClickable);
	ccAddExternalObjectFunction("GUIControl::set_Clickable", (void *)GUIControl_SetClickable);
	ccAddExternalObjectFunction("GUIControl::get_Enabled", (void *)GUIControl_GetEnabled);
	ccAddExternalObjectFunction("GUIControl::set_Enabled", (void *)GUIControl_SetEnabled);
	ccAddExternalObjectFunction("GUIControl::get_Height", (void *)GUIControl_GetHeight);
	ccAddExternalObjectFunction("GUIControl::set_Height", (void *)GUIControl_SetHeight);
	ccAddExternalObjectFunction("GUIControl::get_ID", (void *)GUIControl_GetID);
	ccAddExternalObjectFunction("GUIControl::get_OwningGUI", (void *)GUIControl_GetOwningGUI);
	ccAddExternalObjectFunction("GUIControl::get_Visible", (void *)GUIControl_GetVisible);
	ccAddExternalObjectFunction("GUIControl::set_Visible", (void *)GUIControl_SetVisible);
	ccAddExternalObjectFunction("GUIControl::get_Width", (void *)GUIControl_GetWidth);
	ccAddExternalObjectFunction("GUIControl::set_Width", (void *)GUIControl_SetWidth);
	ccAddExternalObjectFunction("GUIControl::get_X", (void *)GUIControl_GetX);
	ccAddExternalObjectFunction("GUIControl::set_X", (void *)GUIControl_SetX);
	ccAddExternalObjectFunction("GUIControl::get_Y", (void *)GUIControl_GetY);
	ccAddExternalObjectFunction("GUIControl::set_Y", (void *)GUIControl_SetY);
}
