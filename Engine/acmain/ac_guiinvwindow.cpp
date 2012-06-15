
#include "acmain/ac_maindefines.h"


int offset_over_inv(GUIInv *inv) {

    int mover = mouse_ifacebut_xoffs / multiply_up_coordinate(inv->itemWidth);
    // if it's off the edge of the visible items, ignore
    if (mover >= inv->itemsPerLine)
        return -1;
    mover += (mouse_ifacebut_yoffs / multiply_up_coordinate(inv->itemHeight)) * inv->itemsPerLine;
    if (mover >= inv->itemsPerLine * inv->numLines)
        return -1;

    mover += inv->topIndex;
    if ((mover < 0) || (mover >= charextra[inv->CharToDisplay()].invorder_count))
        return -1;

    return charextra[inv->CharToDisplay()].invorder[mover];
}


int GUIInv::CharToDisplay() {
    if (this->charId < 0)
        return game.playercharacter;

    return this->charId;
}

void GUIInv::Draw() {
    if ((IsDisabled()) && (gui_disabled_style == GUIDIS_BLACKOUT))
        return;

    // backwards compatibility
    play.inv_numinline = this->itemsPerLine;
    play.inv_numdisp = this->numLines * this->itemsPerLine;
    play.obsolete_inv_numorder = charextra[game.playercharacter].invorder_count;
    // if the user changes top_inv_item, switch into backwards
    // compatibiltiy mode
    if (play.inv_top) {
        play.inv_backwards_compatibility = 1;
    }

    if (play.inv_backwards_compatibility) {
        this->topIndex = play.inv_top;
    }

    // draw the items
    int xxx = x;
    int uu, cxp = x, cyp = y;
    int lastItem = this->topIndex + (this->itemsPerLine * this->numLines);
    if (lastItem > charextra[this->CharToDisplay()].invorder_count)
        lastItem = charextra[this->CharToDisplay()].invorder_count;

    for (uu = this->topIndex; uu < lastItem; uu++) {
        // draw inv graphic
        wputblock(cxp, cyp, spriteset[game.invinfo[charextra[this->CharToDisplay()].invorder[uu]].pic], 1);
        cxp += multiply_up_coordinate(this->itemWidth);

        // go to next row when appropriate
        if ((uu - this->topIndex) % this->itemsPerLine == (this->itemsPerLine - 1)) {
            cxp = xxx;
            cyp += multiply_up_coordinate(this->itemHeight);
        }
    }

    if ((IsDisabled()) &&
        (gui_disabled_style == GUIDIS_GREYOUT) && 
        (play.inventory_greys_out == 1)) {
            int col8 = get_col8_lookup(8);
            int jj, kk;   // darken the inventory when disabled
            for (jj = 0; jj < wid; jj++) {
                for (kk = jj % 2; kk < hit; kk += 2)
                    putpixel(abuf, x + jj, y + kk, col8);
            }
    }

}


// *** INV WINDOW FUNCTIONS

void InvWindow_SetCharacterToUse(GUIInv *guii, CharacterInfo *chaa) {
  if (chaa == NULL)
    guii->charId = -1;
  else
    guii->charId = chaa->index_id;
  // reset to top of list
  guii->topIndex = 0;

  guis_need_update = 1;
}

CharacterInfo* InvWindow_GetCharacterToUse(GUIInv *guii) {
  if (guii->charId < 0)
    return NULL;

  return &game.chars[guii->charId];
}



void InvWindow_SetItemWidth(GUIInv *guii, int newwidth) {
  guii->itemWidth = newwidth;
  guii->Resized();
}

int InvWindow_GetItemWidth(GUIInv *guii) {
  return guii->itemWidth;
}

void InvWindow_SetItemHeight(GUIInv *guii, int newhit) {
  guii->itemHeight = newhit;
  guii->Resized();
}

int InvWindow_GetItemHeight(GUIInv *guii) {
  return guii->itemHeight;
}

void InvWindow_SetTopItem(GUIInv *guii, int topitem) {
  if (guii->topIndex != topitem) {
    guii->topIndex = topitem;
    guis_need_update = 1;
  }
}

int InvWindow_GetTopItem(GUIInv *guii) {
  return guii->topIndex;
}

int InvWindow_GetItemsPerRow(GUIInv *guii) {
  return guii->itemsPerLine;
}

int InvWindow_GetItemCount(GUIInv *guii) {
  return charextra[guii->CharToDisplay()].invorder_count;
}

int InvWindow_GetRowCount(GUIInv *guii) {
  return guii->numLines;
}

void InvWindow_ScrollDown(GUIInv *guii) {
  if ((charextra[guii->CharToDisplay()].invorder_count) >
      (guii->topIndex + (guii->itemsPerLine * guii->numLines))) { 
    guii->topIndex += guii->itemsPerLine;
    guis_need_update = 1;
  }
}

void InvWindow_ScrollUp(GUIInv *guii) {
  if (guii->topIndex > 0) {
    guii->topIndex -= guii->itemsPerLine;
    if (guii->topIndex < 0)
      guii->topIndex = 0;

    guis_need_update = 1;
  }
}

ScriptInvItem* InvWindow_GetItemAtIndex(GUIInv *guii, int index) {
  if ((index < 0) || (index >= charextra[guii->CharToDisplay()].invorder_count))
    return NULL;
  return &scrInv[charextra[guii->CharToDisplay()].invorder[index]];
}
