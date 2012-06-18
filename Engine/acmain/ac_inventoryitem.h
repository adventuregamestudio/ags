#ifndef __AC_INVENTORYITEM_H
#define __AC_INVENTORYITEM_H

#include "acrun/ac_scriptobject.h"

void set_inv_item_cursorpic(int invItemId, int piccy);
void InventoryItem_SetCursorGraphic(ScriptInvItem *iitem, int newSprite);
int InventoryItem_GetCursorGraphic(ScriptInvItem *iitem);
void set_inv_item_pic(int invi, int piccy);
void InventoryItem_SetGraphic(ScriptInvItem *iitem, int piccy);
void SetInvItemName(int invi, const char *newName);
void InventoryItem_SetName(ScriptInvItem *scii, const char *newname);
int InventoryItem_GetID(ScriptInvItem *scii);
int GetInvAt (int xxx, int yyy);
ScriptInvItem *GetInvAtLocation(int xx, int yy);
void GetInvName(int indx,char*buff);
void InventoryItem_GetName(ScriptInvItem *iitem, char *buff);
const char* InventoryItem_GetName_New(ScriptInvItem *invitem);
int GetInvGraphic(int indx);
int InventoryItem_GetGraphic(ScriptInvItem *iitem);

#endif // __AC_INVENTORYITEM_H