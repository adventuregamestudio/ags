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
#ifndef __AGS_EE_AC__LISTBOX_H
#define __AGS_EE_AC__LISTBOX_H

#include "gui/guilistbox.h"

using AGS::Common::GuiListBox;

int			ListBox_AddItem(GuiListBox *lbb, const char *text);
int			ListBox_InsertItemAt(GuiListBox *lbb, int index, const char *text);
void		ListBox_Clear(GuiListBox *listbox);
void		ListBox_FillDirList(GuiListBox *listbox, const char *filemask);
int			ListBox_GetSaveGameSlots(GuiListBox *listbox, int index);
int			ListBox_FillSaveGameList(GuiListBox *listbox);
int			ListBox_GetItemAtLocation(GuiListBox *listbox, int x, int y);
char		*ListBox_GetItemText(GuiListBox *listbox, int index, char *buffer);
const char* ListBox_GetItems(GuiListBox *listbox, int index);
void		ListBox_SetItemText(GuiListBox *listbox, int index, const char *newtext);
void		ListBox_RemoveItem(GuiListBox *listbox, int itemIndex);
int			ListBox_GetItemCount(GuiListBox *listbox);
int			ListBox_GetFont(GuiListBox *listbox);
void		ListBox_SetFont(GuiListBox *listbox, int newfont);
int			ListBox_GetHideBorder(GuiListBox *listbox);
void		ListBox_SetHideBorder(GuiListBox *listbox, int newValue);
int			ListBox_GetHideScrollArrows(GuiListBox *listbox);
void		ListBox_SetHideScrollArrows(GuiListBox *listbox, int newValue);
int			ListBox_GetSelectedIndex(GuiListBox *listbox);
void		ListBox_SetSelectedIndex(GuiListBox *guisl, int newsel);
int			ListBox_GetTopItem(GuiListBox *listbox);
void		ListBox_SetTopItem(GuiListBox *guisl, int item);
int			ListBox_GetRowCount(GuiListBox *listbox);
void		ListBox_ScrollDown(GuiListBox *listbox);
void		ListBox_ScrollUp(GuiListBox *listbox);

GuiListBox* is_valid_listbox (int guin, int objn);

#endif // __AGS_EE_AC__LISTBOX_H
