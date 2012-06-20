
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
	scAdd_External_Symbol("GUIControl::BringToFront^0", (void *)GUIControl_BringToFront);
	scAdd_External_Symbol("GUIControl::GetAtScreenXY^2", (void *)GetGUIControlAtLocation);
	scAdd_External_Symbol("GUIControl::SendToBack^0", (void *)GUIControl_SendToBack);
	scAdd_External_Symbol("GUIControl::SetPosition^2", (void *)GUIControl_SetPosition);
	scAdd_External_Symbol("GUIControl::SetSize^2", (void *)GUIControl_SetSize);
	scAdd_External_Symbol("GUIControl::get_AsButton", (void *)GUIControl_GetAsButton);
	scAdd_External_Symbol("GUIControl::get_AsInvWindow", (void *)GUIControl_GetAsInvWindow);
	scAdd_External_Symbol("GUIControl::get_AsLabel", (void *)GUIControl_GetAsLabel);
	scAdd_External_Symbol("GUIControl::get_AsListBox", (void *)GUIControl_GetAsListBox);
	scAdd_External_Symbol("GUIControl::get_AsSlider", (void *)GUIControl_GetAsSlider);
	scAdd_External_Symbol("GUIControl::get_AsTextBox", (void *)GUIControl_GetAsTextBox);
	scAdd_External_Symbol("GUIControl::get_Clickable", (void *)GUIControl_GetClickable);
	scAdd_External_Symbol("GUIControl::set_Clickable", (void *)GUIControl_SetClickable);
	scAdd_External_Symbol("GUIControl::get_Enabled", (void *)GUIControl_GetEnabled);
	scAdd_External_Symbol("GUIControl::set_Enabled", (void *)GUIControl_SetEnabled);
	scAdd_External_Symbol("GUIControl::get_Height", (void *)GUIControl_GetHeight);
	scAdd_External_Symbol("GUIControl::set_Height", (void *)GUIControl_SetHeight);
	scAdd_External_Symbol("GUIControl::get_ID", (void *)GUIControl_GetID);
	scAdd_External_Symbol("GUIControl::get_OwningGUI", (void *)GUIControl_GetOwningGUI);
	scAdd_External_Symbol("GUIControl::get_Visible", (void *)GUIControl_GetVisible);
	scAdd_External_Symbol("GUIControl::set_Visible", (void *)GUIControl_SetVisible);
	scAdd_External_Symbol("GUIControl::get_Width", (void *)GUIControl_GetWidth);
	scAdd_External_Symbol("GUIControl::set_Width", (void *)GUIControl_SetWidth);
	scAdd_External_Symbol("GUIControl::get_X", (void *)GUIControl_GetX);
	scAdd_External_Symbol("GUIControl::set_X", (void *)GUIControl_SetX);
	scAdd_External_Symbol("GUIControl::get_Y", (void *)GUIControl_GetY);
	scAdd_External_Symbol("GUIControl::set_Y", (void *)GUIControl_SetY);
}
