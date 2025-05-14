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
#include "ac/mouse.h"
#include "ac/common.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/dynobj/scriptmouse.h"
#include "ac/dynobj/scriptsystem.h"
#include "ac/dynobj/scriptshader.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/global_screen.h"
#include "ac/gui.h"
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
extern ScriptSystem scsystem;
extern SpriteCache spriteset;
extern CharacterInfo*playerchar;
extern IGraphicsDriver *gfxDriver;

ScriptMouse scmouse;
int cur_mode,cur_cursor;
int mouse_hotx = 0, mouse_hoty = 0; // in game cursor hotspot offset
int mouse_frame=0,mouse_delay=0;
int lastmx=-1,lastmy=-1;

CursorGraphicState cursor_gstate;


Bitmap *CursorGraphicState::GetImage() const
{
    return _genImage != nullptr ? _genImage.get() : spriteset[_sprnum];
}

void CursorGraphicState::SetImage(std::unique_ptr<Common::Bitmap> pic)
{
    _genImage = std::move(pic);
    _sprnum = -1;
    MarkChanged();
}

void CursorGraphicState::SetSpriteNum(int sprnum)
{
    _sprnum = sprnum;
    _genImage.reset();
    MarkChanged();
}


// The Mouse:: functions are static so the script doesn't pass
// in an object parameter
void Mouse_SetVisible(int isOn) {
    if (isOn)
        play.mouse_cursor_hidden = 0;
    else
        play.mouse_cursor_hidden = 1;
}

int Mouse_GetVisible() {
    if (play.mouse_cursor_hidden)
        return 0;
    return 1;
}

void Mouse_SetBounds(int x1, int y1, int x2, int y2)
{
    int xmax = play.GetMainViewport().GetWidth() - 1;
    int ymax = play.GetMainViewport().GetHeight() - 1;
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

    play.mbounds = Rect(x1, y1, x2, y2);
    Mouse::SetMoveLimit(play.mbounds);
}

// set_mouse_cursor: changes visual appearance to specified cursor
void set_mouse_cursor(int newcurs, bool force_update)
{
    const int hotspotx = game.mcurs[newcurs].hotx, hotspoty = game.mcurs[newcurs].hoty;
    mouse_hotx = hotspotx, mouse_hoty = hotspoty;

    // if it's same cursor and there's animation in progress, then don't assign a new pic just yet
    if (!force_update &&
        newcurs == cur_cursor && game.mcurs[newcurs].view >= 0 &&
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
    const int cur_pic = game.mcurs[newcurs].pic;
    set_new_cursor_graphic(cur_pic);

    // If it's inventory cursor, draw hotspot crosshair sprite upon it
    if ((newcurs == MODE_USE) && (cur_pic > 0) &&
        ((game.hotdot > 0) || (game.invhotdotsprite > 0)) )
    {
        // If necessary, create a copy of the cursor and put the hotspot dot onto it
        std::unique_ptr<Bitmap> gen_cursor(BitmapHelper::CreateBitmapCopy(spriteset[cur_pic]));

        if (game.invhotdotsprite > 0)
        {
            draw_sprite_slot_support_alpha(gen_cursor.get(),
                hotspotx - game.SpriteInfos[game.invhotdotsprite].Width / 2,
                hotspoty - game.SpriteInfos[game.invhotdotsprite].Height / 2,
                game.invhotdotsprite);
        }
        else
        {
            gen_cursor->PutPixel(hotspotx, hotspoty, MakeColor(game.hotdot));
            if (game.hotdotouter > 0)
            {
                const int outercol = MakeColor(game.hotdotouter);
                gen_cursor->PutPixel(hotspotx + 1, hotspoty, outercol);
                gen_cursor->PutPixel(hotspotx, hotspoty + 1, outercol);
                gen_cursor->PutPixel(hotspotx - 1, hotspoty, outercol);
                gen_cursor->PutPixel(hotspotx, hotspoty - 1, outercol);
            }
        }

        cursor_gstate.SetImage(std::move(gen_cursor));
    }
}

// set_default_cursor: resets visual appearance to current mode (walk, look, etc)
void set_default_cursor() {
    set_mouse_cursor(cur_mode);
}

int Mouse_GetCursor()
{
    return cur_cursor;
}

void Mouse_SetCursor(int newcurs)
{
    set_mouse_cursor(newcurs);
}

void Mouse_SetDefaultCursor()
{
    set_default_cursor();
}

// permanently change cursor graphic
void Mouse_ChangeCursorGraphic (int curs, int newslot) {
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

void Mouse_ChangeCursorHotspot (int curs, int x, int y) {
    if ((curs < 0) || (curs >= game.numcursors))
        quit("!ChangeCursorHotspot: invalid mouse cursor");
    game.mcurs[curs].hotx = x;
    game.mcurs[curs].hoty = y;
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

void Mouse_SetNextCursor () {
    set_cursor_mode (find_next_enabled_cursor(cur_mode + 1));
}

void Mouse_SetPreviousCursor() {
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

void Mouse_SetCursorMode(int newmode)
{
    set_cursor_mode(newmode);
}

void enable_cursor_mode(int modd) {
    game.mcurs[modd].flags&=~MCF_DISABLED;
    // now search the interfaces for related buttons to re-enable
    int uu,ww;

    for (uu=0;uu<game.numgui;uu++) {
        for (ww=0;ww<guis[uu].GetControlCount();ww++) {
            if (guis[uu].GetControlType(ww) != kGUIButton) continue;
            GUIButton*gbpt=(GUIButton*)guis[uu].GetControl(ww);
            if (gbpt->GetClickAction(kGUIClickLeft)!=kGUIAction_SetMode) continue;
            if (gbpt->GetClickData(kGUIClickLeft)!=modd) continue;
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
            if (gbpt->GetClickAction(kGUIClickLeft)!=kGUIAction_SetMode) continue;
            if (gbpt->GetClickData(kGUIClickLeft)!=modd) continue;
            gbpt->SetEnabled(false);
        }
    }
    if (cur_mode==modd) find_next_enabled_cursor(modd);
}

void Mouse_EnableCursorMode(int mode)
{
    enable_cursor_mode(mode);
}

void Mouse_DisableCursorMode(int mode)
{
    disable_cursor_mode(mode);
}

void Mouse_Refresh() {
    ags_domouse();
    scmouse.x = mousex;
    scmouse.y = mousey;
}

void Mouse_SetPosition (int newx, int newy) {
    const Rect &viewport = play.GetMainViewport();

    if (newx < 0)
        newx = 0;
    if (newy < 0)
        newy = 0;
    if (newx >= viewport.GetWidth())
        newx = viewport.GetWidth() - 1;
    if (newy >= viewport.GetHeight())
        newy = viewport.GetHeight() - 1;

    Mouse::SetPosition(Point(newx, newy));
    Mouse_Refresh();
}

int Mouse_GetCursorMode() {
    return cur_mode;
}

int Mouse_IsButtonDown(int which) {
    if ((which < kMouseLeft) || (which > kMouseMiddle))
        quit("!IsButtonDown: only works with eMouseLeft, eMouseRight, eMouseMiddle");
    return ags_misbuttondown(static_cast<eAGSMouseButton>(which)) ? 1 : 0;
}

int Mouse_IsModeEnabled(int which) {
    return (which < 0) || (which >= game.numcursors) ? 0 :
        which == MODE_USE ? playerchar->activeinv > 0 :
        (game.mcurs[which].flags & MCF_DISABLED) == 0;
}

void Mouse_SaveCursorForLocationChange() {
    // update the current location name
    GetLocationName(mousex, mousey);

    if (play.get_loc_name_save_cursor != play.get_loc_name_last_time) {
        play.get_loc_name_save_cursor = play.get_loc_name_last_time;
        play.restore_cursor_mode_to = Mouse_GetCursorMode();
        play.restore_cursor_image_to = Mouse_GetCursor();
        debug_script_log("Saving mouse: mode %d cursor %d", play.restore_cursor_mode_to, play.restore_cursor_image_to);
    }
}

void Mouse_SimulateClick(int button_id) {
    ags_simulate_mouseclick(static_cast<eAGSMouseButton>(button_id));
}

void Mouse_EnableControl(bool on)
{
    bool should_control_mouse =
        usetup.MouseCtrlWhen == kMouseCtrl_Always ||
        (usetup.MouseCtrlWhen == kMouseCtrl_Fullscreen && (scsystem.windowed == 0));
    Mouse::SetMovementControl(should_control_mouse & on);
    usetup.MouseCtrlEnabled = on; // remember setting in config
}

bool Mouse_GetAutoLock()
{
    return usetup.MouseAutoLock;
}

void Mouse_SetAutoLock(bool on)
{
    usetup.MouseAutoLock = on;
    // Only update when in windowed mode, as always locked in fullscreen
    if (scsystem.windowed)
    {
        if (usetup.MouseAutoLock)
            Mouse::TryLockToWindow();
        else
            Mouse::UnlockFromWindow();
    }
}

ScriptShaderInstance *Mouse_GetCursorShader()
{
    return static_cast<ScriptShaderInstance *>(ccGetObjectAddressFromHandle(
        play.GetCursorShaderHandle()));
}

void Mouse_SetCursorShader(ScriptShaderInstance *shader_inst)
{
    play.SetCursorShader(shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID,
                    ccReplaceObjectHandle(play.GetCursorShaderHandle(), shader_inst));
}

//=============================================================================

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

void set_new_cursor_graphic(int spriteslot)
{
    if (spriteslot < 1)
    {
        cursor_gstate.SetImage(std::unique_ptr<Bitmap>(BitmapHelper::CreateTransparentBitmap(1, 1, game.GetColorDepth())));
    }
    else
    {
        cursor_gstate.SetSpriteNum(spriteslot);
    }
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
RuntimeScriptValue Sc_Mouse_ChangeCursorGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(Mouse_ChangeCursorGraphic);
}

// void  (int curs, int x, int y)
RuntimeScriptValue Sc_Mouse_ChangeCursorHotspot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(Mouse_ChangeCursorHotspot);
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
RuntimeScriptValue Sc_Mouse_DisableCursorMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Mouse_DisableCursorMode);
}

// void (int modd)
RuntimeScriptValue Sc_Mouse_EnableCursorMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Mouse_EnableCursorMode);
}

// int (int curs)
RuntimeScriptValue Sc_Mouse_GetModeGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Mouse_GetModeGraphic);
}

// int (int which)
RuntimeScriptValue Sc_Mouse_IsButtonDown(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Mouse_IsButtonDown);
}

// int (int which)
RuntimeScriptValue Sc_Mouse_IsModeEnabled(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Mouse_IsModeEnabled);
}

// void ();
RuntimeScriptValue Sc_Mouse_SaveCursorForLocationChange(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Mouse_SaveCursorForLocationChange);
}

// void  ()
RuntimeScriptValue Sc_Mouse_SetNextCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Mouse_SetNextCursor);
}

// void  ()
RuntimeScriptValue Sc_Mouse_SetPreviousCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Mouse_SetPreviousCursor);
}

// void  (int x1, int y1, int x2, int y2)
RuntimeScriptValue Sc_Mouse_SetBounds(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(Mouse_SetBounds);
}

// void  (int newx, int newy)
RuntimeScriptValue Sc_Mouse_SetPosition(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(Mouse_SetPosition);
}

// void ()
RuntimeScriptValue Sc_Mouse_Refresh(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Mouse_Refresh);
}

// void ()
RuntimeScriptValue Sc_Mouse_SetDefaultCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Mouse_SetDefaultCursor);
}

// void (int newcurs)
RuntimeScriptValue Sc_Mouse_SetCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Mouse_SetCursor);
}

// int ()
RuntimeScriptValue Sc_Mouse_GetCursorMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Mouse_GetCursorMode);
}

// void (int newmode)
RuntimeScriptValue Sc_Mouse_SetCursorMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Mouse_SetCursorMode);
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

RuntimeScriptValue Sc_Mouse_SimulateClick(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Mouse_SimulateClick);
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

RuntimeScriptValue Sc_Mouse_GetCursorShader(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptShaderInstance, Mouse_GetCursorShader);
}

RuntimeScriptValue Sc_Mouse_SetCursorShader(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(Mouse_SetCursorShader, ScriptShaderInstance);
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
        { "Mouse::ChangeModeGraphic^2",       API_FN_PAIR(Mouse_ChangeCursorGraphic) },
        { "Mouse::ChangeModeHotspot^3",       API_FN_PAIR(Mouse_ChangeCursorHotspot) },
        { "Mouse::ChangeModeView^2",          API_FN_PAIR(Mouse_ChangeModeView2) },
        { "Mouse::ChangeModeView^3",          API_FN_PAIR(Mouse_ChangeModeView) },
        { "Mouse::Click^1",                   API_FN_PAIR(Mouse_SimulateClick) },
        { "Mouse::DisableMode^1",             API_FN_PAIR(Mouse_DisableCursorMode) },
        { "Mouse::EnableMode^1",              API_FN_PAIR(Mouse_EnableCursorMode) },
        { "Mouse::GetModeGraphic^1",          API_FN_PAIR(Mouse_GetModeGraphic) },
        { "Mouse::IsButtonDown^1",            API_FN_PAIR(Mouse_IsButtonDown) },
        { "Mouse::IsModeEnabled^1",           API_FN_PAIR(Mouse_IsModeEnabled) },
        { "Mouse::SaveCursorUntilItLeaves^0", API_FN_PAIR(Mouse_SaveCursorForLocationChange) },
        { "Mouse::SelectNextMode^0",          API_FN_PAIR(Mouse_SetNextCursor) },
        { "Mouse::SelectPreviousMode^0",      API_FN_PAIR(Mouse_SetPreviousCursor) },
        { "Mouse::SetBounds^4",               API_FN_PAIR(Mouse_SetBounds) },
        { "Mouse::SetPosition^2",             API_FN_PAIR(Mouse_SetPosition) },
        { "Mouse::Update^0",                  API_FN_PAIR(Mouse_Refresh) },
        { "Mouse::UseDefaultGraphic^0",       API_FN_PAIR(Mouse_SetDefaultCursor) },
        { "Mouse::UseModeGraphic^1",          API_FN_PAIR(Mouse_SetCursor) },
        { "Mouse::get_AutoLock",              API_FN_PAIR(Mouse_GetAutoLock) },
        { "Mouse::set_AutoLock",              API_FN_PAIR(Mouse_SetAutoLock) },
        { "Mouse::get_ControlEnabled",        Sc_Mouse_GetControlEnabled, Mouse::IsControlEnabled },
        { "Mouse::set_ControlEnabled",        Sc_Mouse_SetControlEnabled, Mouse_EnableControl },
        { "Mouse::get_CursorShader",          API_FN_PAIR(Mouse_GetCursorShader) },
        { "Mouse::set_CursorShader",          API_FN_PAIR(Mouse_SetCursorShader) },
        { "Mouse::get_Mode",                  API_FN_PAIR(Mouse_GetCursorMode) },
        { "Mouse::set_Mode",                  API_FN_PAIR(Mouse_SetCursorMode) },
        { "Mouse::get_Speed",                 Sc_Mouse_GetSpeed, Mouse::GetSpeed },
        { "Mouse::set_Speed",                 Sc_Mouse_SetSpeed, Mouse::SetSpeed },
        { "Mouse::get_Visible",               API_FN_PAIR(Mouse_GetVisible) },
        { "Mouse::set_Visible",               API_FN_PAIR(Mouse_SetVisible) },
    };

    ccAddExternalFunctions(mouse_api);
}
