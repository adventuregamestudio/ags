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
#include "ac/global_character.h"
#include "ac/global_display.h"
#include "ac/global_room.h"
#include "ac/mouse.h"
#include "ac/record.h"
#include "game/game_objects.h"
#include "game/script_objects.h"
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
extern CharacterExtras *charextra;
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

int in_inv_screen = 0, inv_screen_newroom = -1;

// *** INV WINDOW FUNCTIONS

void InvWindow_SetCharacterToUse(GuiInvWindow *guii, CharacterInfo *chaa) {
  if (chaa == NULL)
    guii->CharacterId = -1;
  else
    guii->CharacterId = chaa->index_id;
  // reset to top of list
  guii->TopItem = 0;

  guis_need_update = 1;
}

CharacterInfo* InvWindow_GetCharacterToUse(GuiInvWindow *guii) {
  if (guii->CharacterId < 0)
    return NULL;

  return &game.Characters[guii->CharacterId];
}

void InvWindow_SetItemWidth(GuiInvWindow *guii, int newwidth) {
  guii->ItemWidth = newwidth;
  guii->OnResized();
}

int InvWindow_GetItemWidth(GuiInvWindow *guii) {
  return guii->ItemWidth;
}

void InvWindow_SetItemHeight(GuiInvWindow *guii, int newhit) {
  guii->ItemHeight = newhit;
  guii->OnResized();
}

int InvWindow_GetItemHeight(GuiInvWindow *guii) {
  return guii->ItemHeight;
}

void InvWindow_SetTopItem(GuiInvWindow *guii, int topitem) {
  if (guii->TopItem != topitem) {
    guii->TopItem = topitem;
    guis_need_update = 1;
  }
}

int InvWindow_GetTopItem(GuiInvWindow *guii) {
  return guii->TopItem;
}

int InvWindow_GetItemsPerRow(GuiInvWindow *guii) {
  return guii->ColumnCount;
}

int InvWindow_GetItemCount(GuiInvWindow *guii) {
  return charextra[guii->GetCharacterId()].invorder_count;
}

int InvWindow_GetRowCount(GuiInvWindow *guii) {
  return guii->RowCount;
}

void InvWindow_ScrollDown(GuiInvWindow *guii) {
  if ((charextra[guii->GetCharacterId()].invorder_count) >
      (guii->TopItem + (guii->ColumnCount * guii->RowCount))) { 
    guii->TopItem += guii->ColumnCount;
    guis_need_update = 1;
  }
}

void InvWindow_ScrollUp(GuiInvWindow *guii) {
  if (guii->TopItem > 0) {
    guii->TopItem -= guii->ColumnCount;
    if (guii->TopItem < 0)
      guii->TopItem = 0;

    guis_need_update = 1;
  }
}

ScriptInvItem* InvWindow_GetItemAtIndex(GuiInvWindow *guii, int index) {
  if ((index < 0) || (index >= charextra[guii->GetCharacterId()].invorder_count))
    return NULL;
  return &scrInv[charextra[guii->GetCharacterId()].invorder[index]];
}

//=============================================================================

int offset_over_inv(GuiInvWindow *inv) {

    int mover = mouse_ifacebut_xoffs / multiply_up_coordinate(inv->ItemWidth);
    // if it's off the edge of the visible items, ignore
    if (mover >= inv->ColumnCount)
        return -1;
    mover += (mouse_ifacebut_yoffs / multiply_up_coordinate(inv->ItemHeight)) * inv->ColumnCount;
    if (mover >= inv->ColumnCount * inv->RowCount)
        return -1;

    mover += inv->TopItem;
    if ((mover < 0) || (mover >= charextra[inv->GetCharacterId()].invorder_count))
        return -1;

    return charextra[inv->GetCharacterId()].invorder[mover];
}

//
// NOTE: This is an old default inventory screen implementation,
// which became completely obsolete after AGS 2.72.
//

#define ICONSPERLINE 4

struct DisplayInvItem {
    int num;
    int sprnum;
};

struct InventoryScreen
{
    int BUTTONAREAHEIGHT;
    int cmode;
    int toret;
    int top_item;
    int num_visible_items;
    int MAX_ITEMAREA_HEIGHT;
    int wasonitem;
    int bartop;
    int barxp;
    int numitems;
    int widest;
    int highest;
    int windowwid;
    int windowhit;
    int windowxp;
    int windowyp;
    int buttonyp;
    DisplayInvItem dii[MAX_INV];
    static const int ARROWBUTTONWID;

    int break_code;

    void Prepare();
    int Redraw();
    bool Run();
    void Close();
};

const int InventoryScreen::ARROWBUTTONWID = 11;

InventoryScreen InvScr;

void InventoryScreen::Prepare()
{
    BUTTONAREAHEIGHT = get_fixed_pixel_size(30);
    cmode=CURS_ARROW;
    toret = -1;
    top_item = 0;
    num_visible_items = 0;
    MAX_ITEMAREA_HEIGHT = ((scrnhit - BUTTONAREAHEIGHT) - get_fixed_pixel_size(20));
    in_inv_screen++;
    inv_screen_newroom = -1;

    break_code = 0;
}

int InventoryScreen::Redraw()
{
    Bitmap *ds = SetVirtualScreen(virtual_screen);

    numitems=0;
    widest=0;
    highest=0;
    if (charextra[game.PlayerCharacterIndex].invorder_count < 0)
        update_invorder();
    if (charextra[game.PlayerCharacterIndex].invorder_count == 0) {
        DisplayMessage(996);
        in_inv_screen--;
        return -1;
    }

    if (inv_screen_newroom >= 0) {
        in_inv_screen--;
        NewRoom(inv_screen_newroom);
        return -1;
    }

    for (int i = 0; i < charextra[game.PlayerCharacterIndex].invorder_count; ++i) {
        if (game.InventoryItems[charextra[game.PlayerCharacterIndex].invorder[i]].name[0]!=0) {
            dii[numitems].num = charextra[game.PlayerCharacterIndex].invorder[i];
            dii[numitems].sprnum = game.InventoryItems[charextra[game.PlayerCharacterIndex].invorder[i]].pic;
            int snn=dii[numitems].sprnum;
            if (spritewidth[snn] > widest) widest=spritewidth[snn];
            if (spriteheight[snn] > highest) highest=spriteheight[snn];
            numitems++;
        }
    }
    if (numitems != charextra[game.PlayerCharacterIndex].invorder_count)
        quit("inconsistent inventory calculations");

    widest += get_fixed_pixel_size(4);
    highest += get_fixed_pixel_size(4);
    num_visible_items = (MAX_ITEMAREA_HEIGHT / highest) * ICONSPERLINE;

    windowhit = highest * (numitems/ICONSPERLINE) + get_fixed_pixel_size(4);
    if ((numitems%ICONSPERLINE) !=0) windowhit+=highest;
    if (windowhit > MAX_ITEMAREA_HEIGHT) {
        windowhit = (MAX_ITEMAREA_HEIGHT / highest) * highest + get_fixed_pixel_size(4);
    }
    windowhit += BUTTONAREAHEIGHT;

    windowwid = widest*ICONSPERLINE + get_fixed_pixel_size(4);
    if (windowwid < get_fixed_pixel_size(105)) windowwid = get_fixed_pixel_size(105);
    windowxp=scrnwid/2-windowwid/2;
    windowyp=scrnhit/2-windowhit/2;
    buttonyp=windowyp+windowhit-BUTTONAREAHEIGHT;
    color_t draw_color = ds->GetCompatibleColor(play.SierraInventoryBkgColour);
    ds->FillRect(Rect(windowxp,windowyp,windowxp+windowwid,windowyp+windowhit), draw_color);
    draw_color = ds->GetCompatibleColor(0); 
    bartop = windowyp + get_fixed_pixel_size(2);
    barxp = windowxp + get_fixed_pixel_size(2);
    ds->FillRect(Rect(barxp,bartop, windowxp + windowwid - get_fixed_pixel_size(2),buttonyp-1), draw_color);
    for (int i = top_item; i < numitems; ++i) {
        if (i >= top_item + num_visible_items)
            break;
        Bitmap *spof=spriteset[dii[i].sprnum];
        wputblock(ds, barxp+1+((i-top_item)%4)*widest+widest/2-spof->GetWidth()/2,
            bartop+1+((i-top_item)/4)*highest+highest/2-spof->GetHeight()/2,spof,1);
    }
    if ((spriteset[2041] == NULL) || (spriteset[2042] == NULL) || (spriteset[2043] == NULL))
        quit("!InventoryScreen: one or more of the inventory screen graphics have been deleted");
#define BUTTONWID spritewidth[2042]
    // Draw select, look and OK buttons
    wputblock(ds, windowxp+2, buttonyp + get_fixed_pixel_size(2), spriteset[2041], 1);
    wputblock(ds, windowxp+3+BUTTONWID, buttonyp + get_fixed_pixel_size(2), spriteset[2042], 1);
    wputblock(ds, windowxp+4+BUTTONWID*2, buttonyp + get_fixed_pixel_size(2), spriteset[2043], 1);

    // Draw Up and Down buttons if required
    Bitmap *arrowblock = BitmapHelper::CreateTransparentBitmap (ARROWBUTTONWID, ARROWBUTTONWID);
    draw_color = arrowblock->GetCompatibleColor(0);
    if (play.SierraInventoryBkgColour == 0)
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
    wasonitem=-1;

    return 0;
}

bool InventoryScreen::Run()
{
    if (kbhit() != 0)
    {
        return false; // end inventory screen loop
    }

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
                return true; // continue inventory screen loop
            if (mousey<buttonyp) {
                int clickedon=isonitem;
                if (clickedon<0) return true; // continue inventory screen loop
                evblocknum=dii[clickedon].num;
                play.ClickedInvItemIndex = dii[clickedon].num;

                if (cmode==MODE_LOOK) {
                    domouse(2);
                    run_event_block_inv(dii[clickedon].num, 0); 
                    // in case the script did anything to the screen, redraw it
                    UpdateGameOnce();

                    break_code = Redraw();
                    return break_code == 0;
                }
                else if (cmode==MODE_USE) {
                    // use objects on each other
                    play.UsedInvItemIndex=toret;

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

                    break_code = Redraw();
                    return break_code == 0;
                }
                toret=dii[clickedon].num;
                //        int plusng=play.using; play.using=toret;
                update_inv_cursor(toret);
                set_mouse_cursor(MODE_USE);
                cmode=MODE_USE;
                //        play.using=plusng;
                //        break;
                return true; // continue inventory screen loop
            }
            else {
                if (mousex >= windowxp+windowwid-ARROWBUTTONWID) {
                    if (mousey < buttonyp + get_fixed_pixel_size(2) + ARROWBUTTONWID) {
                        if (top_item > 0) {
                            top_item -= ICONSPERLINE;
                            domouse(2);

                            break_code = Redraw();
                            return break_code == 0;
                        }
                    }
                    else if ((mousey < buttonyp + get_fixed_pixel_size(4) + ARROWBUTTONWID*2) && (top_item + num_visible_items < numitems)) {
                        top_item += ICONSPERLINE;
                        domouse(2);
                        
                        break_code = Redraw();
                        return break_code == 0;
                    }
                    return true; // continue inventory screen loop
                }

                int buton=(mousex-windowxp)-2;
                if (buton<0) return true; // continue inventory screen loop
                buton/=BUTTONWID;
                if (buton>=3) return true; // continue inventory screen loop
                if (buton==0) { toret=-1; cmode=MODE_LOOK; }
                else if (buton==1) { cmode=CURS_ARROW; toret=-1; }
                else
                {
                    return false; // end inventory screen loop
                }
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
        Bitmap *ds = SetVirtualScreen(virtual_screen);
        color_t draw_color;
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
        while (timerloop == 0) {
            update_polled_stuff_if_runtime();
            platform->YieldCPU();
        }

    return true; // continue inventory screen loop
}

void InventoryScreen::Close()
{
    set_default_cursor();
    domouse(2);
    construct_virtual_screen(true);
    in_inv_screen--;
}

int __actual_invscreen()
{
    InvScr.Prepare();
    InvScr.break_code = InvScr.Redraw();
    if (InvScr.break_code != 0)
    {
        return InvScr.break_code;
    }

    while (InvScr.Run());

    if (InvScr.break_code != 0)
    {
        return InvScr.break_code;
    }

    // Clear buffered keypresses
    while (kbhit()) getch();

    InvScr.Close();
    return InvScr.toret;
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

// void (GuiInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_ScrollDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GuiInvWindow, InvWindow_ScrollDown);
}

// void (GuiInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_ScrollUp(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GuiInvWindow, InvWindow_ScrollUp);
}

// CharacterInfo* (GuiInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetCharacterToUse(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GuiInvWindow, CharacterInfo, ccDynamicCharacter, InvWindow_GetCharacterToUse);
}

// void (GuiInvWindow *guii, CharacterInfo *chaa)
RuntimeScriptValue Sc_InvWindow_SetCharacterToUse(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GuiInvWindow, InvWindow_SetCharacterToUse, CharacterInfo);
}

// ScriptInvItem* (GuiInvWindow *guii, int index)
RuntimeScriptValue Sc_InvWindow_GetItemAtIndex(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(GuiInvWindow, ScriptInvItem, ccDynamicInv, InvWindow_GetItemAtIndex);
}

// int (GuiInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiInvWindow, InvWindow_GetItemCount);
}

// int (GuiInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiInvWindow, InvWindow_GetItemHeight);
}

// void (GuiInvWindow *guii, int newhit)
RuntimeScriptValue Sc_InvWindow_SetItemHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiInvWindow, InvWindow_SetItemHeight);
}

// int (GuiInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiInvWindow, InvWindow_GetItemWidth);
}

// void (GuiInvWindow *guii, int newwidth)
RuntimeScriptValue Sc_InvWindow_SetItemWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiInvWindow, InvWindow_SetItemWidth);
}

// int (GuiInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemsPerRow(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiInvWindow, InvWindow_GetItemsPerRow);
}

// int (GuiInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetRowCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiInvWindow, InvWindow_GetRowCount);
}

// int (GuiInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiInvWindow, InvWindow_GetTopItem);
}

// void (GuiInvWindow *guii, int topitem)
RuntimeScriptValue Sc_InvWindow_SetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiInvWindow, InvWindow_SetTopItem);
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
