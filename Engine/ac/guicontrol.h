//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GUICONTROL_H
#define __AGS_EE_AC__GUICONTROL_H

#include "gui/guicontrol.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"
#include "ac/dynobj/scriptobjects.h"

using AGS::Common::GUIControl;
using AGS::Common::GUIButton;
using AGS::Common::GUIInvWindow;
using AGS::Common::GUILabel;
using AGS::Common::GUIListBox;
using AGS::Common::GUISlider;
using AGS::Common::GUITextBox;

GUIControl	*GetGUIControlAtLocation(int xx, int yy);
int			GUIControl_GetVisible(GUIControl *guio);
void		GUIControl_SetVisible(GUIControl *guio, int visible);
int			GUIControl_GetClickable(GUIControl *guio);
void		GUIControl_SetClickable(GUIControl *guio, int enabled);
int			GUIControl_GetEnabled(GUIControl *guio);
void		GUIControl_SetEnabled(GUIControl *guio, int enabled);
int			GUIControl_GetID(GUIControl *guio);
ScriptGUI*	GUIControl_GetOwningGUI(GUIControl *guio);
GUIButton*	GUIControl_GetAsButton(GUIControl *guio);
GUIInvWindow* GUIControl_GetAsInvWindow(GUIControl *guio);
GUILabel*	GUIControl_GetAsLabel(GUIControl *guio);
GUIListBox* GUIControl_GetAsListBox(GUIControl *guio);
GUISlider*	GUIControl_GetAsSlider(GUIControl *guio);
GUITextBox* GUIControl_GetAsTextBox(GUIControl *guio);
int			GUIControl_GetX(GUIControl *guio);
void		GUIControl_SetX(GUIControl *guio, int xx);
int			GUIControl_GetY(GUIControl *guio);
void		GUIControl_SetY(GUIControl *guio, int yy);
int         GUIControl_GetZOrder(GUIControl *guio);
void        GUIControl_SetZOrder(GUIControl *guio, int zorder);
void		GUIControl_SetPosition(GUIControl *guio, int xx, int yy);
int			GUIControl_GetWidth(GUIControl *guio);
void		GUIControl_SetWidth(GUIControl *guio, int newwid);
int			GUIControl_GetHeight(GUIControl *guio);
void		GUIControl_SetHeight(GUIControl *guio, int newhit);
void		GUIControl_SetSize(GUIControl *guio, int newwid, int newhit);
void		GUIControl_SendToBack(GUIControl *guio);
void		GUIControl_BringToFront(GUIControl *guio);

#endif // __AGS_EE_AC__GUICONTROL_H
