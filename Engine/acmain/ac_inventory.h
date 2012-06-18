#ifndef __AC_INVENTORY_H
#define __AC_INVENTORY_H

void SetActiveInventory(int iit);
void update_invorder();
void add_inventory(int inum);
void lose_inventory(int inum);
void AddInventoryToCharacter(int charid, int inum);
void LoseInventoryFromCharacter(int charid, int inum);

int __actual_invscreen();
int invscreen();
void sc_invscreen();
void SetInvDimensions(int ww,int hh);

extern int in_inv_screen, inv_screen_newroom;

#endif // __AC_INVENTORY_H
