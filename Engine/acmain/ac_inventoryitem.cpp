
#include "acmain/ac_maindefines.h"



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

ScriptInvItem *GetInvAtLocation(int xx, int yy) {
  int hsnum = GetInvAt(xx, yy);
  if (hsnum <= 0)
    return NULL;
  return &scrInv[hsnum];
}


void GetInvName(int indx,char*buff) {
  VALIDATE_STRING(buff);
  if ((indx<0) | (indx>=game.numinvitems)) quit("!GetInvName: invalid inventory item specified");
  strcpy(buff,get_translation(game.invinfo[indx].name));
}

void InventoryItem_GetName(ScriptInvItem *iitem, char *buff) {
  GetInvName(iitem->id, buff);
}

const char* InventoryItem_GetName_New(ScriptInvItem *invitem) {
  return CreateNewScriptString(get_translation(game.invinfo[invitem->id].name));
}

int GetInvGraphic(int indx) {
  if ((indx<0) | (indx>=game.numinvitems)) quit("!GetInvGraphic: invalid inventory item specified");

  return game.invinfo[indx].pic;
}

int InventoryItem_GetGraphic(ScriptInvItem *iitem) {
  return game.invinfo[iitem->id].pic;
}
