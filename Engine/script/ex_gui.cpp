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
// Exporting Gui script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_gui_script_functions()
{
	scAdd_External_Symbol("GUI::Centre^0", (void *)GUI_Centre);
	scAdd_External_Symbol("GUI::GetAtScreenXY^2", (void *)GetGUIAtLocation);
	scAdd_External_Symbol("GUI::SetPosition^2", (void *)GUI_SetPosition);
	scAdd_External_Symbol("GUI::SetSize^2", (void *)GUI_SetSize);
	scAdd_External_Symbol("GUI::get_BackgroundGraphic", (void *)GUI_GetBackgroundGraphic);
	scAdd_External_Symbol("GUI::set_BackgroundGraphic", (void *)GUI_SetBackgroundGraphic);
	scAdd_External_Symbol("GUI::get_Clickable", (void *)GUI_GetClickable);
	scAdd_External_Symbol("GUI::set_Clickable", (void *)GUI_SetClickable);
	scAdd_External_Symbol("GUI::get_ControlCount", (void *)GUI_GetControlCount);
	scAdd_External_Symbol("GUI::geti_Controls", (void *)GUI_GetiControls);
	scAdd_External_Symbol("GUI::get_Height", (void *)GUI_GetHeight);
	scAdd_External_Symbol("GUI::set_Height", (void *)GUI_SetHeight);
	scAdd_External_Symbol("GUI::get_ID", (void *)GUI_GetID);
	scAdd_External_Symbol("GUI::get_Transparency", (void *)GUI_GetTransparency);
	scAdd_External_Symbol("GUI::set_Transparency", (void *)GUI_SetTransparency);
	scAdd_External_Symbol("GUI::get_Visible", (void *)GUI_GetVisible);
	scAdd_External_Symbol("GUI::set_Visible", (void *)GUI_SetVisible);
	scAdd_External_Symbol("GUI::get_Width", (void *)GUI_GetWidth);
	scAdd_External_Symbol("GUI::set_Width", (void *)GUI_SetWidth);
	scAdd_External_Symbol("GUI::get_X", (void *)GUI_GetX);
	scAdd_External_Symbol("GUI::set_X", (void *)GUI_SetX);
	scAdd_External_Symbol("GUI::get_Y", (void *)GUI_GetY);
	scAdd_External_Symbol("GUI::set_Y", (void *)GUI_SetY);
	scAdd_External_Symbol("GUI::get_ZOrder", (void *)GUI_GetZOrder);
	scAdd_External_Symbol("GUI::set_ZOrder", (void *)GUI_SetZOrder);
}
