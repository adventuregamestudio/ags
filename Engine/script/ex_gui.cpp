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
	ccAddExternalObjectFunction("GUI::Centre^0", (void *)GUI_Centre);
	ccAddExternalStaticFunction("GUI::GetAtScreenXY^2", (void *)GetGUIAtLocation);
	ccAddExternalObjectFunction("GUI::SetPosition^2", (void *)GUI_SetPosition);
	ccAddExternalObjectFunction("GUI::SetSize^2", (void *)GUI_SetSize);
	ccAddExternalObjectFunction("GUI::get_BackgroundGraphic", (void *)GUI_GetBackgroundGraphic);
	ccAddExternalObjectFunction("GUI::set_BackgroundGraphic", (void *)GUI_SetBackgroundGraphic);
	ccAddExternalObjectFunction("GUI::get_Clickable", (void *)GUI_GetClickable);
	ccAddExternalObjectFunction("GUI::set_Clickable", (void *)GUI_SetClickable);
	ccAddExternalObjectFunction("GUI::get_ControlCount", (void *)GUI_GetControlCount);
	ccAddExternalObjectFunction("GUI::geti_Controls", (void *)GUI_GetiControls);
	ccAddExternalObjectFunction("GUI::get_Height", (void *)GUI_GetHeight);
	ccAddExternalObjectFunction("GUI::set_Height", (void *)GUI_SetHeight);
	ccAddExternalObjectFunction("GUI::get_ID", (void *)GUI_GetID);
	ccAddExternalObjectFunction("GUI::get_Transparency", (void *)GUI_GetTransparency);
	ccAddExternalObjectFunction("GUI::set_Transparency", (void *)GUI_SetTransparency);
	ccAddExternalObjectFunction("GUI::get_Visible", (void *)GUI_GetVisible);
	ccAddExternalObjectFunction("GUI::set_Visible", (void *)GUI_SetVisible);
	ccAddExternalObjectFunction("GUI::get_Width", (void *)GUI_GetWidth);
	ccAddExternalObjectFunction("GUI::set_Width", (void *)GUI_SetWidth);
	ccAddExternalObjectFunction("GUI::get_X", (void *)GUI_GetX);
	ccAddExternalObjectFunction("GUI::set_X", (void *)GUI_SetX);
	ccAddExternalObjectFunction("GUI::get_Y", (void *)GUI_GetY);
	ccAddExternalObjectFunction("GUI::set_Y", (void *)GUI_SetY);
	ccAddExternalObjectFunction("GUI::get_ZOrder", (void *)GUI_GetZOrder);
	ccAddExternalObjectFunction("GUI::set_ZOrder", (void *)GUI_SetZOrder);
}
