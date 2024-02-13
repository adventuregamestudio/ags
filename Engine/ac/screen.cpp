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
#include <algorithm>
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/global_screen.h"
#include "ac/screen.h"
#include "ac/sys_events.h"
#include "ac/dynobj/scriptviewport.h"
#include "ac/dynobj/scriptuserobject.h"
#include "debug/debug_log.h"
#include "main/game_run.h"
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
extern RGB palette[256];

std::unique_ptr<Bitmap> saved_viewport_bitmap;
RGB old_palette[256];


//-----------------------------------------------------------------------------
// Game screenshot-making functions.
//-----------------------------------------------------------------------------
// Create a DDB and render game screen on it.
// NOTE: for fading-out:
// please keep in mind: redrawing last saved frame here instead of constructing new one
// is done because of backwards-compatibility issue: originally AGS faded out using frame
// drawn before the script that triggers blocking fade (e.g. instigated by ChangeRoom).
// Unfortunately some existing games were changing looks of the screen during same function,
// but these were not supposed to get on screen until before fade-in.
//
// This special fade-out behavior may be deprecated later if wanted.
static IDriverDependantBitmap *game_frame_to_ddb(bool for_fadein)
{
    const auto &view = play.GetMainViewport();
    auto *shot_ddb = gfxDriver->CreateRenderTargetDDB(view.GetWidth(), view.GetHeight(), gfxDriver->GetDisplayMode().ColorDepth);
    if (for_fadein)
    {
        gfxDriver->ClearDrawLists();
        construct_game_scene(true);
        construct_game_screen_overlay(false);
        gfxDriver->Render(shot_ddb);
    }
    else
    {
        gfxDriver->GetCopyOfScreenIntoDDB(shot_ddb);
    }
    return shot_ddb;
}

// Render game screen and copy to bitmap. This is a variant of game_frame_to_ddb,
// meant for the case when this is meant for software drawing.
// Note for "fade-out" from game_frame_to_ddb() applies here too.
static std::unique_ptr<Bitmap> game_frame_to_bmp(bool for_fadein)
{
    // TODO: use screenshot_to_ddb
    get_palette(old_palette);
    const auto &view = play.GetMainViewport();
    if (for_fadein)
    {
        gfxDriver->ClearDrawLists();
        construct_game_scene(true);
        construct_game_screen_overlay(false);
        gfxDriver->RenderToBackBuffer();
    }
    return std::unique_ptr<Bitmap>(
        CopyScreenIntoBitmap(view.GetWidth(), view.GetHeight()));
}

static IDriverDependantBitmap* get_frame_for_transition_in(bool opaque)
{
    assert(saved_viewport_bitmap);
    if (!saved_viewport_bitmap)
        quit("Crossfade: buffer is null attempting transition");

    // Resize the frame in case main viewport changed;
    // this is mostly for compatibility with old-style letterboxed games
    // which could have viewport changed depending on new room size.
    // TODO: investigate if this is still a case.
    const Rect &viewport = play.GetMainViewport();
    if (saved_viewport_bitmap->GetHeight() != viewport.GetHeight())
    {
        Bitmap *fix_frame = BitmapHelper::CreateBitmap(saved_viewport_bitmap->GetWidth(), viewport.GetHeight(), saved_viewport_bitmap->GetColorDepth());
        int y = std::max(0, (saved_viewport_bitmap->GetHeight() - viewport.GetHeight()) / 2);
        fix_frame->Blit(saved_viewport_bitmap.get(), 0, 0, 0, (viewport.GetHeight() - saved_viewport_bitmap->GetHeight()) / 2, saved_viewport_bitmap->GetWidth(), saved_viewport_bitmap->GetHeight());
        saved_viewport_bitmap.reset(fix_frame);
    }
    return gfxDriver->CreateDDBFromBitmap(saved_viewport_bitmap.get(), false, opaque);
}


//-----------------------------------------------------------------------------
// Software fade routines - for 8-bit and 16/32-bit
//-----------------------------------------------------------------------------

static void fade_highcolor(bool do_fadein,
    int speed, int fade_red, int fade_green, int fade_blue)
{
    Bitmap *bmp_buff = gfxDriver->GetMemoryBackBuffer();
    const int col_depth = bmp_buff->GetColorDepth();
    const int clear_col = makecol_depth(col_depth, fade_red, fade_green, fade_blue);
    play.screen_is_faded_out = 0; // force all game elements to draw
    std::unique_ptr<Bitmap> bmp_frame = game_frame_to_bmp(do_fadein);

    for (int a = 0; a < 256; a += speed)
    {
        bmp_buff->Fill(clear_col);
        set_trans_blender(0, 0, 0, do_fadein ? a : 255 - a);
        bmp_buff->TransBlendBlt(bmp_frame.get(), 0, 0);
        render_to_screen();

        sys_evt_process_pending();
        update_polled_stuff();
        WaitForNextFrame();
    }

    render_to_screen();
}

static RGB faded_out_palette[256];
static void fade_256_init(int r, int g, int b)
{
    for (int a = 0; a < 256; a++)
    {
        faded_out_palette[a].r = r / 4;
	    faded_out_palette[a].g = g / 4;
	    faded_out_palette[a].b = b / 4;
    }
}

static void fade_256_in_range(PALETTE source, PALETTE dest, int speed, int from, int to) 
{
    PALETTE temp;
    for (int c = 0; c < PAL_SIZE; c++)
        temp[c] = source[c];

    for (int c=0; c<64; c+=speed)
    {
        fade_interpolate(source, dest, temp, c, from, to);
        set_palette_range(temp, from, to, TRUE);
        render_to_screen();

        sys_evt_process_pending();
        update_polled_stuff();
        WaitForNextFrame();
    }

    set_palette_range(dest, from, to, TRUE);
}

void fade_in_range(PALETTE *p, int speed, int from, int to, int fade_red, int fade_green, int fade_blue) 
{
    fade_256_init(fade_red, fade_green, fade_blue);
	fade_256_in_range(faded_out_palette, *p, speed, from, to);
}

void fade_out_range(int speed, int from, int to, int fade_red, int fade_green, int fade_blue) 
{
    PALETTE temp;
    fade_256_init(fade_red, fade_green, fade_blue);
    get_palette(temp);
    fade_256_in_range(temp, faded_out_palette, speed, from, to);
}

//-----------------------------------------------------------------------------
// End software fade routines
//-----------------------------------------------------------------------------

void screen_fade_impl(bool do_fadein, int speed)
{
    // harmonise speeds with software driver which is faster (???)
    speed *= 2;
    // Create a "screenshot" on a texture
    play.screen_is_faded_out = 0; // force all game elements to draw
    auto *shot_ddb = game_frame_to_ddb(do_fadein);

    const auto &view = play.GetMainViewport();
    std::unique_ptr<Bitmap> black_bmp(BitmapHelper::CreateBitmap(16, 16, game.GetColorDepth()));
    black_bmp->Clear(makecol(play.fade_to_red, play.fade_to_green, play.fade_to_blue));
    IDriverDependantBitmap *fade = gfxDriver->CreateDDBFromBitmap(black_bmp.get(), false, true);
    fade->SetStretch(view.GetWidth(), view.GetHeight(), false);

    for (int alpha = 1; alpha < 255; alpha += speed)
    {
        // Construct scene in order: game screen, fade fx, post game overlay
        gfxDriver->BeginSpriteBatch(view, SpriteTransform());
        gfxDriver->DrawSprite(0, 0, shot_ddb);
        fade->SetAlpha(do_fadein ? (255 - alpha) : alpha);
        gfxDriver->DrawSprite(0, 0, fade);
        gfxDriver->EndSpriteBatch();
        render_to_screen();

        sys_evt_process_pending();
        update_polled_stuff();
        WaitForNextFrame();
    }

    gfxDriver->DestroyDDB(fade);
    gfxDriver->DestroyDDB(shot_ddb);
}

void screen_fade_software_impl(bool do_fadein, PALETTE *p, int speed)
{
    // TODO: scan these functions and double check the palette manipulations
    if (game.color_depth > 1)
    {
        set_palette(*p);
    }

    if (game.color_depth > 1)
    {
        fade_highcolor(do_fadein, speed * 4, play.fade_to_red, play.fade_to_green, play.fade_to_blue);
    }
    else
    {
        if (do_fadein)
            fade_in_range(p, speed, 0, 255, play.fade_to_red, play.fade_to_green, play.fade_to_blue);
        else
            fade_out_range(speed, 0, 255, play.fade_to_red, play.fade_to_green, play.fade_to_blue);
    }
}

void screen_effect_fade(bool do_fadein, int speed)
{
    if (play.screen_is_faded_out == !do_fadein)
        return; // already in the wanted state

    if (speed <= 0)
        speed = 16;

    if (gfxDriver->UsesMemoryBackBuffer())
        screen_fade_software_impl(do_fadein, &palette, speed);
    else
        screen_fade_impl(do_fadein, speed);
    
    play.screen_is_faded_out = !do_fadein;
}

void screen_box_impl(bool do_fadein, int speed)
{
    // Create a "screenshot" on a texture
    play.screen_is_faded_out = 0; // force all game elements to draw
    auto *shot_ddb = game_frame_to_ddb(do_fadein);

    const auto &view = play.GetMainViewport();
    std::unique_ptr<Bitmap> black_bmp(BitmapHelper::CreateBitmap(16, 16, game.GetColorDepth()));
    black_bmp->Clear(makecol(play.fade_to_red, play.fade_to_green, play.fade_to_blue));
    // For fade-in we create 4 boxes, one across each side of the screen;
    // for fade-out we need 1 box that will stretch from center until covers whole screen
    IDriverDependantBitmap *fade[4]{};
    for (int i = 0; i < (do_fadein ? 4 : 1); i++)
    {
        fade[i] = gfxDriver->CreateDDBFromBitmap(black_bmp.get(), false, true);
        fade[i]->SetStretch(view.GetWidth(), view.GetHeight(), false);
    }

    const int yspeed = view.GetHeight() / (view.GetWidth() / speed);
    int boxWidth = speed;
    int boxHeight = yspeed;

    while (boxWidth < view.GetWidth())
    {
        boxWidth += speed;
        boxHeight += yspeed;

        // Construct scene in order: game screen, fade fx, post game overlay
        gfxDriver->BeginSpriteBatch(view, SpriteTransform());
        gfxDriver->DrawSprite(0, 0, shot_ddb);
        if (do_fadein)
        {
            gfxDriver->DrawSprite(view.GetWidth() / 2 - boxWidth / 2 - view.GetWidth(), 0, fade[0]);
            gfxDriver->DrawSprite(0, view.GetHeight() / 2 - boxHeight / 2 - view.GetHeight(), fade[1]);
            gfxDriver->DrawSprite(view.GetWidth() / 2 + boxWidth / 2, 0, fade[2]);
            gfxDriver->DrawSprite(0, view.GetHeight() / 2 + boxHeight / 2, fade[3]);
        }
        else
        {
            fade[0]->SetStretch(boxWidth, boxHeight, false);
            gfxDriver->DrawSprite(view.GetWidth() / 2 - boxWidth / 2, view.GetHeight() / 2 - boxHeight / 2, fade[0]);
        }
        gfxDriver->EndSpriteBatch();
        render_to_screen();

        sys_evt_process_pending();
        update_polled_stuff();
        WaitForNextFrame();
    }

    for (int i = 0; i < 4; i++)
    {
        if (fade[i])
            gfxDriver->DestroyDDB(fade[i]);
    }
    gfxDriver->DestroyDDB(shot_ddb);
}

void screen_box_software_impl(bool do_fadein, int speed)
{
    // First of all we render the game once again and get the drawn frame as a bitmap.
    // Then we keep drawing saved image of the game, simulating "box-out".
    // TODO: maybe use screenshot_to_ddb instead?
    play.screen_is_faded_out = 0; // force all game elements to draw
    set_palette_range(palette, 0, 255, 0); // TODO: investigate and comment the meaning of this
    std::unique_ptr<Bitmap> bmp_frame = game_frame_to_bmp(do_fadein);

    const Rect &view = play.GetMainViewport();
    const int yspeed = view.GetHeight() / (view.GetWidth() / speed);
    int boxwid = speed, boxhit = yspeed;
    Bitmap *bmp_buff = gfxDriver->GetMemoryBackBuffer();
    
    if (do_fadein)
    {
        bmp_buff->Clear();
        while (boxwid < bmp_buff->GetWidth())
        {
            boxwid += speed;
            boxhit += yspeed;
            boxwid = Math::Clamp(boxwid, 0, view.GetWidth());
            boxhit = Math::Clamp(boxhit, 0, view.GetHeight());
            int lxp = view.GetWidth() / 2 - boxwid / 2;
            int lyp = view.GetHeight() / 2 - boxhit / 2;
            bmp_buff->Blit(bmp_frame.get(), lxp, lyp, lxp, lyp, boxwid, boxhit);
            render_to_screen();

            sys_evt_process_pending();
            update_polled_stuff();
            WaitForNextFrame();
        }
    }
    else
    {
        while (boxwid < view.GetWidth())
        {
            boxwid += speed;
            boxhit += yspeed;
            int vcentre = view.GetHeight() / 2;
            bmp_frame->FillRect(Rect(view.GetWidth() / 2 - boxwid / 2, vcentre - boxhit / 2,
                view.GetWidth() / 2 + boxwid / 2, vcentre + boxhit / 2), 0);
            bmp_buff->Fill(0);
            bmp_buff->Blit(bmp_frame.get());
            render_to_screen();

            sys_evt_process_pending();
            update_polled_stuff();
            WaitForNextFrame();
        }
    }
}

void screen_effect_box(bool do_fadein, int speed)
{
    if (play.screen_is_faded_out == !do_fadein)
        return; // already in the wanted state

    if (gfxDriver->UsesMemoryBackBuffer())
        screen_box_software_impl(do_fadein, speed);
    else
        screen_box_impl(do_fadein, speed);
    
    play.screen_is_faded_out = !do_fadein;
}

void screen_effect_crossfade()
{
    if (game.color_depth == 1)
        quit("!Cannot use crossfade screen transition in 256-colour games");

    // TODO: crossfade does not need a screen with transparency, it should be opaque;
    // but Software renderer cannot alpha-blend non-masked sprite at the moment,
    // see comment to drawing opaque sprite in SDLRendererGraphicsDriver!
    IDriverDependantBitmap *ddb = get_frame_for_transition_in(false /* transparent */);
    for (int alpha = 254; alpha > 0; alpha -= 16)
    {
        // do the crossfade
        ddb->SetAlpha(alpha);
        invalidate_screen();
        gfxDriver->ClearDrawLists();
        construct_game_scene(true);
        construct_game_screen_overlay(false);
        // draw old screen on top while alpha > 16
        if (alpha > 16)
        {
            gfxDriver->BeginSpriteBatch(play.GetMainViewport(), SpriteTransform());
            gfxDriver->DrawSprite(0, 0, ddb);
            gfxDriver->EndSpriteBatch();
        }
        render_to_screen();

        sys_evt_process_pending();
        update_polled_stuff();
        WaitForNextFrame();
    }

    saved_viewport_bitmap.reset();
    set_palette_range(palette, 0, 255, 0);
    gfxDriver->DestroyDDB(ddb);
}

void screen_effect_dissolve()
{
    int pattern[16]={0,4,14,9,5,11,2,8,10,3,12,7,15,6,13,1};
    RGB interpal[256];
    const Rect &viewport = play.GetMainViewport();

    IDriverDependantBitmap *ddb = get_frame_for_transition_in(false /* transparent */);
    for (int step = 0; step < 16; ++step)
    {
        // merge the palette while dithering
        if (game.color_depth == 1) 
        {
            fade_interpolate(old_palette, palette, interpal, step*4, 0, 255);
            set_palette_range(interpal, 0, 255, 0);
        }
        // do the dissolving
        int maskCol = saved_viewport_bitmap->GetMaskColor();
        for (int x = 0; x < viewport.GetWidth(); x += 4)
        {
            for (int y = 0; y < viewport.GetHeight(); y += 4)
            {
                saved_viewport_bitmap->PutPixel(x + pattern[step] / 4, y + pattern[step] % 4, maskCol);
            }
        }
        gfxDriver->UpdateDDBFromBitmap(ddb, saved_viewport_bitmap.get(), false);

        gfxDriver->ClearDrawLists();
        construct_game_scene(true);
        construct_game_screen_overlay(false);
        gfxDriver->BeginSpriteBatch(play.GetMainViewport(), SpriteTransform());
        gfxDriver->DrawSprite(0, 0, ddb);
        gfxDriver->EndSpriteBatch();
        render_to_screen();

        sys_evt_process_pending();
        update_polled_stuff();
        WaitForNextFrame();
    }

    saved_viewport_bitmap.reset();
    set_palette_range(palette, 0, 255, 0);
    gfxDriver->DestroyDDB(ddb);
}

void current_fade_out_effect()
{
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
        screen_effect_fade(false, 5);
    }
    else if (theTransition == FADE_BOXOUT) 
    {
        screen_effect_box(false, get_fixed_pixel_size(16));
    }
    else 
    {
        saved_viewport_bitmap = game_frame_to_bmp(false /* fade out */);
    }
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
    data_to_game_coords(&scrx, &scry);

    VpPoint vpt = play.ScreenToRoom(scrx, scry, restrict);
    if (vpt.second < 0)
        return nullptr;

    game_to_data_coords(vpt.first.X, vpt.first.Y);
    return ScriptStructHelpers::CreatePoint(vpt.first.X, vpt.first.Y);
}

ScriptUserObject* Screen_ScreenToRoomPoint2(int scrx, int scry)
{
    return Screen_ScreenToRoomPoint(scrx, scry, true);
}

ScriptUserObject *Screen_RoomToScreenPoint(int roomx, int roomy)
{
    data_to_game_coords(&roomx, &roomy);
    Point pt = play.RoomToScreen(roomx, roomy);
    game_to_data_coords(pt.X, pt.Y);
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
