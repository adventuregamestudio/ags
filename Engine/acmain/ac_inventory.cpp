
#include "acmain/ac_maindefines.h"


void SetActiveInventory(int iit) {

    ScriptInvItem *tosend = NULL;
    if ((iit > 0) && (iit < game.numinvitems))
        tosend = &scrInv[iit];
    else if (iit != -1)
        quitprintf("!SetActiveInventory: invalid inventory number %d", iit);

    Character_SetActiveInventory(playerchar, tosend);
}


void update_invorder() {
    for (int cc = 0; cc < game.numcharacters; cc++) {
        charextra[cc].invorder_count = 0;
        int ff, howmany;
        // Iterate through all inv items, adding them once (or multiple
        // times if requested) to the list.
        for (ff=0;ff < game.numinvitems;ff++) {
            howmany = game.chars[cc].inv[ff];
            if ((game.options[OPT_DUPLICATEINV] == 0) && (howmany > 1))
                howmany = 1;

            for (int ts = 0; ts < howmany; ts++) {
                if (charextra[cc].invorder_count >= MAX_INVORDER)
                    quit("!Too many inventory items to display: 500 max");

                charextra[cc].invorder[charextra[cc].invorder_count] = ff;
                charextra[cc].invorder_count++;
            }
        }
    }
    // backwards compatibility
    play.obsolete_inv_numorder = charextra[game.playercharacter].invorder_count;

    guis_need_update = 1;
}



void add_inventory(int inum) {
    if ((inum < 0) || (inum >= MAX_INV))
        quit("!AddInventory: invalid inventory number");

    Character_AddInventory(playerchar, &scrInv[inum], SCR_NO_VALUE);

    play.obsolete_inv_numorder = charextra[game.playercharacter].invorder_count;
}

void lose_inventory(int inum) {
    if ((inum < 0) || (inum >= MAX_INV))
        quit("!LoseInventory: invalid inventory number");

    Character_LoseInventory(playerchar, &scrInv[inum]);

    play.obsolete_inv_numorder = charextra[game.playercharacter].invorder_count;
}

void AddInventoryToCharacter(int charid, int inum) {
    if (!is_valid_character(charid))
        quit("!AddInventoryToCharacter: invalid character specified");
    if ((inum < 1) || (inum >= game.numinvitems))
        quit("!AddInventory: invalid inv item specified");

    Character_AddInventory(&game.chars[charid], &scrInv[inum], SCR_NO_VALUE);
}

void LoseInventoryFromCharacter(int charid, int inum) {
    if (!is_valid_character(charid))
        quit("!LoseInventoryFromCharacter: invalid character specified");
    if ((inum < 1) || (inum >= game.numinvitems))
        quit("!AddInventory: invalid inv item specified");

    Character_LoseInventory(&game.chars[charid], &scrInv[inum]);
}



#define ICONSPERLINE 4

struct DisplayInvItem {
  int num;
  int sprnum;
  };
int __actual_invscreen() {
  
  int BUTTONAREAHEIGHT = get_fixed_pixel_size(30);
  int cmode=CURS_ARROW, toret = -1;
  int top_item = 0, num_visible_items = 0;
  int MAX_ITEMAREA_HEIGHT = ((scrnhit - BUTTONAREAHEIGHT) - get_fixed_pixel_size(20));
  in_inv_screen++;
  inv_screen_newroom = -1;

start_actinv:
  wsetscreen(virtual_screen);
  
  DisplayInvItem dii[MAX_INV];
  int numitems=0,ww,widest=0,highest=0;
  if (charextra[game.playercharacter].invorder_count < 0)
    update_invorder();
  if (charextra[game.playercharacter].invorder_count == 0) {
    DisplayMessage(996);
    in_inv_screen--;
    return -1;
  }

  if (inv_screen_newroom >= 0) {
    in_inv_screen--;
    NewRoom(inv_screen_newroom);
    return -1;
  }

  for (ww = 0; ww < charextra[game.playercharacter].invorder_count; ww++) {
    if (game.invinfo[charextra[game.playercharacter].invorder[ww]].name[0]!=0) {
      dii[numitems].num = charextra[game.playercharacter].invorder[ww];
      dii[numitems].sprnum = game.invinfo[charextra[game.playercharacter].invorder[ww]].pic;
      int snn=dii[numitems].sprnum;
      if (spritewidth[snn] > widest) widest=spritewidth[snn];
      if (spriteheight[snn] > highest) highest=spriteheight[snn];
      numitems++;
      }
    }
  if (numitems != charextra[game.playercharacter].invorder_count)
    quit("inconsistent inventory calculations");

  widest += get_fixed_pixel_size(4);
  highest += get_fixed_pixel_size(4);
  num_visible_items = (MAX_ITEMAREA_HEIGHT / highest) * ICONSPERLINE;

  int windowhit = highest * (numitems/ICONSPERLINE) + get_fixed_pixel_size(4);
  if ((numitems%ICONSPERLINE) !=0) windowhit+=highest;
  if (windowhit > MAX_ITEMAREA_HEIGHT) {
    windowhit = (MAX_ITEMAREA_HEIGHT / highest) * highest + get_fixed_pixel_size(4);
  }
  windowhit += BUTTONAREAHEIGHT;

  int windowwid = widest*ICONSPERLINE + get_fixed_pixel_size(4);
  if (windowwid < get_fixed_pixel_size(105)) windowwid = get_fixed_pixel_size(105);
  int windowxp=scrnwid/2-windowwid/2;
  int windowyp=scrnhit/2-windowhit/2;
  int buttonyp=windowyp+windowhit-BUTTONAREAHEIGHT;
  wsetcolor(play.sierra_inv_color);
  wbar(windowxp,windowyp,windowxp+windowwid,windowyp+windowhit);
  wsetcolor(0); 
  int bartop = windowyp + get_fixed_pixel_size(2);
  int barxp = windowxp + get_fixed_pixel_size(2);
  wbar(barxp,bartop, windowxp + windowwid - get_fixed_pixel_size(2),buttonyp-1);
  for (ww = top_item; ww < numitems; ww++) {
    if (ww >= top_item + num_visible_items)
      break;
    block spof=spriteset[dii[ww].sprnum];
    wputblock(barxp+1+((ww-top_item)%4)*widest+widest/2-wgetblockwidth(spof)/2,
      bartop+1+((ww-top_item)/4)*highest+highest/2-wgetblockheight(spof)/2,spof,1);
    }
  if ((spriteset[2041] == NULL) || (spriteset[2042] == NULL) || (spriteset[2043] == NULL))
    quit("!InventoryScreen: one or more of the inventory screen graphics have been deleted");
  #define BUTTONWID spritewidth[2042]
  // Draw select, look and OK buttons
  wputblock(windowxp+2, buttonyp + get_fixed_pixel_size(2), spriteset[2041], 1);
  wputblock(windowxp+3+BUTTONWID, buttonyp + get_fixed_pixel_size(2), spriteset[2042], 1);
  wputblock(windowxp+4+BUTTONWID*2, buttonyp + get_fixed_pixel_size(2), spriteset[2043], 1);

  // Draw Up and Down buttons if required
  const int ARROWBUTTONWID = 11;
  block arrowblock = create_bitmap (ARROWBUTTONWID, ARROWBUTTONWID);
  clear_to_color(arrowblock, bitmap_mask_color(arrowblock));
  int usecol;
  __my_setcolor(&usecol, 0);
  if (play.sierra_inv_color == 0)
    __my_setcolor(&usecol, 14);

  line(arrowblock,ARROWBUTTONWID/2, 2, ARROWBUTTONWID-2, 9, usecol);
  line(arrowblock,ARROWBUTTONWID/2, 2, 2, 9, usecol);
  line(arrowblock, 2, 9, ARROWBUTTONWID-2, 9, usecol);
  floodfill(arrowblock, ARROWBUTTONWID/2, 4, usecol);

  if (top_item > 0)
    wputblock(windowxp+windowwid-ARROWBUTTONWID, buttonyp + get_fixed_pixel_size(2), arrowblock, 1);
  if (top_item + num_visible_items < numitems)
    draw_sprite_v_flip (abuf, arrowblock, windowxp+windowwid-ARROWBUTTONWID, buttonyp + get_fixed_pixel_size(4) + ARROWBUTTONWID);
  wfreeblock(arrowblock);

  domouse(1);
  set_mouse_cursor(cmode);
  int wasonitem=-1;
  while (!kbhit()) {
    timerloop = 0;
    NEXT_ITERATION();
    domouse(0);
    update_polled_stuff_and_crossfade();
    write_screen();

    int isonitem=((mousey-bartop)/highest)*ICONSPERLINE+(mousex-barxp)/widest;
    if (mousey<=bartop) isonitem=-1;
    else if (isonitem >= 0) isonitem += top_item;
    if ((isonitem<0) | (isonitem>=numitems) | (isonitem >= top_item + num_visible_items))
      isonitem=-1;

    int mclick = mgetbutton();
    if (mclick == LEFT) {
      if ((mousey<windowyp) | (mousey>windowyp+windowhit) | (mousex<windowxp) | (mousex>windowxp+windowwid))
        continue;
      if (mousey<buttonyp) {
        int clickedon=isonitem;
        if (clickedon<0) continue;
        evblocknum=dii[clickedon].num;
        play.used_inv_on = dii[clickedon].num;

        if (cmode==MODE_LOOK) {
          domouse(2);
          run_event_block_inv(dii[clickedon].num, 0); 
          // in case the script did anything to the screen, redraw it
          mainloop();
          
          goto start_actinv;
          continue;
        }
        else if (cmode==MODE_USE) {
          // use objects on each other
          play.usedinv=toret;

          // set the activeinv so the script can check it
          int activeinvwas = playerchar->activeinv;
          playerchar->activeinv = toret;

          domouse(2);
          run_event_block_inv(dii[clickedon].num, 3);

          // if the script didn't change it, then put it back
          if (playerchar->activeinv == toret)
            playerchar->activeinv = activeinvwas;

          // in case the script did anything to the screen, redraw it
          mainloop();
          
          // They used the active item and lost it
          if (playerchar->inv[toret] < 1) {
            cmode = CURS_ARROW;
            set_mouse_cursor(cmode);
            toret = -1;
          }
 
          goto start_actinv;
//          continue;
          }
        toret=dii[clickedon].num;
//        int plusng=play.using; play.using=toret;
        update_inv_cursor(toret);
        set_mouse_cursor(MODE_USE);
        cmode=MODE_USE;
//        play.using=plusng;
//        break;
        continue;
        }
      else {
        if (mousex >= windowxp+windowwid-ARROWBUTTONWID) {
          if (mousey < buttonyp + get_fixed_pixel_size(2) + ARROWBUTTONWID) {
            if (top_item > 0) {
              top_item -= ICONSPERLINE;
              domouse(2);
              goto start_actinv;
              }
            }
          else if ((mousey < buttonyp + get_fixed_pixel_size(4) + ARROWBUTTONWID*2) && (top_item + num_visible_items < numitems)) {
            top_item += ICONSPERLINE;
            domouse(2);
            goto start_actinv;
            }
          continue;
          }

        int buton=(mousex-windowxp)-2;
        if (buton<0) continue;
        buton/=BUTTONWID;
        if (buton>=3) continue;
        if (buton==0) { toret=-1; cmode=MODE_LOOK; }
        else if (buton==1) { cmode=CURS_ARROW; toret=-1; }
        else break;
        set_mouse_cursor(cmode);
        }
      }
    else if (mclick == RIGHT) {
      if (cmode == CURS_ARROW)
        cmode = MODE_LOOK;
      else
        cmode = CURS_ARROW;
      toret = -1;
      set_mouse_cursor(cmode);
    }
    else if (isonitem!=wasonitem) { domouse(2);
      int rectxp=barxp+1+(wasonitem%4)*widest;
      int rectyp=bartop+1+((wasonitem - top_item)/4)*highest;
      if (wasonitem>=0) {
        wsetcolor(0);
        wrectangle(rectxp,rectyp,rectxp+widest-1,rectyp+highest-1);
        }
      if (isonitem>=0) { wsetcolor(14);//opts.invrectcol);
        rectxp=barxp+1+(isonitem%4)*widest;
        rectyp=bartop+1+((isonitem - top_item)/4)*highest;
        wrectangle(rectxp,rectyp,rectxp+widest-1,rectyp+highest-1);
        }
      domouse(1);
      }
    wasonitem=isonitem;
    while (timerloop == 0) {
      update_polled_stuff_if_runtime();
      platform->YieldCPU();
    }
  }
  while (kbhit()) getch();
  set_default_cursor();
  domouse(2);
  construct_virtual_screen(true);
  in_inv_screen--;
  return toret;
  }

int invscreen() {
  int selt=__actual_invscreen();
  if (selt<0) return -1;
  playerchar->activeinv=selt;
  guis_need_update = 1;
  set_cursor_mode(MODE_USE);
  return selt;
  }

void sc_invscreen() {
  curscript->queue_action(ePSAInvScreen, 0, "InventoryScreen");
}

void SetInvDimensions(int ww,int hh) {
  play.inv_item_wid = ww;
  play.inv_item_hit = hh;
  play.inv_numdisp = 0;
  // backwards compatibility
  for (int i = 0; i < numguiinv; i++) {
    guiinv[i].itemWidth = ww;
    guiinv[i].itemHeight = hh;
    guiinv[i].Resized();
  }
  guis_need_update = 1;
}
