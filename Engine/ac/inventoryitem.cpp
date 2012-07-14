
#include "ac/inventoryitem.h"
#include "util/wgt2allg.h"
#include "ac/characterinfo.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_inventoryitem.h"
#include "ac/global_translation.h"
#include "ac/mouse.h"
#include "ac/properties.h"
#include "ac/rundefines.h"
#include "ac/string.h"


extern GameSetupStruct game;
extern ScriptInvItem scrInv[MAX_INV];
extern int cur_cursor;
extern CharacterInfo*playerchar;


void InventoryItem_SetCursorGraphic(ScriptInvItem *iitem, int newSprite) 
{
    set_inv_item_cursorpic(iitem->id, newSprite);
}

int InventoryItem_GetCursorGraphic(ScriptInvItem *iitem) 
{
    return game.invinfo[iitem->id].cursorPic;
}

void InventoryItem_SetGraphic(ScriptInvItem *iitem, int piccy) {
    set_inv_item_pic(iitem->id, piccy);
}

void InventoryItem_SetName(ScriptInvItem *scii, const char *newname) {
    SetInvItemName(scii->id, newname);
}

int InventoryItem_GetID(ScriptInvItem *scii) {
    return scii->id;
}

ScriptInvItem *GetInvAtLocation(int xx, int yy) {
  int hsnum = GetInvAt(xx, yy);
  if (hsnum <= 0)
    return NULL;
  return &scrInv[hsnum];
}

void InventoryItem_GetName(ScriptInvItem *iitem, char *buff) {
  GetInvName(iitem->id, buff);
}

const char* InventoryItem_GetName_New(ScriptInvItem *invitem) {
  return CreateNewScriptString(get_translation(game.invinfo[invitem->id].name));
}

int InventoryItem_GetGraphic(ScriptInvItem *iitem) {
  return game.invinfo[iitem->id].pic;
}

void InventoryItem_RunInteraction(ScriptInvItem *iitem, int mood) {
    RunInventoryInteraction(iitem->id, mood);
}

int InventoryItem_CheckInteractionAvailable(ScriptInvItem *iitem, int mood) {
  return IsInventoryInteractionAvailable(iitem->id, mood);
}

int InventoryItem_GetProperty(ScriptInvItem *scii, const char *property) {
    return get_int_property (&game.invProps[scii->id], property);
}

void InventoryItem_GetPropertyText(ScriptInvItem *scii, const char *property, char *bufer) {
    get_text_property(&game.invProps[scii->id], property, bufer);
}

const char* InventoryItem_GetTextProperty(ScriptInvItem *scii, const char *property) {
    return get_text_property_dynamic_string(&game.invProps[scii->id], property);
}

//=============================================================================

void set_inv_item_cursorpic(int invItemId, int piccy) 
{
    game.invinfo[invItemId].cursorPic = piccy;

    if ((cur_cursor == MODE_USE) && (playerchar->activeinv == invItemId)) 
    {
        update_inv_cursor(invItemId);
        set_mouse_cursor(cur_cursor);
    }
}
