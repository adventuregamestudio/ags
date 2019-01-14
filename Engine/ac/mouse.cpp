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

#include "ac/mouse.h"
#include "ac/common.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/dynobj/scriptmouse.h"
#include "ac/dynobj/scriptsystem.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_mouse.h"
#include "ac/global_plugin.h"
#include "ac/global_screen.h"
#include "ac/system.h"
#include "ac/viewframe.h"
#include "debug/debug_log.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "device/mousew32.h"
#include "ac/spritecache.h"
#include "gfx/graphicsdriver.h"
#include "gfx/gfxfilter.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern GameState play;
extern ScriptSystem scsystem;
extern Bitmap *mousecurs[MAXCURSORS];
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern CharacterInfo*playerchar;
extern IGraphicsDriver *gfxDriver;

ScriptMouse scmouse;
int cur_mode,cur_cursor;
int mouse_frame=0,mouse_delay=0;
int lastmx=-1,lastmy=-1;
char alpha_blend_cursor = 0;
Bitmap *dotted_mouse_cursor = NULL;
IDriverDependantBitmap *mouseCursor = NULL;
Bitmap *blank_mouse_cursor = NULL;

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

void SetMouseBounds (int x1, int y1, int x2, int y2) {
    if ((x1 == 0) && (y1 == 0) && (x2 == 0) && (y2 == 0)) {
        x2 = BASEWIDTH-1;
        y2 = MOUSE_MAX_Y - 1;
    }
    if (x2 == BASEWIDTH) x2 = BASEWIDTH-1;
    if (y2 == MOUSE_MAX_Y) y2 = MOUSE_MAX_Y - 1;
    if ((x1 > x2) || (y1 > y2) || (x1 < 0) || (x2 >= BASEWIDTH) ||
        (y1 < 0) || (y2 >= MOUSE_MAX_Y))
        quit("!SetMouseBounds: invalid co-ordinates, must be within (0,0) - (320,200)");
    debug_script_log("Mouse bounds constrained to (%d,%d)-(%d,%d)", x1, y1, x2, y2);

    play.mboundx1 = x1;
    play.mboundx2 = x2;
    play.mboundy1 = y1;
    play.mboundy2 = y2;
    Mouse::SetMoveLimit(Rect(x1, y1, x2, y2));
}

// mouse cursor functions:
// set_mouse_cursor: changes visual appearance to specified cursor
void set_mouse_cursor(int newcurs) {
    int hotspotx = game.mcurs[newcurs].hotx, hotspoty = game.mcurs[newcurs].hoty;

    set_new_cursor_graphic(game.mcurs[newcurs].pic);
    delete dotted_mouse_cursor;
    dotted_mouse_cursor = NULL;

    if ((newcurs == MODE_USE) && (game.mcurs[newcurs].pic > 0) &&
        ((game.hotdot > 0) || (game.invhotdotsprite > 0)) ) {
            // If necessary, create a copy of the cursor and put the hotspot
            // dot onto it
            dotted_mouse_cursor = BitmapHelper::CreateBitmapCopy(mousecurs[0]);

            if (game.invhotdotsprite > 0) {
                //Bitmap *abufWas = abuf;
                //abuf = dotted_mouse_cursor;

                draw_sprite_slot_support_alpha(dotted_mouse_cursor,
                    (game.spriteflags[game.mcurs[newcurs].pic] & SPF_ALPHACHANNEL) != 0,
                    hotspotx - spritewidth[game.invhotdotsprite] / 2,
                    hotspoty - spriteheight[game.invhotdotsprite] / 2,
                    game.invhotdotsprite);

                //abuf = abufWas;
            }
            else {
                putpixel_compensate (dotted_mouse_cursor, hotspotx, hotspoty,
                    (dotted_mouse_cursor->GetColorDepth() > 8) ? GetVirtualScreen()->GetCompatibleColor (game.hotdot) : game.hotdot);

                if (game.hotdotouter > 0) {
                    int outercol = game.hotdotouter;
                    if (dotted_mouse_cursor->GetColorDepth () > 8)
                        outercol = GetVirtualScreen()->GetCompatibleColor(game.hotdotouter);

                    putpixel_compensate (dotted_mouse_cursor, hotspotx + 1, hotspoty, outercol);
                    putpixel_compensate (dotted_mouse_cursor, hotspotx, hotspoty + 1, outercol);
                    putpixel_compensate (dotted_mouse_cursor, hotspotx - 1, hotspoty, outercol);
                    putpixel_compensate (dotted_mouse_cursor, hotspotx, hotspoty - 1, outercol);
                }
            }
            mousecurs[0] = dotted_mouse_cursor;
            update_cached_mouse_cursor();
    }
    msethotspot(hotspotx, hotspoty);
    if (newcurs != cur_cursor)
    {
        cur_cursor = newcurs;
        mouse_frame=0;
        mouse_delay=0;
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
    spriteset.precache (newslot);
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
    game.mcurs[curs].hotx = x;
    game.mcurs[curs].hoty = y;
    if (curs == cur_cursor)
        set_mouse_cursor (cur_cursor);
}

void Mouse_ChangeModeView(int curs, int newview) {
    if ((curs < 0) || (curs >= game.numcursors))
        quit("!Mouse.ChangeModeView: invalid mouse cursor");

    newview--;

    game.mcurs[curs].view = newview;

    if (newview >= 0)
    {
        precache_view(newview);
    }

    if (curs == cur_cursor)
        mouse_delay = 0;  // force update
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

    guis_need_update = 1;
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
        for (ww=0;ww<guis[uu].ControlCount;ww++) {
            if ((guis[uu].CtrlRefs[ww] >> 16)!=kGUIButton) continue;
            GUIButton*gbpt=(GUIButton*)guis[uu].Controls[ww];
            if (gbpt->ClickAction[kMouseLeft]!=kGUIAction_SetMode) continue;
            if (gbpt->ClickData[kMouseLeft]!=modd) continue;
            gbpt->Enable();
        }
    }
    guis_need_update = 1;
}

void disable_cursor_mode(int modd) {
    game.mcurs[modd].flags|=MCF_DISABLED;
    // now search the interfaces for related buttons to kill
    int uu,ww;

    for (uu=0;uu<game.numgui;uu++) {
        for (ww=0;ww<guis[uu].ControlCount;ww++) {
            if ((guis[uu].CtrlRefs[ww] >> 16)!=kGUIButton) continue;
            GUIButton*gbpt=(GUIButton*)guis[uu].Controls[ww];
            if (gbpt->ClickAction[kMouseLeft]!=kGUIAction_SetMode) continue;
            if (gbpt->ClickData[kMouseLeft]!=modd) continue;
            gbpt->Disable();
        }
    }
    if (cur_mode==modd) find_next_enabled_cursor(modd);
    guis_need_update = 1;
}

void RefreshMouse() {
    domouse(DOMOUSE_NOCURSOR);
    scmouse.x = mousex;
    scmouse.y = mousey;
}

void SetMousePosition (int newx, int newy) {
    if (newx < 0)
        newx = 0;
    if (newy < 0)
        newy = 0;
    if (newx >= BASEWIDTH)
        newx = BASEWIDTH - 1;
    if (newy >= BASEHEIGHT)
        newy = BASEHEIGHT - 1;

    Mouse::SetPosition(Point(newx, newy));
    RefreshMouse();
}

int GetCursorMode() {
    return cur_mode;
}

int IsButtonDown(int which) {
    if ((which < 1) || (which > 3))
        quit("!IsButtonDown: only works with eMouseLeft, eMouseRight, eMouseMiddle");
    if (misbuttondown(which-1))
        return 1;
    return 0;
}

int IsModeEnabled(int which) {
    return (which < 0) || (which >= game.numcursors) ? 0 :
        which == MODE_USE ? playerchar->activeinv > 0 :
        (game.mcurs[which].flags & MCF_DISABLED) == 0;
}

//=============================================================================

int GetMouseCursor() {
    return cur_cursor;
}

void update_script_mouse_coords() {
    scmouse.x = mousex;
    scmouse.y = mousey;
}

void update_inv_cursor(int invnum) {

    if ((game.options[OPT_FIXEDINVCURSOR]==0) && (invnum > 0)) {
        int cursorSprite = game.invinfo[invnum].cursorPic;

        // Fall back to the inventory pic if no cursor pic is defined.
        if (cursorSprite == 0)
            cursorSprite = game.invinfo[invnum].pic;

        game.mcurs[MODE_USE].pic = cursorSprite;
        // all cursor images must be pre-cached
        spriteset.precache(cursorSprite);

        if ((game.invinfo[invnum].hotx > 0) || (game.invinfo[invnum].hoty > 0)) {
            // if the hotspot was set (unfortunately 0,0 isn't a valid co-ord)
            game.mcurs[MODE_USE].hotx=game.invinfo[invnum].hotx;
            game.mcurs[MODE_USE].hoty=game.invinfo[invnum].hoty;
        }
        else {
            game.mcurs[MODE_USE].hotx = spritewidth[cursorSprite] / 2;
            game.mcurs[MODE_USE].hoty = spriteheight[cursorSprite] / 2;
        }
    }
}

void update_cached_mouse_cursor() 
{
    if (mouseCursor != NULL)
        gfxDriver->DestroyDDB(mouseCursor);
    mouseCursor = gfxDriver->CreateDDBFromBitmap(mousecurs[0], alpha_blend_cursor != 0);
}

void set_new_cursor_graphic (int spriteslot) {
    mousecurs[0] = spriteset[spriteslot];

    // It looks like spriteslot 0 can be used in games with version 2.72 and lower.
    // The NULL check should ensure that the sprite is valid anyway.
    if ((spriteslot < 1)  || (mousecurs[0] == NULL))
    {
        if (blank_mouse_cursor == NULL)
        {
            blank_mouse_cursor = BitmapHelper::CreateTransparentBitmap(1, 1, System_GetColorDepth());
        }
        mousecurs[0] = blank_mouse_cursor;
    }

    if (game.spriteflags[spriteslot] & SPF_ALPHACHANNEL)
        alpha_blend_cursor = 1;
    else
        alpha_blend_cursor = 0;

    update_cached_mouse_cursor();
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
RuntimeScriptValue Sc_Mouse_ChangeModeView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(Mouse_ChangeModeView);
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
    API_SCALL_VOID_PINT(PluginSimulateMouseClick);
}

RuntimeScriptValue Sc_Mouse_GetControlEnabled(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Mouse::IsControlEnabled);
}

RuntimeScriptValue Sc_Mouse_GetSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT(Mouse::GetSpeed);
}

RuntimeScriptValue Sc_Mouse_SetSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_VARIABLE_VALUE("Mouse::Speed");
    Mouse::SetSpeed(params[0].FValue);
    return RuntimeScriptValue();
}

void RegisterMouseAPI()
{
    ccAddExternalStaticFunction("Mouse::ChangeModeGraphic^2",       Sc_ChangeCursorGraphic);
    ccAddExternalStaticFunction("Mouse::ChangeModeHotspot^3",       Sc_ChangeCursorHotspot);
    ccAddExternalStaticFunction("Mouse::ChangeModeView^2",          Sc_Mouse_ChangeModeView);
    ccAddExternalStaticFunction("Mouse::Click^1",                   Sc_Mouse_Click);
    ccAddExternalStaticFunction("Mouse::DisableMode^1",             Sc_disable_cursor_mode);
    ccAddExternalStaticFunction("Mouse::EnableMode^1",              Sc_enable_cursor_mode);
    ccAddExternalStaticFunction("Mouse::GetModeGraphic^1",          Sc_Mouse_GetModeGraphic);
    ccAddExternalStaticFunction("Mouse::IsButtonDown^1",            Sc_IsButtonDown);
    ccAddExternalStaticFunction("Mouse::IsModeEnabled^1",           Sc_IsModeEnabled);
    ccAddExternalStaticFunction("Mouse::SaveCursorUntilItLeaves^0", Sc_SaveCursorForLocationChange);
    ccAddExternalStaticFunction("Mouse::SelectNextMode^0",          Sc_SetNextCursor);
    ccAddExternalStaticFunction("Mouse::SelectPreviousMode^0",      Sc_SetPreviousCursor);
    ccAddExternalStaticFunction("Mouse::SetBounds^4",               Sc_SetMouseBounds);
    ccAddExternalStaticFunction("Mouse::SetPosition^2",             Sc_SetMousePosition);
    ccAddExternalStaticFunction("Mouse::Update^0",                  Sc_RefreshMouse);
    ccAddExternalStaticFunction("Mouse::UseDefaultGraphic^0",       Sc_set_default_cursor);
    ccAddExternalStaticFunction("Mouse::UseModeGraphic^1",          Sc_set_mouse_cursor);
    ccAddExternalStaticFunction("Mouse::get_ControlEnabled",        Sc_Mouse_GetControlEnabled);
    ccAddExternalStaticFunction("Mouse::get_Mode",                  Sc_GetCursorMode);
    ccAddExternalStaticFunction("Mouse::set_Mode",                  Sc_set_cursor_mode);
    ccAddExternalStaticFunction("Mouse::get_Speed",                 Sc_Mouse_GetSpeed);
    ccAddExternalStaticFunction("Mouse::set_Speed",                 Sc_Mouse_SetSpeed);
    ccAddExternalStaticFunction("Mouse::get_Visible",               Sc_Mouse_GetVisible);
    ccAddExternalStaticFunction("Mouse::set_Visible",               Sc_Mouse_SetVisible);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("Mouse::ChangeModeGraphic^2",       (void*)ChangeCursorGraphic);
    ccAddExternalFunctionForPlugin("Mouse::ChangeModeHotspot^3",       (void*)ChangeCursorHotspot);
    ccAddExternalFunctionForPlugin("Mouse::ChangeModeView^2",          (void*)Mouse_ChangeModeView);
    ccAddExternalFunctionForPlugin("Mouse::DisableMode^1",             (void*)disable_cursor_mode);
    ccAddExternalFunctionForPlugin("Mouse::EnableMode^1",              (void*)enable_cursor_mode);
    ccAddExternalFunctionForPlugin("Mouse::GetModeGraphic^1",          (void*)Mouse_GetModeGraphic);
    ccAddExternalFunctionForPlugin("Mouse::IsButtonDown^1",            (void*)IsButtonDown);
    ccAddExternalFunctionForPlugin("Mouse::IsModeEnabled^1",           (void*)IsModeEnabled);
    ccAddExternalFunctionForPlugin("Mouse::SaveCursorUntilItLeaves^0", (void*)SaveCursorForLocationChange);
    ccAddExternalFunctionForPlugin("Mouse::SelectNextMode^0",          (void*)SetNextCursor);
    ccAddExternalFunctionForPlugin("Mouse::SelectPreviousMode^0",      (void*)SetPreviousCursor);
    ccAddExternalFunctionForPlugin("Mouse::SetBounds^4",               (void*)SetMouseBounds);
    ccAddExternalFunctionForPlugin("Mouse::SetPosition^2",             (void*)SetMousePosition);
    ccAddExternalFunctionForPlugin("Mouse::Update^0",                  (void*)RefreshMouse);
    ccAddExternalFunctionForPlugin("Mouse::UseDefaultGraphic^0",       (void*)set_default_cursor);
    ccAddExternalFunctionForPlugin("Mouse::UseModeGraphic^1",          (void*)set_mouse_cursor);
    ccAddExternalFunctionForPlugin("Mouse::get_Mode",                  (void*)GetCursorMode);
    ccAddExternalFunctionForPlugin("Mouse::set_Mode",                  (void*)set_cursor_mode);
    ccAddExternalFunctionForPlugin("Mouse::get_Visible",               (void*)Mouse_GetVisible);
    ccAddExternalFunctionForPlugin("Mouse::set_Visible",               (void*)Mouse_SetVisible);
}
