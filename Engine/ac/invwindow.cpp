//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
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
#include "ac/global_inventoryitem.h"
#include "ac/global_room.h"
#include "ac/gui.h"
#include "ac/mouse.h"
#include "ac/spritecache.h"
#include "ac/sys_events.h"
#include "ac/timer.h"
#include "ac/dynobj/cc_character.h"
#include "ac/dynobj/cc_inventory.h"
#include "debug/debug_log.h"
#include "gui/guidialog.h"
#include "gui/guimain.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "script/runtimescriptvalue.h"
#include "util/wgt2allg.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern ScriptInvItem scrInv[MAX_INV];
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;
extern SpriteCache spriteset;
extern CharacterInfo*playerchar;
extern AGSPlatformDriver *platform;
extern CCCharacter ccDynamicCharacter;
extern CCInventory ccDynamicInv;

int in_inv_screen = 0, inv_screen_newroom = -1;

// *** INV WINDOW FUNCTIONS

void InvWindow_SetCharacterToUse(GUIInvWindow *guii, CharacterInfo *chaa) {
  if (chaa == nullptr)
    guii->SetCharacterID(-1);
  else
    guii->SetCharacterID(chaa->index_id);
  // reset to top of list
  guii->SetTopItem(0);
}

CharacterInfo* InvWindow_GetCharacterToUse(GUIInvWindow *guii) {
  if (guii->GetCharacterID() < 0)
    return nullptr;

  return &game.chars[guii->GetCharacterID()];
}

void InvWindow_SetItemWidth(GUIInvWindow *guii, int newwidth) {
  guii->SetItemWidth(newwidth);
  guii->OnResized();
}

int InvWindow_GetItemWidth(GUIInvWindow *guii) {
  return guii->GetItemWidth();
}

void InvWindow_SetItemHeight(GUIInvWindow *guii, int newhit) {
  guii->SetItemHeight(newhit);
  guii->OnResized();
}

int InvWindow_GetItemHeight(GUIInvWindow *guii) {
  return guii->GetItemHeight();
}

void InvWindow_SetTopItem(GUIInvWindow *guii, int topitem) {
  if (guii->GetTopItem() != topitem) {
    guii->SetTopItem(topitem);
  }
}

int InvWindow_GetTopItem(GUIInvWindow *guii) {
  return guii->GetTopItem();
}

int InvWindow_GetItemsPerRow(GUIInvWindow *guii) {
  return guii->GetColCount();
}

int InvWindow_GetItemCount(GUIInvWindow *guii) {
  return charextra[guii->GetCharacterID()].invorder_count;
}

int InvWindow_GetRowCount(GUIInvWindow *guii) {
  return guii->GetRowCount();
}

void InvWindow_ScrollDown(GUIInvWindow *guii) {
  if ((charextra[guii->GetCharacterID()].invorder_count) >
      (guii->GetTopItem() + (guii->GetColCount() * guii->GetRowCount()))) { 
    guii->SetTopItem(guii->GetTopItem() + guii->GetColCount());
  }
}

void InvWindow_ScrollUp(GUIInvWindow *guii) {
  if (guii->GetTopItem() > 0) {
    guii->SetTopItem(std::max(0, guii->GetTopItem() - guii->GetColCount()));
  }
}

ScriptInvItem* InvWindow_GetItemAtIndex(GUIInvWindow *guii, int index) {
  if ((index < 0) || (index >= charextra[guii->GetCharacterID()].invorder_count))
    return nullptr;
  return &scrInv[charextra[guii->GetCharacterID()].invorder[index]];
}

//=============================================================================

int offset_over_inv(GUIInvWindow *inv) {
    if (inv->GetItemWidth() <= 0 || inv->GetItemHeight() <= 0)
        return -1;
    int mover = mouse_ifacebut_xoffs / data_to_game_coord(inv->GetItemWidth());
    // if it's off the edge of the visible items, ignore
    if (mover >= inv->GetColCount())
        return -1;
    mover += (mouse_ifacebut_yoffs / data_to_game_coord(inv->GetItemHeight())) * inv->GetColCount();
    if (mover >= inv->GetColCount() * inv->GetRowCount())
        return -1;

    mover += inv->GetTopItem();
    if ((mover < 0) || (mover >= charextra[inv->GetCharacterID()].invorder_count))
        return -1;

    return charextra[inv->GetCharacterID()].invorder[mover];
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

class InventoryScreen : public GameState
{
public:
    InventoryScreen();

    // Begin the state, initialize and prepare any resources
    void Begin() override;
    // End the state, release all resources
    void End() override;
    // Draw the state
    void Draw() override;
    // Update the state during a game tick
    bool Run() override;

    int GetResult() const { return toret; }

private:
    // Checks any exit conditions
    bool CheckExitCondition();
    // Updates an redraws inventory screen; returns if should continue
    bool UpdateAndDraw();
    void Draw(Bitmap *ds);
    void RedrawOverItem(Bitmap *ds, int isonitem);
    // Process all the buffered input events; returns if handled
    bool RunControls(int mx, int my, int isonitem);
    // Process single mouse event; returns if handled
    bool RunMouse(eAGSMouseButton mbut, int mx, int my, int isonitem);


    static const int ARROWBUTTONWID = 11;

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
    int btn_look_sprite;
    int btn_select_sprite;
    int btn_ok_sprite;

    bool is_done;
    bool need_redraw;
};

InventoryScreen::InventoryScreen()
{
}

void InventoryScreen::Begin()
{
    BUTTONAREAHEIGHT = get_fixed_pixel_size(30);
    cmode=CURS_ARROW;
    toret = -1;
    top_item = 0;
    num_visible_items = 0;
    MAX_ITEMAREA_HEIGHT = ((play.GetUIViewport().GetHeight() - BUTTONAREAHEIGHT) - get_fixed_pixel_size(20));
    in_inv_screen++;
    inv_screen_newroom = -1;

    // Sprites 2041, 2042 and 2043 were hardcoded in the older versions of
    // the engine to be used in the built-in inventory window.
    // If they did not exist engine first fell back to sprites 0, 1, 2 instead.
    // Fun fact: this fallback does not seem to be intentional, and was a
    // coincidental result of SpriteCache incorrectly remembering "last seeked
    // sprite" as 2041/2042/2043 while in fact stream was after sprite 0.
    if (!spriteset.DoesSpriteExist(2041) || !spriteset.DoesSpriteExist(2042) || !spriteset.DoesSpriteExist(2043))
        debug_script_warn("InventoryScreen: one or more of the inventory screen graphics (sprites 2041, 2042, 2043) does not exist, fallback to sprites 0, 1, 2 instead");
    btn_look_sprite = spriteset.DoesSpriteExist(2041) ? 2041 : 0;
    btn_select_sprite = spriteset.DoesSpriteExist(2042) ? 2042 : (spriteset.DoesSpriteExist(1) ? 1 : 0);
    btn_ok_sprite = spriteset.DoesSpriteExist(2043) ? 2043 : (spriteset.DoesSpriteExist(2) ? 2 : 0);

    is_done = false;
}

bool InventoryScreen::CheckExitCondition()
{
    if (charextra[game.playercharacter].invorder_count < 0)
        update_invorder();
    if (charextra[game.playercharacter].invorder_count == 0) {
        DisplayMessage(996);
        in_inv_screen--;
        return false;
    }

    if (inv_screen_newroom >= 0) {
        in_inv_screen--;
        NewRoom(inv_screen_newroom);
        return false;
    }

    return true;
}

// TODO: refactor and move to Run
bool InventoryScreen::UpdateAndDraw()
{
    if (!CheckExitCondition())
        return false;

    Draw();
    return true;
}

void InventoryScreen::Draw()
{
    if (charextra[game.playercharacter].invorder_count <= 0)
        return; // something is wrong, update needed?

    numitems = 0;
    widest = 0;
    highest = 0;

    for (int i = 0; i < charextra[game.playercharacter].invorder_count; ++i) {
        if (!game.invinfo[charextra[game.playercharacter].invorder[i]].name.IsEmpty()) {
            dii[numitems].num = charextra[game.playercharacter].invorder[i];
            dii[numitems].sprnum = game.invinfo[charextra[game.playercharacter].invorder[i]].pic;
            int snn=dii[numitems].sprnum;
            if (game.SpriteInfos[snn].Width > widest) widest=game.SpriteInfos[snn].Width;
            if (game.SpriteInfos[snn].Height > highest) highest= game.SpriteInfos[snn].Height;
            numitems++;
        }
    }
    if (numitems != charextra[game.playercharacter].invorder_count)
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
    windowxp=play.GetUIViewport().GetWidth()/2-windowwid/2;
    windowyp=play.GetUIViewport().GetHeight()/2-windowhit/2;
    buttonyp = windowhit - BUTTONAREAHEIGHT;
    bartop = get_fixed_pixel_size(2);
    barxp = get_fixed_pixel_size(2);

    Bitmap *ds = prepare_gui_screen(windowxp, windowyp, windowwid, windowhit, true);
    Draw(ds);
    set_mouse_cursor(cmode);
    wasonitem = -1;
}

void InventoryScreen::Draw(Bitmap *ds)
{
    color_t draw_color = ds->GetCompatibleColor(play.sierra_inv_color);
    ds->FillRect(Rect(0,0,windowwid,windowhit), draw_color);
    draw_color = ds->GetCompatibleColor(0);
    ds->FillRect(Rect(barxp,bartop, windowwid - get_fixed_pixel_size(2),buttonyp-1), draw_color);
    for (int i = top_item; i < numitems; ++i) {
        if (i >= top_item + num_visible_items)
            break;
        Bitmap *spof=spriteset[dii[i].sprnum];
        wputblock(ds, barxp+1+((i-top_item)%4)*widest+widest/2-spof->GetWidth()/2,
            bartop+1+((i-top_item)/4)*highest+highest/2-spof->GetHeight()/2,spof,1);
    }
#define BUTTONWID std::max(1, game.SpriteInfos[btn_select_sprite].Width)
    // Draw select, look and OK buttons
    wputblock(ds, 2, buttonyp + get_fixed_pixel_size(2), spriteset[btn_look_sprite], 1);
    wputblock(ds, 3+BUTTONWID, buttonyp + get_fixed_pixel_size(2), spriteset[btn_select_sprite], 1);
    wputblock(ds, 4+BUTTONWID*2, buttonyp + get_fixed_pixel_size(2), spriteset[btn_ok_sprite], 1);

    // Draw Up and Down buttons if required
    Bitmap *arrowblock = BitmapHelper::CreateTransparentBitmap (ARROWBUTTONWID, ARROWBUTTONWID);
    draw_color = arrowblock->GetCompatibleColor(0);
    if (play.sierra_inv_color == 0)
        draw_color = ds->GetCompatibleColor(14);

    arrowblock->DrawLine(Line(ARROWBUTTONWID/2, 2, ARROWBUTTONWID-2, 9), draw_color);
    arrowblock->DrawLine(Line(ARROWBUTTONWID/2, 2, 2, 9), draw_color);
    arrowblock->DrawLine(Line(2, 9, ARROWBUTTONWID-2, 9), draw_color);
	arrowblock->FloodFill(ARROWBUTTONWID/2, 4, draw_color);

    if (top_item > 0)
        wputblock(ds, windowwid-ARROWBUTTONWID, buttonyp + get_fixed_pixel_size(2), arrowblock, 1);
    if (top_item + num_visible_items < numitems)
        arrowblock->FlipBlt(arrowblock, windowwid-ARROWBUTTONWID, buttonyp + get_fixed_pixel_size(4) + ARROWBUTTONWID, Common::kFlip_Vertical);
    delete arrowblock;
}

void InventoryScreen::RedrawOverItem(Bitmap *ds, int isonitem)
{
    int rectxp=barxp+1+(wasonitem%4)*widest;
    int rectyp=bartop+1+((wasonitem - top_item)/4)*highest;
    if (wasonitem>=0)
    {
        color_t draw_color = ds->GetCompatibleColor(0);
        ds->DrawRect(Rect(rectxp,rectyp,rectxp+widest-1,rectyp+highest-1), draw_color);
    }
    if (isonitem>=0)
    {
        color_t draw_color = ds->GetCompatibleColor(14);//opts.invrectcol);
        rectxp=barxp+1+(isonitem%4)*widest;
        rectyp=bartop+1+((isonitem - top_item)/4)*highest;
        ds->DrawRect(Rect(rectxp,rectyp,rectxp+widest-1,rectyp+highest-1), draw_color);
    }
}

bool InventoryScreen::Run()
{
    if (!CheckExitCondition())
        return false;

    // Run() can be called in a loop, so keep events going.
    sys_evt_process_pending();

    need_redraw = false;

    update_audio_system_on_game_loop();
    refresh_gui_screen();

    // Handle mouse over options
    // NOTE: this is because old code was working with full game screen
    const int mx = ::mousex - windowxp;
    const int my = ::mousey - windowyp;

    int isonitem=((my-bartop)/highest)*ICONSPERLINE+(mx-barxp)/widest;
    if (my<=bartop) isonitem=-1;
    else if (isonitem >= 0) isonitem += top_item;
    if ((isonitem<0) | (isonitem>=numitems) | (isonitem >= top_item + num_visible_items))
        isonitem=-1;

    is_done = false;
    // Handle player's input
    RunControls(mx, my, isonitem);

    // Test if need to break the loop
    if (is_done)
    {
        return false;
    }
    // Handle redraw
    if (need_redraw)
    {
        is_done = !UpdateAndDraw();
        return !is_done;
    }
    else if (isonitem!=wasonitem)
    {
        RedrawOverItem(get_gui_screen(), isonitem);
    }
    wasonitem=isonitem;

    // Go for another inventory loop round
    update_polled_stuff();
    WaitForNextFrame();
    return true; // continue inventory screen loop
}

bool InventoryScreen::RunControls(int mx, int my, int isonitem)
{
    bool state_handled = false;
    for (InputType type = ags_inputevent_ready(); type != kInputNone; type = ags_inputevent_ready())
    {
        if (type == kInputKeyboard)
        {
            KeyInput ki;
            if (!run_service_key_controls(ki) || state_handled)
                continue; // handled by engine layer, or resolved
            if (!play.IsIgnoringInput() && !IsAGSServiceKey(ki.Key))
            {
                is_done = true;
                state_handled = true; // always handle for any key
            }
        }
        else if (type == kInputMouse)
        {
            eAGSMouseButton mbut;
            if (!run_service_mb_controls(mbut) || state_handled)
                continue; // handled by engine layer, or resolved
            if (!play.IsIgnoringInput() && RunMouse(mbut, mx, my, isonitem))
            {
                state_handled = true; // handled
            }
        }
        else
        {
            ags_drop_next_inputevent();
        }
    }
    ags_check_mouse_wheel(); // poll always, otherwise it accumulates
    return state_handled;
}

bool InventoryScreen::RunMouse(eAGSMouseButton mbut, int mx, int my, int isonitem)
{
    if (mbut == kMouseLeft)
    {
        if ((my < 0) | (my > windowhit) | (mx < 0) | (mx > windowwid))
        {
            return false; // not handled
        }
        else if (my < buttonyp)
        {
            // Left click on item
            int clickedon=isonitem;
            if (clickedon<0)
                return false; // not handled
            play.used_inv_on = dii[clickedon].num;

            if (cmode==MODE_LOOK)
            {
                RunInventoryInteraction(dii[clickedon].num, MODE_LOOK); 
                // in case the script did anything to the screen, redraw it
                UpdateGameOnce();
                need_redraw = true;
            }
            else if (cmode==MODE_USE)
            {
                // set the activeinv so the script can check it
                int activeinvwas = playerchar->activeinv;
                playerchar->activeinv = toret;

                RunInventoryInteraction(dii[clickedon].num, MODE_USE);

                // if the script didn't change it, then put it back
                if (playerchar->activeinv == toret)
                    playerchar->activeinv = activeinvwas;

                // in case the script did anything to the screen, redraw it
                UpdateGameOnce();

                // They used the active item and lost it
                if (playerchar->inv[toret] < 1)
                {
                    cmode = CURS_ARROW;
                    set_mouse_cursor(cmode);
                    toret = -1;
                }

                need_redraw = true;
            }
            else
            {
                // Select item
                toret=dii[clickedon].num;
                update_inv_cursor(toret);
                set_mouse_cursor(MODE_USE);
                cmode=MODE_USE;
            }
            return true; // handled
        }
        else
        {
            // Left click on button strip
            if (mx >= windowwid-ARROWBUTTONWID)
            {
                // Scroll arrows
                if (my < buttonyp + get_fixed_pixel_size(2) + ARROWBUTTONWID)
                {
                    if (top_item > 0)
                    {
                        top_item -= ICONSPERLINE;
                        need_redraw = true;
                    }
                }
                else if ((my < buttonyp + get_fixed_pixel_size(4) + ARROWBUTTONWID*2) && (top_item + num_visible_items < numitems))
                {
                    top_item += ICONSPERLINE;
                    need_redraw = true;
                }
                return true; // handled
            }

            int buton=mx-2;
            if (buton<0)
                return false; // no button, not handled
            buton/=BUTTONWID;
            if (buton>=3)
                return false; // no button, not handled
            // Click on actual button
            if (buton==0)
            { // Switch to Look cursor
                toret=-1; cmode=MODE_LOOK;
                set_mouse_cursor(cmode);
            }
            else if (buton==1)
            { // Switch to Select (arrow) cursor
                cmode=CURS_ARROW; toret=-1;
                set_mouse_cursor(cmode);
            }
            else
            { // Close inventory screen
                is_done = true;
            }
            return true; // handled
        }
    }
    else if (mbut == kMouseRight)
    {
        if (cmode == CURS_ARROW)
            cmode = MODE_LOOK;
        else
            cmode = CURS_ARROW;
        toret = -1;
        set_mouse_cursor(cmode);
        return true; // handled
    }

    return false; // other mouse button, not handled
}

void InventoryScreen::End()
{
    clear_gui_screen();
    set_default_cursor();
    invalidate_screen();
    in_inv_screen--;
}

int __actual_invscreen()
{
    InventoryScreen invscr;

    invscr.Begin();
    invscr.Draw();
    while (invscr.Run());
    invscr.End();

    return invscr.GetResult();
}

int invscreen() {
    int selt=__actual_invscreen();
    if (selt<0) return -1;
    playerchar->activeinv=selt;
    GUIE::MarkInventoryForUpdate(playerchar->index_id, true);
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

// void (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_ScrollDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIInvWindow, InvWindow_ScrollDown);
}

// void (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_ScrollUp(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIInvWindow, InvWindow_ScrollUp);
}

// CharacterInfo* (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetCharacterToUse(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIInvWindow, CharacterInfo, ccDynamicCharacter, InvWindow_GetCharacterToUse);
}

// void (GUIInvWindow *guii, CharacterInfo *chaa)
RuntimeScriptValue Sc_InvWindow_SetCharacterToUse(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUIInvWindow, InvWindow_SetCharacterToUse, CharacterInfo);
}

// ScriptInvItem* (GUIInvWindow *guii, int index)
RuntimeScriptValue Sc_InvWindow_GetItemAtIndex(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(GUIInvWindow, ScriptInvItem, ccDynamicInv, InvWindow_GetItemAtIndex);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetItemCount);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetItemHeight);
}

// void (GUIInvWindow *guii, int newhit)
RuntimeScriptValue Sc_InvWindow_SetItemHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIInvWindow, InvWindow_SetItemHeight);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetItemWidth);
}

// void (GUIInvWindow *guii, int newwidth)
RuntimeScriptValue Sc_InvWindow_SetItemWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIInvWindow, InvWindow_SetItemWidth);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemsPerRow(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetItemsPerRow);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetRowCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetRowCount);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetTopItem);
}

// void (GUIInvWindow *guii, int topitem)
RuntimeScriptValue Sc_InvWindow_SetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIInvWindow, InvWindow_SetTopItem);
}



void RegisterInventoryWindowAPI()
{
    ScFnRegister invwindow_api[] = {
        { "InvWindow::ScrollDown^0",          API_FN_PAIR(InvWindow_ScrollDown) },
        { "InvWindow::ScrollUp^0",            API_FN_PAIR(InvWindow_ScrollUp) },
        { "InvWindow::get_CharacterToUse",    API_FN_PAIR(InvWindow_GetCharacterToUse) },
        { "InvWindow::set_CharacterToUse",    API_FN_PAIR(InvWindow_SetCharacterToUse) },
        { "InvWindow::geti_ItemAtIndex",      API_FN_PAIR(InvWindow_GetItemAtIndex) },
        { "InvWindow::get_ItemCount",         API_FN_PAIR(InvWindow_GetItemCount) },
        { "InvWindow::get_ItemHeight",        API_FN_PAIR(InvWindow_GetItemHeight) },
        { "InvWindow::set_ItemHeight",        API_FN_PAIR(InvWindow_SetItemHeight) },
        { "InvWindow::get_ItemWidth",         API_FN_PAIR(InvWindow_GetItemWidth) },
        { "InvWindow::set_ItemWidth",         API_FN_PAIR(InvWindow_SetItemWidth) },
        { "InvWindow::get_ItemsPerRow",       API_FN_PAIR(InvWindow_GetItemsPerRow) },
        { "InvWindow::get_RowCount",          API_FN_PAIR(InvWindow_GetRowCount) },
        { "InvWindow::get_TopItem",           API_FN_PAIR(InvWindow_GetTopItem) },
        { "InvWindow::set_TopItem",           API_FN_PAIR(InvWindow_SetTopItem) },
    };

    ccAddExternalFunctions(invwindow_api);
}
