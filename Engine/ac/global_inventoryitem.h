
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALINVENTORYITEM_H
#define __AGS_EE_AC__GLOBALINVENTORYITEM_H

void set_inv_item_pic(int invi, int piccy);
void SetInvItemName(int invi, const char *newName);
int  GetInvAt (int xxx, int yyy);
void GetInvName(int indx,char*buff);
int  GetInvGraphic(int indx);
void RunInventoryInteraction (int iit, int modd);
int  IsInventoryInteractionAvailable (int item, int mood);

#endif // __AGS_EE_AC__GLOBALINVENTORYITEM_H
