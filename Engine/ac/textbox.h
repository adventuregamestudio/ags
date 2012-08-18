
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__TEXTBOX_H
#define __AGS_EE_AC__TEXTBOX_H

#include "gui/guitextbox.h"

const char*	TextBox_GetText_New(GUITextBox *texbox);
void		TextBox_GetText(GUITextBox *texbox, char *buffer);
void		TextBox_SetText(GUITextBox *texbox, const char *newtex);
int			TextBox_GetTextColor(GUITextBox *guit);
void		TextBox_SetTextColor(GUITextBox *guit, int colr);
int			TextBox_GetFont(GUITextBox *guit);
void		TextBox_SetFont(GUITextBox *guit, int fontnum);

#endif // __AGS_EE_AC__TEXTBOX_H
