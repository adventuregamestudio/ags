//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/global_screen.h"
#include "ac/screen.h"
#include "ac/dynobj/scriptviewport.h"
#include "ac/dynobj/scriptuserobject.h"
#include "debug/debug_log.h"
#include "script/script_runtime.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin_evts.h"
#include "plugin/plugin_engine.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern GameState play;
extern IGraphicsDriver *gfxDriver;
extern AGSPlatformDriver *platform;
extern int displayed_room;

void fadein_impl(PALETTE p, int speed) {
    if (game.color_depth > 1) {
        set_palette (p);

        play.screen_is_faded_out = 0;

        if (play.no_hicolor_fadein) {
            return;
        }
    }

    gfxDriver->FadeIn(speed, p, play.fade_to_red, play.fade_to_green, play.fade_to_blue);
}

Bitmap *saved_viewport_bitmap = nullptr;
RGB old_palette[256];
void current_fade_out_effect () {
    debug_script_log("Transition-out in room %d", displayed_room);
    if (pl_run_plugin_hooks(AGSE_TRANSITIONOUT, 0))
        return;

    // get the screen transition type
    int theTransition = play.fade_effect;
    // was a temporary transition selected? if so, use it
    if (play.next_screen_transition >= 0)
        theTransition = play.next_screen_transition;
    const bool ignore_transition = play.screen_tint > 0;

    if ((theTransition == FADE_INSTANT) || ignore_transition) {
        if (!play.keep_screen_during_instant_transition)
            set_palette_range(black_palette, 0, 255, 0);
    }
    else if (theTransition == FADE_NORMAL)
    {
        fadeout_impl(5);
    }
    else if (theTransition == FADE_BOXOUT) 
    {
        gfxDriver->BoxOutEffect(true, 16, 1000 / GetGameSpeed());
        play.screen_is_faded_out = 1;
    }
    else 
    {
        get_palette(old_palette);
        const Rect &viewport = play.GetMainViewport();
        saved_viewport_bitmap = CopyScreenIntoBitmap(viewport.GetWidth(), viewport.GetHeight());
    }
}

IDriverDependantBitmap* prepare_screen_for_transition_in()
{
    if (saved_viewport_bitmap == nullptr)
        quit("Crossfade: buffer is null attempting transition");

    const Rect &viewport = play.GetMainViewport();
    if (saved_viewport_bitmap->GetHeight() < viewport.GetHeight())
    {
        Bitmap *enlargedBuffer = BitmapHelper::CreateBitmap(saved_viewport_bitmap->GetWidth(), viewport.GetHeight(), saved_viewport_bitmap->GetColorDepth());
        enlargedBuffer->Blit(saved_viewport_bitmap, 0, 0, 0, (viewport.GetHeight() - saved_viewport_bitmap->GetHeight()) / 2, saved_viewport_bitmap->GetWidth(), saved_viewport_bitmap->GetHeight());
        delete saved_viewport_bitmap;
        saved_viewport_bitmap = enlargedBuffer;
    }
    else if (saved_viewport_bitmap->GetHeight() > viewport.GetHeight())
    {
        Bitmap *clippedBuffer = BitmapHelper::CreateBitmap(saved_viewport_bitmap->GetWidth(), viewport.GetHeight(), saved_viewport_bitmap->GetColorDepth());
        clippedBuffer->Blit(saved_viewport_bitmap, 0, (saved_viewport_bitmap->GetHeight() - viewport.GetHeight()) / 2, 0, 0, saved_viewport_bitmap->GetWidth(), saved_viewport_bitmap->GetHeight());
        delete saved_viewport_bitmap;
        saved_viewport_bitmap = clippedBuffer;
    }
    return gfxDriver->CreateDDBFromBitmap(saved_viewport_bitmap, true);
}

//=============================================================================
//
// Screen script API.
//
//=============================================================================

int Screen_GetScreenWidth()
{
    return game.GetGameRes().Width;
}

int Screen_GetScreenHeight()
{
    return game.GetGameRes().Height;
}

bool Screen_GetAutoSizeViewport()
{
    return play.IsAutoRoomViewport();
}

void Screen_SetAutoSizeViewport(bool on)
{
    play.SetAutoRoomViewport(on);
}

ScriptViewport* Screen_GetViewport()
{
    return play.GetScriptViewport(0);
}

int Screen_GetViewportCount()
{
    return play.GetRoomViewportCount();
}

ScriptViewport* Screen_GetAnyViewport(int index)
{
    return play.GetScriptViewport(index);
}

ScriptUserObject* Screen_ScreenToRoomPoint(int scrx, int scry, bool restrict)
{
    VpPoint vpt = play.ScreenToRoom(scrx, scry, restrict);
    if (vpt.second < 0)
        return nullptr;
    return ScriptStructHelpers::CreatePoint(vpt.first.X, vpt.first.Y);
}

ScriptUserObject* Screen_ScreenToRoomPoint2(int scrx, int scry)
{
    return Screen_ScreenToRoomPoint(scrx, scry, true);
}

ScriptUserObject *Screen_RoomToScreenPoint(int roomx, int roomy)
{
    Point pt = play.RoomToScreen(roomx, roomy);
    return ScriptStructHelpers::CreatePoint(pt.X, pt.Y);
}

RuntimeScriptValue Sc_Screen_GetScreenHeight(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Screen_GetScreenHeight);
}

RuntimeScriptValue Sc_Screen_GetScreenWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Screen_GetScreenWidth);
}

RuntimeScriptValue Sc_Screen_GetAutoSizeViewport(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Screen_GetAutoSizeViewport);
}

RuntimeScriptValue Sc_Screen_SetAutoSizeViewport(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PBOOL(Screen_SetAutoSizeViewport);
}

RuntimeScriptValue Sc_Screen_GetViewport(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptViewport, Screen_GetViewport);
}

RuntimeScriptValue Sc_Screen_GetViewportCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Screen_GetViewportCount);
}

RuntimeScriptValue Sc_Screen_GetAnyViewport(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptViewport, Screen_GetAnyViewport);
}

RuntimeScriptValue Sc_Screen_ScreenToRoomPoint2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptUserObject, Screen_ScreenToRoomPoint2);
}

RuntimeScriptValue Sc_Screen_ScreenToRoomPoint(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2_PBOOL(ScriptUserObject, Screen_ScreenToRoomPoint);
}

RuntimeScriptValue Sc_Screen_RoomToScreenPoint(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptUserObject, Screen_RoomToScreenPoint);
}

void RegisterScreenAPI()
{
    ScFnRegister screen_api[] = {
        { "Screen::get_Height",             API_FN_PAIR(Screen_GetScreenHeight) },
        { "Screen::get_Width",              API_FN_PAIR(Screen_GetScreenWidth) },
        { "Screen::get_AutoSizeViewportOnRoomLoad", API_FN_PAIR(Screen_GetAutoSizeViewport) },
        { "Screen::set_AutoSizeViewportOnRoomLoad", API_FN_PAIR(Screen_SetAutoSizeViewport) },
        { "Screen::get_Viewport",           API_FN_PAIR(Screen_GetViewport) },
        { "Screen::get_ViewportCount",      API_FN_PAIR(Screen_GetViewportCount) },
        { "Screen::geti_Viewports",         API_FN_PAIR(Screen_GetAnyViewport) },
        { "Screen::ScreenToRoomPoint^2",    API_FN_PAIR(Screen_ScreenToRoomPoint2) },
        { "Screen::ScreenToRoomPoint^3",    API_FN_PAIR(Screen_ScreenToRoomPoint) },
        { "Screen::RoomToScreenPoint",      API_FN_PAIR(Screen_RoomToScreenPoint) },
    };

    ccAddExternalFunctions(screen_api);
}
