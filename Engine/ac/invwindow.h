//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__INVWINDOW_H
#define __AGS_EE_AC__INVWINDOW_H

#include "ac/characterinfo.h"
#include "ac/dynobj/scriptinvitem.h"
#include "gui/guiinv.h"

void			InvWindow_SetCharacterToUse(GUIInv *guii, CharacterInfo *chaa);
CharacterInfo*	InvWindow_GetCharacterToUse(GUIInv *guii);
void			InvWindow_SetItemWidth(GUIInv *guii, int newwidth);
int				InvWindow_GetItemWidth(GUIInv *guii);
void			InvWindow_SetItemHeight(GUIInv *guii, int newhit);
int				InvWindow_GetItemHeight(GUIInv *guii);
void			InvWindow_SetTopItem(GUIInv *guii, int topitem);
int				InvWindow_GetTopItem(GUIInv *guii);
int				InvWindow_GetItemsPerRow(GUIInv *guii);
int				InvWindow_GetItemCount(GUIInv *guii);
int				InvWindow_GetRowCount(GUIInv *guii);
void			InvWindow_ScrollDown(GUIInv *guii);
void			InvWindow_ScrollUp(GUIInv *guii);
ScriptInvItem*	InvWindow_GetItemAtIndex(GUIInv *guii, int index);

int				offset_over_inv(GUIInv *inv);

#endif // __AGS_EE_AC__INVWINDOW_H
