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

#include "ac/common.h"
#include "ac/global_gui.h"
#include "ac/global_inventoryitem.h"
#include "ac/global_translation.h"
#include "ac/inventoryitem.h"
#include "ac/invwindow.h"
#include "ac/properties.h"
#include "ac/string.h"
#include "gui/guimain.h"
#include "gui/guiinv.h"
#include "ac/event.h"
#include "game/game_objects.h"

extern int guis_need_update;
extern int mousex, mousey;
extern GUIMain*guis;
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;
extern char*evblockbasename;
extern int evblocknum;
extern CharacterInfo*playerchar;


void set_inv_item_pic(int invi, int piccy) {
    if ((invi < 1) || (invi > game.InvItemCount))
        quit("!SetInvItemPic: invalid inventory item specified");

    if (game.InventoryItems[invi].pic == piccy)
        return;

    if (game.InventoryItems[invi].pic == game.InventoryItems[invi].cursorPic)
    {
        // Backwards compatibility -- there didn't used to be a cursorPic,
        // so if they're the same update both.
        set_inv_item_cursorpic(invi, piccy);
    }

    game.InventoryItems[invi].pic = piccy;
    guis_need_update = 1;
}

void SetInvItemName(int invi, const char *newName) {
    if ((invi < 1) || (invi > game.InvItemCount))
        quit("!SetInvName: invalid inventory item specified");

    // set the new name, making sure it doesn't overflow the buffer
    strncpy(game.InventoryItems[invi].name, newName, 25);
    game.InventoryItems[invi].name[24] = 0;

    // might need to redraw the GUI if it has the inv item name on it
    guis_need_update = 1;
}

int GetInvAt (int xxx, int yyy) {
  int ongui = GetGUIAt (xxx, yyy);
  if (ongui >= 0) {
    int mxwas = mousex, mywas = mousey;
    mousex = multiply_up_coordinate(xxx) - guis[ongui].x;
    mousey = multiply_up_coordinate(yyy) - guis[ongui].y;
    int onobj = guis[ongui].find_object_under_mouse();
    if (onobj>=0) {
      mouse_ifacebut_xoffs = mousex-(guis[ongui].objs[onobj]->x);
      mouse_ifacebut_yoffs = mousey-(guis[ongui].objs[onobj]->y);
    }
    mousex = mxwas;
    mousey = mywas;
    if ((onobj>=0) && ((guis[ongui].objrefptr[onobj] >> 16)==GOBJ_INVENTORY))
      return offset_over_inv((GUIInv*)guis[ongui].objs[onobj]);
  }
  return -1;
}

void GetInvName(int indx,char*buff) {
  VALIDATE_STRING(buff);
  if ((indx<0) | (indx>=game.InvItemCount)) quit("!GetInvName: invalid inventory item specified");
  strcpy(buff,get_translation(game.InventoryItems[indx].name));
}

int GetInvGraphic(int indx) {
  if ((indx<0) | (indx>=game.InvItemCount)) quit("!GetInvGraphic: invalid inventory item specified");

  return game.InventoryItems[indx].pic;
}

void RunInventoryInteraction (int iit, int modd) {
    if ((iit < 0) || (iit >= game.InvItemCount))
        quit("!RunInventoryInteraction: invalid inventory number");

    evblocknum = iit;
    if (modd == MODE_LOOK)
        run_event_block_inv(iit, 0);
    else if (modd == MODE_HAND)
        run_event_block_inv(iit, 1);
    else if (modd == MODE_USE) {
        play.UsedInvItemIndex = playerchar->activeinv;
        run_event_block_inv(iit, 3);
    }
    else if (modd == MODE_TALK)
        run_event_block_inv(iit, 2);
    else // other click on invnetory
        run_event_block_inv(iit, 4);
}

int IsInventoryInteractionAvailable (int item, int mood) {
  if ((item < 0) || (item >= MAX_INV))
    quit("!IsInventoryInteractionAvailable: invalid inventory number");

  play.TestInteractionMode = 1;

  RunInventoryInteraction(item, mood);

  int ciwas = play.TestInteractionMode;
  play.TestInteractionMode = 0;

  if (ciwas == 2)
    return 1;

  return 0;
}

int GetInvProperty (int item, const char *property) {
    return get_int_property (&game.InvItemProperties[item], property);
}

void GetInvPropertyText (int item, const char *property, char *bufer) {
    get_text_property (&game.InvItemProperties[item], property, bufer);
}
