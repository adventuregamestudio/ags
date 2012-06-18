#ifndef __AC_GUIINVWINDOW_H
#define __AC_GUIINVWINDOW_H

#include "acgui/ac_guiinv.h"
#include "ac/ac_characterinfo.h"
#include "acrun/ac_scriptobject.h"

int offset_over_inv(GUIInv *inv);

void InvWindow_SetCharacterToUse(GUIInv *guii, CharacterInfo *chaa);
CharacterInfo* InvWindow_GetCharacterToUse(GUIInv *guii);
void InvWindow_SetItemWidth(GUIInv *guii, int newwidth);
int InvWindow_GetItemWidth(GUIInv *guii);
void InvWindow_SetItemHeight(GUIInv *guii, int newhit);
int InvWindow_GetItemHeight(GUIInv *guii);
void InvWindow_SetTopItem(GUIInv *guii, int topitem);
int InvWindow_GetTopItem(GUIInv *guii);
int InvWindow_GetItemsPerRow(GUIInv *guii);
int InvWindow_GetItemCount(GUIInv *guii);
int InvWindow_GetRowCount(GUIInv *guii);
void InvWindow_ScrollDown(GUIInv *guii);
void InvWindow_ScrollUp(GUIInv *guii);
ScriptInvItem* InvWindow_GetItemAtIndex(GUIInv *guii, int index);

#endif // __AC_GUIINVWINDOW_H