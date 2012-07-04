
#include "ac/global_inventoryitem.h"
#include "wgt2allg.h"
#include "ac/ac_common.h"
#include "acmain/ac_maindefines.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_gui.h"
#include "ac/inventoryitem.h"
#include "ac/invwindow.h"
#include "acmain/ac_translation.h"
#include "gui/guimain.h"
#include "gui/guiinv.h"

extern GameSetupStruct game;
extern int guis_need_update;
extern int mousex, mousey;
extern GUIMain*guis;
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;


void set_inv_item_pic(int invi, int piccy) {
    if ((invi < 1) || (invi > game.numinvitems))
        quit("!SetInvItemPic: invalid inventory item specified");

    if (game.invinfo[invi].pic == piccy)
        return;

    if (game.invinfo[invi].pic == game.invinfo[invi].cursorPic)
    {
        // Backwards compatibility -- there didn't used to be a cursorPic,
        // so if they're the same update both.
        set_inv_item_cursorpic(invi, piccy);
    }

    game.invinfo[invi].pic = piccy;
    guis_need_update = 1;
}

void SetInvItemName(int invi, const char *newName) {
    if ((invi < 1) || (invi > game.numinvitems))
        quit("!SetInvName: invalid inventory item specified");

    // set the new name, making sure it doesn't overflow the buffer
    strncpy(game.invinfo[invi].name, newName, 25);
    game.invinfo[invi].name[24] = 0;

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
  if ((indx<0) | (indx>=game.numinvitems)) quit("!GetInvName: invalid inventory item specified");
  strcpy(buff,get_translation(game.invinfo[indx].name));
}

int GetInvGraphic(int indx) {
  if ((indx<0) | (indx>=game.numinvitems)) quit("!GetInvGraphic: invalid inventory item specified");

  return game.invinfo[indx].pic;
}

