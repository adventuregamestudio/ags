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
#ifndef __AGS_EE_AC__BUTTON_H
#define __AGS_EE_AC__BUTTON_H

#include "gui/animatingguibutton.h"
#include "gui/guibutton.h"

using AGS::Common::GuiButton;

void		Button_Animate(GuiButton *butt, int view, int loop, int speed, int repeat);
const char* Button_GetText_New(GuiButton *butt);
void		Button_GetText(GuiButton *butt, char *buffer);
void		Button_SetText(GuiButton *butt, const char *newtx);
void		Button_SetFont(GuiButton *butt, int newFont);
int			Button_GetFont(GuiButton *butt);
int			Button_GetClipImage(GuiButton *butt);
void		Button_SetClipImage(GuiButton *butt, int newval);
int			Button_GetGraphic(GuiButton *butt);
int			Button_GetMouseOverGraphic(GuiButton *butt);
void		Button_SetMouseOverGraphic(GuiButton *guil, int slotn);
int			Button_GetNormalGraphic(GuiButton *butt);
void		Button_SetNormalGraphic(GuiButton *guil, int slotn);
int			Button_GetPushedGraphic(GuiButton *butt);
void		Button_SetPushedGraphic(GuiButton *guil, int slotn);
int			Button_GetTextColor(GuiButton *butt);
void		Button_SetTextColor(GuiButton *butt, int newcol);

int			UpdateAnimatingButton(int bu);
void		StopButtonAnimation(int idxn);
void		FindAndRemoveButtonAnimation(int guin, int objn);

extern AnimatingGUIButton animbuts[MAX_ANIMATING_BUTTONS];
extern int numAnimButs;

#endif // __AGS_EE_AC__BUTTON_H
