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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GUICONTROL_H
#define __AGS_EE_AC__GUICONTROL_H

#include "gui/guiobject.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"
#include "ac/dynobj/scriptgui.h"

using AGS::Common::GuiButton;
using AGS::Common::GuiInvWindow;
using AGS::Common::GuiLabel;
using AGS::Common::GuiListBox;
using AGS::Common::GuiObject;
using AGS::Common::GuiSlider;
using AGS::Common::GuiTextBox;

GuiObject	*GetGUIControlAtLocation(int xx, int yy);
int			GUIControl_GetVisible(GuiObject *guio);
void		GUIControl_SetVisible(GuiObject *guio, int visible);
int			GUIControl_GetClickable(GuiObject *guio);
void		GUIControl_SetClickable(GuiObject *guio, int enabled);
int			GUIControl_GetEnabled(GuiObject *guio);
void		GUIControl_SetEnabled(GuiObject *guio, int enabled);
int			GUIControl_GetID(GuiObject *guio);
ScriptGUI*	GUIControl_GetOwningGUI(GuiObject *guio);
GuiButton*	GUIControl_GetAsButton(GuiObject *guio);
GuiInvWindow*		GUIControl_GetAsInvWindow(GuiObject *guio);
GuiLabel*	GUIControl_GetAsLabel(GuiObject *guio);
GuiListBox* GUIControl_GetAsListBox(GuiObject *guio);
GuiSlider*	GUIControl_GetAsSlider(GuiObject *guio);
GuiTextBox* GUIControl_GetAsTextBox(GuiObject *guio);
int			GUIControl_GetX(GuiObject *guio);
void		GUIControl_SetX(GuiObject *guio, int xx);
int			GUIControl_GetY(GuiObject *guio);
void		GUIControl_SetY(GuiObject *guio, int yy);
void		GUIControl_SetPosition(GuiObject *guio, int xx, int yy);
int			GUIControl_GetWidth(GuiObject *guio);
void		GUIControl_SetWidth(GuiObject *guio, int newwid);
int			GUIControl_GetHeight(GuiObject *guio);
void		GUIControl_SetHeight(GuiObject *guio, int newhit);
void		GUIControl_SetSize(GuiObject *guio, int newwid, int newhit);
void		GUIControl_SendToBack(GuiObject *guio);
void		GUIControl_BringToFront(GuiObject *guio);

#endif // __AGS_EE_AC__GUICONTROL_H
