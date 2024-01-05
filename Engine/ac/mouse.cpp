//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/mouse.h"
#include "ac/common.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/dynobj/scriptmouse.h"
#include "ac/dynobj/scriptsystem.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_mouse.h"
#include "ac/global_screen.h"
#include "ac/sys_events.h"
#include "ac/system.h"
#include "ac/viewframe.h"
#include "debug/debug_log.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "device/mousew32.h"
#include "ac/spritecache.h"
#include "gfx/graphicsdriver.h"
#include "gfx/gfxfilter.h"
#include "platform/base/agsplatformdriver.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern GameState play;
extern ScriptSystem scsystem;
extern SpriteCache spriteset;
extern CharacterInfo*playerchar;
extern IGraphicsDriver *gfxDriver;

ScriptMouse scmouse;
int cur_mode,cur_cursor;
int mouse_frame=0,mouse_delay=0;
int lastmx=-1,lastmy=-1;
std::unique_ptr<Bitmap> dotted_mouse_cursor;
std::unique_ptr<Bitmap> blank_mouse_cursor;
// Current mouse cursor, may be a sprite or a generated bitmap
// TODO: refactor, replace these with ObjTexture, and move to draw.cpp
int mouse_cur_pic = 0;
bool alpha_blend_cursor = false;
IDriverDependantBitmap *mouse_cur_ddb = nullptr;

// The Mouse:: functions are static so the script doesn't pass
// in an object parameter
void Mouse_SetVisible(int isOn) {
    if (isOn)
        ShowMouseCursor();
    else
        HideMouseCursor();
}

int Mouse_GetVisible() {
    if (play.mouse_cursor_hidden)
        return 0;
    return 1;
}

void SetMouseBounds(int x1, int y1, int x2, int y2)
{
    int xmax = game_to_data_coord(play.GetMainViewport().GetWidth()) - 1;
    int ymax = game_to_data_coord(play.GetMainViewport().GetHeight()) - 1;
    if ((x1 == 0) && (y1 == 0) && (x2 == 0) && (y2 == 0))
    {
        x2 = xmax;
        y2 = ymax;
    }
    else
    {
        if (x1 < 0 || x1 > xmax || x2 < 0 || x2 > xmax || x1 > x2 || y1 < 0 || y1 > ymax || y2 < 0 || y2 > ymax || y1 > y2)
            debug_script_warn("SetMouseBounds: arguments are out of range and will be corrected: (%d,%d)-(%d,%d), range is (%d,%d)-(%d,%d)",
                x1, y1, x2, y2, 0, 0, xmax, ymax);
        x1 = Math::Clamp(x1, 0, xmax);
        x2 = Math::Clamp(x2, x1, xmax);
        y1 = Math::Clamp(y1, 0, ymax);
        y2 = Math::Clamp(y2, y1, ymax);
    }

    debug_script_log("Mouse bounds constrained to (%d,%d)-(%d,%d)", x1, y1, x2, y2);
    data_to_game_coords(&x1, &y1);
    data_to_game_round_up(&x2, &y2);

    play.mboundx1 = x1;
    play.mboundx2 = x2;
    play.mboundy1 = y1;
    play.mboundy2 = y2;
    Mouse::SetMoveLimit(Rect(x1, y1, x2, y2));
}

// mouse cursor functions:
void update_cached_mouse_cursor(Bitmap *use_bmp) 
{
    mouse_cur_ddb = recycle_ddb_bitmap(mouse_cur_ddb, use_bmp, alpha_blend_cursor);
}

// set_mouse_cursor: changes visual appearance to specified cursor
void set_mouse_cursor(int newcurs) {
    const int hotspotx = game.mcurs[newcurs].hotx, hotspoty = game.mcurs[newcurs].hoty;
    Mouse::SetHotspot(hotspotx, hotspoty);

    // if it's same cursor and there's animation in progress, then don't assign a new pic just yet
    if (newcurs == cur_cursor && game.mcurs[newcurs].view >= 0 &&
        (mouse_frame > 0 || mouse_delay > 0))
    {
        return;
    }

    // reset animation timing only if it's another cursor
    if (newcurs != cur_cursor)
    {
        cur_cursor = newcurs;
        mouse_frame = 0;
        mouse_delay = 0;
    }

    // Assign new pic
    set_new_cursor_graphic(game.mcurs[newcurs].pic);
    dotted_mouse_cursor = nullptr;

    // If it's inventory cursor, draw hotspot crosshair sprite upon it
    if ((newcurs == MODE_USE) && (game.mcurs[newcurs].pic > 0) &&
        ((game.hotdot > 0) || (game.invhotdotsprite > 0)) ) {
            // If necessary, create a copy of the cursor and put the hotspot dot onto it
            Bitmap *mouse_cur_bmp = (mouse_cur_pic >= 0) ? spriteset[mouse_cur_pic] : blank_mouse_cursor.get();
            dotted_mouse_cursor.reset(BitmapHelper::CreateBitmapCopy(mouse_cur_bmp));

            if (game.invhotdotsprite > 0) {
                draw_sprite_slot_support_alpha(dotted_mouse_cursor.get(),
                    (game.SpriteInfos[game.mcurs[newcurs].pic].Flags & SPF_ALPHACHANNEL) != 0,
                    hotspotx - game.SpriteInfos[game.invhotdotsprite].Width / 2,
                    hotspoty - game.SpriteInfos[game.invhotdotsprite].Height / 2,
                    game.invhotdotsprite);
            }
            else {
                putpixel_compensate (dotted_mouse_cursor.get(), hotspotx, hotspoty, MakeColor(game.hotdot));

                if (game.hotdotouter > 0) {
                    int outercol = MakeColor(game.hotdotouter);

                    putpixel_compensate (dotted_mouse_cursor.get(), hotspotx + get_fixed_pixel_size(1), hotspoty, outercol);
                    putpixel_compensate (dotted_mouse_cursor.get(), hotspotx, hotspoty + get_fixed_pixel_size(1), outercol);
                    putpixel_compensate (dotted_mouse_cursor.get(), hotspotx - get_fixed_pixel_size(1), hotspoty, outercol);
                    putpixel_compensate (dotted_mouse_cursor.get(), hotspotx, hotspoty - get_fixed_pixel_size(1), outercol);
                }
            }

            update_cached_mouse_cursor(dotted_mouse_cursor.get());
    }
}

// set_default_cursor: resets visual appearance to current mode (walk, look, etc)
void set_default_cursor() {
    set_mouse_cursor(cur_mode);
}

// permanently change cursor graphic
void ChangeCursorGraphic (int curs, int newslot) {
    if ((curs < 0) || (curs >= game.numcursors))
        quit("!ChangeCursorGraphic: invalid mouse cursor");

    if ((curs == MODE_USE) && (game.options[OPT_FIXEDINVCURSOR] == 0))
        debug_script_warn("Mouse.ChangeModeGraphic should not be used on the Inventory cursor when the cursor is linked to the active inventory item");

    game.mcurs[curs].pic = newslot;
    spriteset.PrecacheSprite(newslot);
    if (curs == cur_mode)
        set_mouse_cursor (curs);
}

int Mouse_GetModeGraphic(int curs) {
    if ((curs < 0) || (curs >= game.numcursors))
        quit("!Mouse.GetModeGraphic: invalid mouse cursor");

    return game.mcurs[curs].pic;
}

void ChangeCursorHotspot (int curs, int x, int y) {
    if ((curs < 0) || (curs >= game.numcursors))
        quit("!ChangeCursorHotspot: invalid mouse cursor");
    game.mcurs[curs].hotx = data_to_game_coord(x);
    game.mcurs[curs].hoty = data_to_game_coord(y);
    if (curs == cur_cursor)
        set_mouse_cursor (cur_cursor);
}

void Mouse_ChangeModeView(int curs, int newview, int delay) {
    if ((curs < 0) || (curs >= game.numcursors))
        quit("!Mouse.ChangeModeView: invalid mouse cursor");

    newview--;

    game.mcurs[curs].view = newview;
    if (delay != SCR_NO_VALUE)
        game.mcurs[curs].animdelay = delay;

    if (newview >= 0)
    {
        precache_view(newview);
    }

    if (curs == cur_cursor)
        mouse_delay = 0;  // force update
}

void Mouse_ChangeModeView2(int curs, int newview) {
    Mouse_ChangeModeView(curs, newview, SCR_NO_VALUE);
}

void SetNextCursor () {
    set_cursor_mode (find_next_enabled_cursor(cur_mode + 1));
}

void SetPreviousCursor() {
    set_cursor_mode(find_previous_enabled_cursor(cur_mode - 1));
}

// set_cursor_mode: changes mode and appearance
void set_cursor_mode(int newmode) {
    if ((newmode < 0) || (newmode >= game.numcursors))
        quit("!SetCursorMode: invalid cursor mode specified");

    if (game.mcurs[newmode].flags & MCF_DISABLED) {
        find_next_enabled_cursor(newmode);
        return; }
    if (newmode == MODE_USE) {
        if (playerchar->activeinv == -1) {
            find_next_enabled_cursor(0);
            return;
        }
        update_inv_cursor(playerchar->activeinv);
    }
    cur_mode=newmode;
    set_default_cursor();

    debug_script_log("Cursor mode set to %d", newmode);
}

void enable_cursor_mode(int modd) {
    game.mcurs[modd].flags&=~MCF_DISABLED;
    // now search the interfaces for related buttons to re-enable
    int uu,ww;

    for (uu=0;uu<game.numgui;uu++) {
        for (ww=0;ww<guis[uu].GetControlCount();ww++) {
            if (guis[uu].GetControlType(ww) != kGUIButton) continue;
            GUIButton*gbpt=(GUIButton*)guis[uu].GetControl(ww);
            if (gbpt->ClickAction[kGUIClickLeft]!=kGUIAction_SetMode) continue;
            if (gbpt->ClickData[kGUIClickLeft]!=modd) continue;
            gbpt->SetEnabled(true);
        }
    }
}

void disable_cursor_mode(int modd) {
    game.mcurs[modd].flags|=MCF_DISABLED;
    // now search the interfaces for related buttons to kill
    int uu,ww;

    for (uu=0;uu<game.numgui;uu++) {
        for (ww=0;ww<guis[uu].GetControlCount();ww++) {
            if (guis[uu].GetControlType(ww) != kGUIButton) continue;
            GUIButton*gbpt=(GUIButton*)guis[uu].GetControl(ww);
            if (gbpt->ClickAction[kGUIClickLeft]!=kGUIAction_SetMode) continue;
            if (gbpt->ClickData[kGUIClickLeft]!=modd) continue;
            gbpt->SetEnabled(false);
        }
    }
    if (cur_mode==modd) find_next_enabled_cursor(modd);
}

void RefreshMouse() {
    ags_domouse();
    scmouse.x = game_to_data_coord(mousex);
    scmouse.y = game_to_data_coord(mousey);
}

void SetMousePosition (int newx, int newy) {
    const Rect &viewport = play.GetMainViewport();

    if (newx < 0)
        newx = 0;
    if (newy < 0)
        newy = 0;
    if (newx >= viewport.GetWidth())
        newx = viewport.GetWidth() - 1;
    if (newy >= viewport.GetHeight())
        newy = viewport.GetHeight() - 1;

    data_to_game_coords(&newx, &newy);
    Mouse::SetPosition(Point(newx, newy));
    RefreshMouse();
}

int GetCursorMode() {
    return cur_mode;
}

int IsButtonDown(int which) {
    if ((which < kMouseLeft) || (which > kMouseMiddle))
        quit("!IsButtonDown: only works with eMouseLeft, eMouseRight, eMouseMiddle");
    return ags_misbuttondown(static_cast<eAGSMouseButton>(which)) ? 1 : 0;
}

int IsModeEnabled(int which) {
    return (which < 0) || (which >= game.numcursors) ? 0 :
        which == MODE_USE ? playerchar->activeinv > 0 :
        (game.mcurs[which].flags & MCF_DISABLED) == 0;
}

void SimulateMouseClick(int button_id) {
    ags_simulate_mouseclick(static_cast<eAGSMouseButton>(button_id));
}

void Mouse_EnableControl(bool on)
{
    bool should_control_mouse =
        usetup.mouse_ctrl_when == kMouseCtrl_Always ||
        (usetup.mouse_ctrl_when == kMouseCtrl_Fullscreen && (scsystem.windowed == 0));
    Mouse::SetMovementControl(should_control_mouse & on);
    usetup.mouse_ctrl_enabled = on; // remember setting in config
}

bool Mouse_GetAutoLock()
{
    return usetup.mouse_auto_lock;
}

void Mouse_SetAutoLock(bool on)
{
    usetup.mouse_auto_lock = on;
    if (scsystem.windowed)
    {
        if (usetup.mouse_auto_lock)
            Mouse::TryLockToWindow();
        else
            Mouse::UnlockFromWindow();
    }
}

//=============================================================================

int GetMouseCursor() {
    return cur_cursor;
}

void update_script_mouse_coords() {
    scmouse.x = game_to_data_coord(mousex);
    scmouse.y = game_to_data_coord(mousey);
}

void update_inv_cursor(int invnum) {

    if ((game.options[OPT_FIXEDINVCURSOR]==0) && (invnum > 0)) {
        int cursorSprite = game.invinfo[invnum].cursorPic;

        // Fall back to the inventory pic if no cursor pic is defined.
        if (cursorSprite == 0)
            cursorSprite = game.invinfo[invnum].pic;

        game.mcurs[MODE_USE].pic = cursorSprite;
        // all cursor images must be pre-cached
        spriteset.PrecacheSprite(cursorSprite);

        if ((game.invinfo[invnum].hotx > 0) || (game.invinfo[invnum].hoty > 0)) {
            // if the hotspot was set (unfortunately 0,0 isn't a valid co-ord)
            game.mcurs[MODE_USE].hotx=game.invinfo[invnum].hotx;
            game.mcurs[MODE_USE].hoty=game.invinfo[invnum].hoty;
        }
        else {
            game.mcurs[MODE_USE].hotx = game.SpriteInfos[cursorSprite].Width / 2;
            game.mcurs[MODE_USE].hoty = game.SpriteInfos[cursorSprite].Height / 2;
        }
    }
}

void set_new_cursor_graphic (int spriteslot)
{
    // It looks like spriteslot 0 can be used in games with version 2.72 and lower.
    // The NULL check should ensure that the sprite is valid anyway.
    Bitmap *mouse_cur_bmp;
    if ((spriteslot < 1) && (loaded_game_file_version > kGameVersion_272))
    {
        spriteslot = -1;
        if (blank_mouse_cursor == nullptr)
            blank_mouse_cursor.reset(BitmapHelper::CreateTransparentBitmap(1, 1, game.GetColorDepth()));
        mouse_cur_bmp = blank_mouse_cursor.get();
    }
    else
    {
        mouse_cur_bmp = spriteset[spriteslot];
    }

    mouse_cur_pic = spriteslot;
    alpha_blend_cursor = (spriteslot >= 0) ?
        ((game.SpriteInfos[spriteslot].Flags & SPF_ALPHACHANNEL) != 0) : false;
    update_cached_mouse_cursor(mouse_cur_bmp);
}

bool is_standard_cursor_enabled(int curs) {
    if ((game.mcurs[curs].flags & MCF_DISABLED) == 0) {
        // inventory cursor, and they have an active item
        if (curs == MODE_USE)
        {
            if (playerchar->activeinv > 0)
                return true;
        }
        // standard cursor that's not disabled, go with it
        else if (game.mcurs[curs].flags & MCF_STANDARD)
            return true;
    }
    return false;
}

int find_next_enabled_cursor(int startwith) {
    if (startwith >= game.numcursors)
        startwith = 0;
    int testing=startwith;
    do {
        if (is_standard_cursor_enabled(testing)) break;
        testing++;
        if (testing >= game.numcursors) testing=0;
    } while (testing!=startwith);

    if (testing!=startwith)
        set_cursor_mode(testing);

    return testing;
}

int find_previous_enabled_cursor(int startwith) {
    if (startwith < 0)
        startwith = game.numcursors - 1;
    int testing = startwith;
    do {
        if (is_standard_cursor_enabled(testing)) break;
        testing--;
        if (testing < 0) testing = game.numcursors - 1;
    } while (testing != startwith);
    
    if (testing != startwith)
        set_cursor_mode(testing);

    return testing;
}


//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/global_game.h"

// void  (int curs, int newslot)
RuntimeScriptValue Sc_ChangeCursorGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(ChangeCursorGraphic);
}

// void  (int curs, int x, int y)
RuntimeScriptValue Sc_ChangeCursorHotspot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(ChangeCursorHotspot);
}

// void (int curs, int newview)
RuntimeScriptValue Sc_Mouse_ChangeModeView2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(Mouse_ChangeModeView2);
}

RuntimeScriptValue Sc_Mouse_ChangeModeView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(Mouse_ChangeModeView);
}

// void (int modd)
RuntimeScriptValue Sc_disable_cursor_mode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(disable_cursor_mode);
}

// void (int modd)
RuntimeScriptValue Sc_enable_cursor_mode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(enable_cursor_mode);
}

// int (int curs)
RuntimeScriptValue Sc_Mouse_GetModeGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Mouse_GetModeGraphic);
}

// int (int which)
RuntimeScriptValue Sc_IsButtonDown(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsButtonDown);
}

// int (int which)
RuntimeScriptValue Sc_IsModeEnabled(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsModeEnabled);
}

// void ();
RuntimeScriptValue Sc_SaveCursorForLocationChange(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(SaveCursorForLocationChange);
}

// void  ()
RuntimeScriptValue Sc_SetNextCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(SetNextCursor);
}

// void  ()
RuntimeScriptValue Sc_SetPreviousCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(SetPreviousCursor);
}

// void  (int x1, int y1, int x2, int y2)
RuntimeScriptValue Sc_SetMouseBounds(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetMouseBounds);
}

// void  (int newx, int newy)
RuntimeScriptValue Sc_SetMousePosition(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetMousePosition);
}

// void ()
RuntimeScriptValue Sc_RefreshMouse(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(RefreshMouse);
}

// void ()
RuntimeScriptValue Sc_set_default_cursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(set_default_cursor);
}

// void (int newcurs)
RuntimeScriptValue Sc_set_mouse_cursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(set_mouse_cursor);
}

// int ()
RuntimeScriptValue Sc_GetCursorMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetCursorMode);
}

// void (int newmode)
RuntimeScriptValue Sc_set_cursor_mode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(set_cursor_mode);
}

// int ()
RuntimeScriptValue Sc_Mouse_GetVisible(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Mouse_GetVisible);
}

// void (int isOn)
RuntimeScriptValue Sc_Mouse_SetVisible(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Mouse_SetVisible);
}

RuntimeScriptValue Sc_Mouse_Click(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SimulateMouseClick);
}

RuntimeScriptValue Sc_Mouse_GetControlEnabled(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Mouse::IsControlEnabled);
}

RuntimeScriptValue Sc_Mouse_SetControlEnabled(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PBOOL(Mouse_EnableControl);
}

RuntimeScriptValue Sc_Mouse_GetAutoLock(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Mouse_GetAutoLock);
}

RuntimeScriptValue Sc_Mouse_SetAutoLock(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PBOOL(Mouse_SetAutoLock);
}


RuntimeScriptValue Sc_Mouse_GetSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT(Mouse::GetSpeed);
}

RuntimeScriptValue Sc_Mouse_SetSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_PARAM_COUNT("Mouse::Speed", 1);
    Mouse::SetSpeed(params[0].FValue);
    return RuntimeScriptValue();
}

void RegisterMouseAPI()
{
    ScFnRegister mouse_api[] = {
        { "Mouse::ChangeModeGraphic^2",       API_FN_PAIR(ChangeCursorGraphic) },
        { "Mouse::ChangeModeHotspot^3",       API_FN_PAIR(ChangeCursorHotspot) },
        { "Mouse::ChangeModeView^2",          API_FN_PAIR(Mouse_ChangeModeView2) },
        { "Mouse::ChangeModeView^3",          API_FN_PAIR(Mouse_ChangeModeView) },
        { "Mouse::Click^1",                   Sc_Mouse_Click, SimulateMouseClick },
        { "Mouse::DisableMode^1",             API_FN_PAIR(disable_cursor_mode) },
        { "Mouse::EnableMode^1",              API_FN_PAIR(enable_cursor_mode) },
        { "Mouse::GetModeGraphic^1",          API_FN_PAIR(Mouse_GetModeGraphic) },
        { "Mouse::IsButtonDown^1",            API_FN_PAIR(IsButtonDown) },
        { "Mouse::IsModeEnabled^1",           API_FN_PAIR(IsModeEnabled) },
        { "Mouse::SaveCursorUntilItLeaves^0", API_FN_PAIR(SaveCursorForLocationChange) },
        { "Mouse::SelectNextMode^0",          API_FN_PAIR(SetNextCursor) },
        { "Mouse::SelectPreviousMode^0",      API_FN_PAIR(SetPreviousCursor) },
        { "Mouse::SetBounds^4",               API_FN_PAIR(SetMouseBounds) },
        { "Mouse::SetPosition^2",             API_FN_PAIR(SetMousePosition) },
        { "Mouse::Update^0",                  API_FN_PAIR(RefreshMouse) },
        { "Mouse::UseDefaultGraphic^0",       API_FN_PAIR(set_default_cursor) },
        { "Mouse::UseModeGraphic^1",          API_FN_PAIR(set_mouse_cursor) },
        { "Mouse::get_AutoLock",              API_FN_PAIR(Mouse_GetAutoLock) },
        { "Mouse::set_AutoLock",              API_FN_PAIR(Mouse_SetAutoLock) },
        { "Mouse::get_ControlEnabled",        Sc_Mouse_GetControlEnabled, Mouse::IsControlEnabled },
        { "Mouse::set_ControlEnabled",        Sc_Mouse_SetControlEnabled, Mouse_EnableControl },
        { "Mouse::get_Mode",                  API_FN_PAIR(GetCursorMode) },
        { "Mouse::set_Mode",                  API_FN_PAIR(set_cursor_mode) },
        { "Mouse::get_Speed",                 Sc_Mouse_GetSpeed, Mouse::GetSpeed },
        { "Mouse::set_Speed",                 Sc_Mouse_SetSpeed, Mouse::SetSpeed },
        { "Mouse::get_Visible",               API_FN_PAIR(Mouse_GetVisible) },
        { "Mouse::set_Visible",               API_FN_PAIR(Mouse_SetVisible) },
    };

    ccAddExternalFunctions(mouse_api);
}
