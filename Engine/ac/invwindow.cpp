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

#include "ac/invwindow.h"
#include "ac/common.h"
#include "ac/characterextras.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_character.h"
#include "ac/global_display.h"
#include "ac/global_room.h"
#include "ac/mouse.h"
#include "ac/record.h"
#include "main/game_run.h"
#include "media/audio/audio.h"
#include "platform/base/agsplatformdriver.h"
#include "ac/spritecache.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_character.h"
#include "ac/dynobj/cc_inventory.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

extern int guis_need_update;
extern GameSetupStruct game;
extern GameState play;
extern CharacterExtras *charextra;
extern ScriptInvItem scrInv[MAX_INV];
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;
extern int scrnwid,scrnhit;
extern Bitmap *virtual_screen;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern int mousex,mousey;
extern volatile int timerloop;
extern int evblocknum;
extern CharacterInfo*playerchar;
extern AGSPlatformDriver *platform;
extern CCCharacter ccDynamicCharacter;
extern CCInventory ccDynamicInv;

int in_inv_screen = 0, inv_screen_newroom = -1;

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

//=============================================================================

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
    Bitmap *ds = NULL;

start_actinv:
    ds = SetVirtualScreen(virtual_screen);

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
    color_t draw_color = ds->GetCompatibleColor(play.sierra_inv_color);
    ds->FillRect(Rect(windowxp,windowyp,windowxp+windowwid,windowyp+windowhit), draw_color);
    draw_color = ds->GetCompatibleColor(0); 
    int bartop = windowyp + get_fixed_pixel_size(2);
    int barxp = windowxp + get_fixed_pixel_size(2);
    ds->FillRect(Rect(barxp,bartop, windowxp + windowwid - get_fixed_pixel_size(2),buttonyp-1), draw_color);
    for (ww = top_item; ww < numitems; ww++) {
        if (ww >= top_item + num_visible_items)
            break;
        Bitmap *spof=spriteset[dii[ww].sprnum];
        wputblock(ds, barxp+1+((ww-top_item)%4)*widest+widest/2-spof->GetWidth()/2,
            bartop+1+((ww-top_item)/4)*highest+highest/2-spof->GetHeight()/2,spof,1);
    }
    if ((spriteset[2041] == NULL) || (spriteset[2042] == NULL) || (spriteset[2043] == NULL))
        quit("!InventoryScreen: one or more of the inventory screen graphics have been deleted");
#define BUTTONWID spritewidth[2042]
    // Draw select, look and OK buttons
    wputblock(ds, windowxp+2, buttonyp + get_fixed_pixel_size(2), spriteset[2041], 1);
    wputblock(ds, windowxp+3+BUTTONWID, buttonyp + get_fixed_pixel_size(2), spriteset[2042], 1);
    wputblock(ds, windowxp+4+BUTTONWID*2, buttonyp + get_fixed_pixel_size(2), spriteset[2043], 1);

    // Draw Up and Down buttons if required
    const int ARROWBUTTONWID = 11;
    Bitmap *arrowblock = BitmapHelper::CreateTransparentBitmap (ARROWBUTTONWID, ARROWBUTTONWID);
    draw_color = arrowblock->GetCompatibleColor(0);
    if (play.sierra_inv_color == 0)
        draw_color = ds->GetCompatibleColor(14);

    arrowblock->DrawLine(Line(ARROWBUTTONWID/2, 2, ARROWBUTTONWID-2, 9), draw_color);
    arrowblock->DrawLine(Line(ARROWBUTTONWID/2, 2, 2, 9), draw_color);
    arrowblock->DrawLine(Line(2, 9, ARROWBUTTONWID-2, 9), draw_color);
	arrowblock->FloodFill(ARROWBUTTONWID/2, 4, draw_color);

    if (top_item > 0)
        wputblock(ds, windowxp+windowwid-ARROWBUTTONWID, buttonyp + get_fixed_pixel_size(2), arrowblock, 1);
    if (top_item + num_visible_items < numitems)
        arrowblock->FlipBlt(arrowblock, windowxp+windowwid-ARROWBUTTONWID, buttonyp + get_fixed_pixel_size(4) + ARROWBUTTONWID, Common::kBitmap_VFlip);
    delete arrowblock;

    domouse(1);
    set_mouse_cursor(cmode);
    int wasonitem=-1;
    while (!kbhit()) {
        timerloop = 0;
        NEXT_ITERATION();
        domouse(0);
        update_polled_audio_and_crossfade();
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
                    UpdateGameOnce();

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
                    UpdateGameOnce();

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
            draw_color = ds->GetCompatibleColor(0);
            ds->DrawRect(Rect(rectxp,rectyp,rectxp+widest-1,rectyp+highest-1), draw_color);
        }
        if (isonitem>=0) { draw_color = ds->GetCompatibleColor(14);//opts.invrectcol);
        rectxp=barxp+1+(isonitem%4)*widest;
        rectyp=bartop+1+((isonitem - top_item)/4)*highest;
        ds->DrawRect(Rect(rectxp,rectyp,rectxp+widest-1,rectyp+highest-1), draw_color);
        }
        domouse(1);
        }
        wasonitem=isonitem;
        PollUntilNextFrame();
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

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// void (GUIInv *guii)
RuntimeScriptValue Sc_InvWindow_ScrollDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIInv, InvWindow_ScrollDown);
}

// void (GUIInv *guii)
RuntimeScriptValue Sc_InvWindow_ScrollUp(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIInv, InvWindow_ScrollUp);
}

// CharacterInfo* (GUIInv *guii)
RuntimeScriptValue Sc_InvWindow_GetCharacterToUse(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIInv, CharacterInfo, ccDynamicCharacter, InvWindow_GetCharacterToUse);
}

// void (GUIInv *guii, CharacterInfo *chaa)
RuntimeScriptValue Sc_InvWindow_SetCharacterToUse(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUIInv, InvWindow_SetCharacterToUse, CharacterInfo);
}

// ScriptInvItem* (GUIInv *guii, int index)
RuntimeScriptValue Sc_InvWindow_GetItemAtIndex(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(GUIInv, ScriptInvItem, ccDynamicInv, InvWindow_GetItemAtIndex);
}

// int (GUIInv *guii)
RuntimeScriptValue Sc_InvWindow_GetItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInv, InvWindow_GetItemCount);
}

// int (GUIInv *guii)
RuntimeScriptValue Sc_InvWindow_GetItemHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInv, InvWindow_GetItemHeight);
}

// void (GUIInv *guii, int newhit)
RuntimeScriptValue Sc_InvWindow_SetItemHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIInv, InvWindow_SetItemHeight);
}

// int (GUIInv *guii)
RuntimeScriptValue Sc_InvWindow_GetItemWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInv, InvWindow_GetItemWidth);
}

// void (GUIInv *guii, int newwidth)
RuntimeScriptValue Sc_InvWindow_SetItemWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIInv, InvWindow_SetItemWidth);
}

// int (GUIInv *guii)
RuntimeScriptValue Sc_InvWindow_GetItemsPerRow(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInv, InvWindow_GetItemsPerRow);
}

// int (GUIInv *guii)
RuntimeScriptValue Sc_InvWindow_GetRowCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInv, InvWindow_GetRowCount);
}

// int (GUIInv *guii)
RuntimeScriptValue Sc_InvWindow_GetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInv, InvWindow_GetTopItem);
}

// void (GUIInv *guii, int topitem)
RuntimeScriptValue Sc_InvWindow_SetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIInv, InvWindow_SetTopItem);
}



void RegisterInventoryWindowAPI()
{
    ccAddExternalObjectFunction("InvWindow::ScrollDown^0",          Sc_InvWindow_ScrollDown);
    ccAddExternalObjectFunction("InvWindow::ScrollUp^0",            Sc_InvWindow_ScrollUp);
    ccAddExternalObjectFunction("InvWindow::get_CharacterToUse",    Sc_InvWindow_GetCharacterToUse);
    ccAddExternalObjectFunction("InvWindow::set_CharacterToUse",    Sc_InvWindow_SetCharacterToUse);
    ccAddExternalObjectFunction("InvWindow::geti_ItemAtIndex",      Sc_InvWindow_GetItemAtIndex);
    ccAddExternalObjectFunction("InvWindow::get_ItemCount",         Sc_InvWindow_GetItemCount);
    ccAddExternalObjectFunction("InvWindow::get_ItemHeight",        Sc_InvWindow_GetItemHeight);
    ccAddExternalObjectFunction("InvWindow::set_ItemHeight",        Sc_InvWindow_SetItemHeight);
    ccAddExternalObjectFunction("InvWindow::get_ItemWidth",         Sc_InvWindow_GetItemWidth);
    ccAddExternalObjectFunction("InvWindow::set_ItemWidth",         Sc_InvWindow_SetItemWidth);
    ccAddExternalObjectFunction("InvWindow::get_ItemsPerRow",       Sc_InvWindow_GetItemsPerRow);
    ccAddExternalObjectFunction("InvWindow::get_RowCount",          Sc_InvWindow_GetRowCount);
    ccAddExternalObjectFunction("InvWindow::get_TopItem",           Sc_InvWindow_GetTopItem);
    ccAddExternalObjectFunction("InvWindow::set_TopItem",           Sc_InvWindow_SetTopItem);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("InvWindow::ScrollDown^0",          (void*)InvWindow_ScrollDown);
    ccAddExternalFunctionForPlugin("InvWindow::ScrollUp^0",            (void*)InvWindow_ScrollUp);
    ccAddExternalFunctionForPlugin("InvWindow::get_CharacterToUse",    (void*)InvWindow_GetCharacterToUse);
    ccAddExternalFunctionForPlugin("InvWindow::set_CharacterToUse",    (void*)InvWindow_SetCharacterToUse);
    ccAddExternalFunctionForPlugin("InvWindow::geti_ItemAtIndex",      (void*)InvWindow_GetItemAtIndex);
    ccAddExternalFunctionForPlugin("InvWindow::get_ItemCount",         (void*)InvWindow_GetItemCount);
    ccAddExternalFunctionForPlugin("InvWindow::get_ItemHeight",        (void*)InvWindow_GetItemHeight);
    ccAddExternalFunctionForPlugin("InvWindow::set_ItemHeight",        (void*)InvWindow_SetItemHeight);
    ccAddExternalFunctionForPlugin("InvWindow::get_ItemWidth",         (void*)InvWindow_GetItemWidth);
    ccAddExternalFunctionForPlugin("InvWindow::set_ItemWidth",         (void*)InvWindow_SetItemWidth);
    ccAddExternalFunctionForPlugin("InvWindow::get_ItemsPerRow",       (void*)InvWindow_GetItemsPerRow);
    ccAddExternalFunctionForPlugin("InvWindow::get_RowCount",          (void*)InvWindow_GetRowCount);
    ccAddExternalFunctionForPlugin("InvWindow::get_TopItem",           (void*)InvWindow_GetTopItem);
    ccAddExternalFunctionForPlugin("InvWindow::set_TopItem",           (void*)InvWindow_SetTopItem);
}
