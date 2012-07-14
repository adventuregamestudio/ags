
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__BUTTON_H
#define __AGS_EE_AC__BUTTON_H

#include "gui/guibutton.h"

void		Button_Animate(GUIButton *butt, int view, int loop, int speed, int repeat);
const char* Button_GetText_New(GUIButton *butt);
void		Button_GetText(GUIButton *butt, char *buffer);
void		Button_SetText(GUIButton *butt, const char *newtx);
void		Button_SetFont(GUIButton *butt, int newFont);
int			Button_GetFont(GUIButton *butt);
int			Button_GetClipImage(GUIButton *butt);
void		Button_SetClipImage(GUIButton *butt, int newval);
int			Button_GetGraphic(GUIButton *butt);
int			Button_GetMouseOverGraphic(GUIButton *butt);
void		Button_SetMouseOverGraphic(GUIButton *guil, int slotn);
int			Button_GetNormalGraphic(GUIButton *butt);
void		Button_SetNormalGraphic(GUIButton *guil, int slotn);
int			Button_GetPushedGraphic(GUIButton *butt);
void		Button_SetPushedGraphic(GUIButton *guil, int slotn);
int			Button_GetTextColor(GUIButton *butt);
void		Button_SetTextColor(GUIButton *butt, int newcol);

int			UpdateAnimatingButton(int bu);
void		StopButtonAnimation(int idxn);
void		FindAndRemoveButtonAnimation(int guin, int objn);

#endif // __AGS_EE_AC__BUTTON_H
