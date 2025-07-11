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
#include "plugin/plugin_engine.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern IGraphicsDriver *gfxDriver;
extern AGSPlatformDriver *platform;
extern int displayed_room;
extern RGB palette[256];

std::unique_ptr<Bitmap> saved_viewport_bitmap;
RGB old_palette[256];


//-----------------------------------------------------------------------------
// Game screenshot-making functions.
//-----------------------------------------------------------------------------
// Render game screen and copy to bitmap.
// NOTE: for fading-out:
// please keep in mind: redrawing last saved frame here instead of constructing new one
// is done because of backwards-compatibility issue: originally AGS faded out using frame
// drawn before the script that triggers blocking fade (e.g. instigated by ChangeRoom).
// Unfortunately some existing games were changing looks of the screen during same function,
// but these were not supposed to get on screen until before fade-in.
//
// This special fade-out behavior may be deprecated later if wanted.
static std::unique_ptr<Bitmap> game_frame_to_bmp(bool for_fadein)
{
    get_palette(old_palette);
    const auto &view = play.GetMainViewport();
    if (for_fadein)
    {
        gfxDriver->ClearDrawLists();
        construct_game_scene(true);
        construct_game_screen_overlay(false /* no cursor */);
        gfxDriver->RenderToBackBuffer();
    }
    return std::unique_ptr<Bitmap>(
        CopyScreenIntoBitmap(view.GetWidth(), view.GetHeight(),
        &view, true /* always in native res */, RENDER_SHOT_SKIP_ON_FADE));
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
        fix_frame->Blit(saved_viewport_bitmap.get(),
            0, 0, 0, (viewport.GetHeight() - saved_viewport_bitmap->GetHeight()) / 2,
            saved_viewport_bitmap->GetWidth(), saved_viewport_bitmap->GetHeight());
        saved_viewport_bitmap.reset(fix_frame);
    }
    return gfxDriver->CreateDDBFromBitmap(saved_viewport_bitmap.get(), opaque ? kTxFlags_Opaque : kTxFlags_None);
}


//-----------------------------------------------------------------------------
// Transition game states
//-----------------------------------------------------------------------------

class ScreenTransition : public GameState
{
public:
    ScreenTransition(ScreenTransitionStyle style, bool fade_in, int speed)
        : _style(style), _fadein(fade_in), _speed(speed)
    {
    }

    ~ScreenTransition() override
    {
        saved_viewport_bitmap.reset();
    }

    // Update the state during a game tick
    bool Run() override
    {
        if (!RunImpl())
            return false;

        Draw();

        sys_evt_process_pending();
        update_polled_stuff();
        WaitForNextFrame();
        return true;
    }

protected:
    virtual bool RunImpl() = 0;

    ScreenTransitionStyle _style = kScrTran_Instant;
    bool _fadein = false;
    int _speed = 0;
};

class ScreenFade : public ScreenTransition
{
public:
    ScreenFade(bool fade_in, int speed)
        : ScreenTransition(kScrTran_Fade, fade_in, speed) {}

    // Begin the state, initialize and prepare any resources
    void Begin() override
    {
        if (_speed <= 0)
            _speed = 16;

        // harmonise speeds with software driver which is faster (???)
        _speed *= 2;
        // Create a "screenshot" on a texture
        play.screen_is_faded_out = 0; // force all game elements to draw

        _view = play.GetMainViewport();
        _sprTrans = play.GetGlobalTransform(true);

        std::unique_ptr<Bitmap> black_bmp(BitmapHelper::CreateBitmap(16, 16, game.GetColorDepth()));
        black_bmp->Clear(makecol(play.fade_to_red, play.fade_to_green, play.fade_to_blue));
        _fade = gfxDriver->CreateDDBFromBitmap(black_bmp.get(), kTxFlags_Opaque);
        _fade->SetStretch(_view.GetWidth(), _view.GetHeight(), false);

        _alpha = 1;
    }
    // End the state, release all resources
    void End() override
    {
        gfxDriver->DestroyDDB(_fade);
    }
    // Draw the state
    void Draw() override
    {
        // Construct scene in order: game screen, fade fx, post game overlay.
        // NOTE: for backwards compatibility we have to redraw last frame
        // when fading out. Please see the note to game_frame_to_bmp() for details.
        if (_fadein)
            construct_game_scene();
        else
            gfxDriver->RedrawLastFrame(RENDER_SHOT_SKIP_ON_FADE);

        gfxDriver->BeginSpriteBatch(_view, _sprTrans, kFlip_None, nullptr, RENDER_SHOT_SKIP_ON_FADE);
        _fade->SetAlpha(_fadein ? (255 - _alpha) : _alpha);
        gfxDriver->DrawSprite(0, 0, _fade);
        gfxDriver->EndSpriteBatch();
        render_to_screen();
    }
    // Update the state during a game tick
    bool RunImpl() override
    {
        if (_alpha >= 255)
            return false;
        _alpha = std::min(_alpha + _speed, 255);
        return true;
    }

private:
    bool _fadeIn = false;
    IDriverDependantBitmap *_fade = nullptr;
    int _alpha = 0;
    Rect _view;
    SpriteTransform _sprTrans;
};

// TODO: split further onto 256- and highcolor- software fade
class ScreenFadeSoftware : public ScreenTransition
{
public:
    ScreenFadeSoftware(PALETTE *fadein_pal, bool fade_in, int speed, int fader, int fadeg, int fadeb)
        : ScreenTransition(kScrTran_Fade, fade_in, speed)
        , _fadeInPal(fadein_pal)
    {
        _fadeCol.r = fader;
        _fadeCol.g = fadeg;
        _fadeCol.b = fadeb;
    }

    // Begin the state, initialize and prepare any resources
    void Begin() override
    {
        // TODO: scan these functions and double check the palette manipulations
        if (game.color_depth > 1)
        {
            set_palette(*_fadeInPal);
        }

        play.screen_is_faded_out = 0; // force all game elements to draw
        _alpha = 0;

        if (game.color_depth > 1)
        {
            _speed *= 4; // speed things up for high-color games
            _view = play.GetMainViewport();

            // First of all we render the game once again and get the drawn frame as a bitmap.
            // Then we keep drawing saved image of the game with different alpha,
            // simulating fade-in or out.
            _bmpFrame = game_frame_to_bmp(_fadein);

            _bmpBuff = gfxDriver->GetMemoryBackBuffer();
            const int col_depth = _bmpBuff->GetColorDepth();
            _clearCol = makecol_depth(col_depth, _fadeCol.r, _fadeCol.g, _fadeCol.b);
        }
        else
        {
            // Prepare game frame to the backbuffer once, but don't present yet
            gfxDriver->ClearDrawLists();
            construct_game_scene(true);
            construct_game_screen_overlay(false /* no cursor */);
            gfxDriver->RenderToBackBuffer();

            Fade256Init(_fadeCol.r, _fadeCol.g, _fadeCol.b);
            if (_fadein)
            {
                _srcPal = &_fadePal;
                _destPal = _fadeInPal;
            }
            else
            {
                get_palette(_lastPal);
                _srcPal = &_lastPal;
                _destPal = &_fadePal;
            }

            for (int c = 0; c < PAL_SIZE; c++)
                _dynamicPal[c] = (*_srcPal)[c];
        }
    }
    // End the state, release all resources
    void End() override
    {
        if (game.color_depth > 1)
        {
            invalidate_screen();
        }
        else
        {
            set_palette_range(*_destPal, _rangeFrom, _rangeTo, TRUE);
        }
    }
    // Draw the state
    void Draw() override
    {
        if (game.color_depth > 1)
        {
            _bmpBuff->Fill(_clearCol);
            set_trans_blender(0, 0, 0, _fadein ? _alpha : 255 - _alpha);
            _bmpBuff->TransBlendBlt(_bmpFrame.get(), _view.Left, _view.Top);
            render_to_screen();
        }
        else
        {
            render_to_screen();
        }
    }
    // Update the state during a game tick
    bool RunImpl() override
    {
        if (game.color_depth > 1)
        {
            if (_alpha >= 255)
                return false;
            _alpha = std::min(_alpha + _speed, 255);
            return true;
        }
        else
        {
            if (_alpha >= 64)
                return false;
            _alpha = std::min(_alpha + _speed, 64);
            fade_interpolate(*_srcPal, *_destPal, _dynamicPal, _alpha, _rangeFrom, _rangeTo);
            set_palette_range(_dynamicPal, _rangeFrom, _rangeTo, TRUE);
            return true;
        }
    }

private:
    void Fade256Init(int r, int g, int b)
    {
        for (int a = 0; a < 256; a++)
        {
            _fadePal[a].r = r / 4;
	        _fadePal[a].g = g / 4;
	        _fadePal[a].b = b / 4;
        }
    }

    PALETTE *_fadeInPal = nullptr;
    RGB _fadeCol{};
    int _alpha = 0;
    int _rangeFrom = 0;
    int _rangeTo = 255;
    Rect _view;

    // High-color state
    Bitmap *_bmpBuff = nullptr;
    std::unique_ptr<Bitmap> _bmpFrame;
    int _clearCol = 0;

    // 256-color state
    PALETTE _fadePal{};
    PALETTE _lastPal{};
    PALETTE *_srcPal = nullptr;
    PALETTE *_destPal = nullptr;
    PALETTE _dynamicPal{};
};

class ScreenBoxOut : public ScreenTransition
{
public:
    ScreenBoxOut(bool fade_in, int speed)
        : ScreenTransition(kScrTran_Boxout, fade_in, speed) {}

    // Begin the state, initialize and prepare any resources
    void Begin() override
    {
        // Create a "screenshot" on a texture
        play.screen_is_faded_out = 0; // force all game elements to draw

        _view = play.GetMainViewport();
        _sprTrans = play.GetGlobalTransform(true);

        std::unique_ptr<Bitmap> black_bmp(BitmapHelper::CreateBitmap(16, 16, game.GetColorDepth()));
        black_bmp->Clear(makecol(play.fade_to_red, play.fade_to_green, play.fade_to_blue));
        for (int i = 0; i < (_fadein ? 4 : 1); i++)
        {
            _fade[i] = gfxDriver->CreateDDBFromBitmap(black_bmp.get(), kTxFlags_Opaque);
            _fade[i]->SetStretch(_view.GetWidth(), _view.GetHeight(), false);
        }

        _yspeed = _view.GetHeight() / (_view.GetWidth() / _speed);
        _boxWidth = _speed;
        _boxHeight = _yspeed;
    }
    // End the state, release all resources
    void End() override
    {
        for (int i = 0; i < 4; i++)
        {
            if (_fade[i])
                gfxDriver->DestroyDDB(_fade[i]);
        }
    }
    // Draw the state
    void Draw() override
    {
        // NOTE: for backwards compatibility we have to redraw last frame
        // when fading out. Please see the note to game_frame_to_bmp() for details.
        if (_fadein)
            construct_game_scene();
        else
            gfxDriver->RedrawLastFrame(RENDER_SHOT_SKIP_ON_FADE);

        gfxDriver->BeginSpriteBatch(_view, _sprTrans, kFlip_None, nullptr, RENDER_SHOT_SKIP_ON_FADE);
        if (_fadein)
        {
            gfxDriver->DrawSprite(_view.GetWidth() / 2 - _boxWidth / 2 - _view.GetWidth(), 0, _fade[0]);
            gfxDriver->DrawSprite(0, _view.GetHeight() / 2 - _boxHeight / 2 - _view.GetHeight(), _fade[1]);
            gfxDriver->DrawSprite(_view.GetWidth() / 2 + _boxWidth / 2, 0, _fade[2]);
            gfxDriver->DrawSprite(0, _view.GetHeight() / 2 + _boxHeight / 2, _fade[3]);
        }
        else
        {
            _fade[0]->SetStretch(_boxWidth, _boxHeight, false);
            gfxDriver->DrawSprite(_view.GetWidth() / 2 - _boxWidth / 2, _view.GetHeight() / 2 - _boxHeight / 2, _fade[0]);
        }
        gfxDriver->EndSpriteBatch();
        render_to_screen();
    }
    // Update the state during a game tick
    bool RunImpl() override
    {
        if (_boxWidth >= _view.GetWidth())
            return false;
        _boxWidth = std::min(_boxWidth + _speed, _view.GetWidth());
        _boxHeight = std::min(_boxHeight + _yspeed, _view.GetHeight());
        return true;
    }

private:
    // For fade-in we create 4 boxes, one across each side of the screen;
    // for fade-out we need 1 box that will stretch from center until covers whole screen
    IDriverDependantBitmap *_fade[4]{};
    int _yspeed = 0;
    int _boxWidth = 0;
    int _boxHeight = 0;
    Rect _view;
    SpriteTransform _sprTrans;
};

class ScreenBoxOutSoftware : public ScreenTransition
{
public:
    ScreenBoxOutSoftware(bool fade_in, int speed)
        : ScreenTransition(kScrTran_Boxout, fade_in, speed) {}

    // Begin the state, initialize and prepare any resources
    void Begin() override
    {
        // First of all we render the game once again and get the drawn frame as a bitmap.
        // Then we keep drawing saved image of the game, simulating "box-out".
        // TODO: maybe use screenshot_to_ddb instead?
        play.screen_is_faded_out = 0; // force all game elements to draw
        set_palette_range(palette, 0, 255, 0); // TODO: investigate and comment the meaning of this
        _bmpFrame = game_frame_to_bmp(_fadein);

        _view = play.GetMainViewport();
        _yspeed = _view.GetHeight() / (_view.GetWidth() / _speed);
        _boxWidth = _speed;
        _boxHeight = _yspeed;
        _bmpBuff = gfxDriver->GetMemoryBackBuffer();

        if (_fadein)
        {
            _bmpBuff->Clear();
        }
    }
    // End the state, release all resources
    void End() override
    {
        invalidate_screen();
    }
    // Draw the state
    void Draw() override
    {
        if (_fadein)
        {
            _boxWidth = Math::Clamp(_boxWidth, 0, _view.GetWidth());
            _boxHeight = Math::Clamp(_boxHeight, 0, _view.GetHeight());
            int srcx = _view.GetWidth() / 2 - _boxWidth / 2;
            int srcy = _view.GetHeight() / 2 - _boxHeight / 2;
            _bmpBuff->Blit(_bmpFrame.get(), srcx, srcy, _view.Left + srcx, _view.Top + srcy, _boxWidth, _boxHeight);
            render_to_screen();
        }
        else
        {
            int hcentre = _view.GetWidth() / 2;
            int vcentre = _view.GetHeight() / 2;
            _bmpFrame->FillRect(Rect(hcentre - _boxWidth / 2, vcentre - _boxHeight / 2,
                hcentre + _boxWidth / 2, vcentre + _boxHeight / 2), 0);
            _bmpBuff->Fill(0);
            _bmpBuff->Blit(_bmpFrame.get(), _view.Left, _view.Top);
            render_to_screen();
        }
    }
    // Update the state during a game tick
    bool RunImpl() override
    {
        const int target_width = _fadein ?
            _bmpBuff->GetWidth() :
            _view.GetWidth();
        const int target_height = _fadein ?
            _bmpBuff->GetHeight() :
            _view.GetHeight();
        if (_boxWidth >= target_width)
            return false;
        _boxWidth = std::min(_boxWidth + _speed, target_width);
        _boxHeight = std::min(_boxHeight + _yspeed, target_height);
        return true;
    }

private:
    Bitmap *_bmpBuff = nullptr;
    std::unique_ptr<Bitmap> _bmpFrame;
    int _yspeed = 0;
    int _boxWidth = 0;
    int _boxHeight = 0;
    Rect _view;
};

class ScreenCrossfade : public ScreenTransition
{
public:
    ScreenCrossfade(bool fade_in, int speed)
        : ScreenTransition(kScrTran_Crossfade, fade_in, speed) {}

    // Begin the state, initialize and prepare any resources
    void Begin() override
    {
        if (game.color_depth == 1)
            quit("!Cannot use crossfade screen transition in 256-colour games");

        play.screen_is_faded_out = 0; // force all game elements to draw
        _view = play.GetMainViewport();
        _sprTrans = play.GetGlobalTransform(gfxDriver->RequiresFullRedrawEachFrame());
        // TODO: crossfade does not need a screen with transparency, it should be opaque;
        // but Software renderer cannot alpha-blend non-masked sprite at the moment,
        // see comment to drawing opaque sprite in SDLRendererGraphicsDriver!
        _shot_ddb = get_frame_for_transition_in(false /* transparent */);

        _alpha = 254;
    }
    // End the state, release all resources
    void End() override
    {
        saved_viewport_bitmap.reset();
        invalidate_screen();
        set_palette_range(palette, 0, 255, 0);
        gfxDriver->DestroyDDB(_shot_ddb);
    }
    // Draw the state
    void Draw() override
    {
        // do the crossfade
        _shot_ddb->SetAlpha(_alpha);
        construct_game_scene(true);
        construct_game_screen_overlay(false /* no cursor */);
        // draw old screen on top while alpha > 16
        if (_alpha > 16)
        {
            gfxDriver->BeginSpriteBatch(_view, _sprTrans);
            gfxDriver->DrawSprite(0, 0, _shot_ddb);
            gfxDriver->EndSpriteBatch();
        }
        render_to_screen();
    }
    // Update the state during a game tick
    bool RunImpl() override
    {
        if (_alpha <= 0)
            return false;
        _alpha = std::max(_alpha - 16, 0);
        return true;
    }

private:
    IDriverDependantBitmap *_shot_ddb = nullptr;
    int _alpha = 0;
    Rect _view;
    SpriteTransform _sprTrans;
};

class ScreenDissolve : public ScreenTransition
{
public:
    ScreenDissolve(bool fade_in, int speed)
        : ScreenTransition(kScrTran_Dissolve, fade_in, speed) {}

    // Begin the state, initialize and prepare any resources
    void Begin() override
    {
        play.screen_is_faded_out = 0; // force all game elements to draw
        _view = play.GetMainViewport();
        _sprTrans = play.GetGlobalTransform(gfxDriver->RequiresFullRedrawEachFrame());
        _shot_ddb = get_frame_for_transition_in(false /* transparent */);
        _step = 0;
    }
    // End the state, release all resources
    void End() override
    {
        saved_viewport_bitmap.reset();
        invalidate_screen();
        set_palette_range(palette, 0, 255, 0);
        gfxDriver->DestroyDDB(_shot_ddb);
    }
    // Draw the state
    void Draw() override
    {
        construct_game_scene(true);
        construct_game_screen_overlay(false /* no cursor */);
        gfxDriver->BeginSpriteBatch(_view, _sprTrans);
        gfxDriver->DrawSprite(0, 0, _shot_ddb);
        gfxDriver->EndSpriteBatch();
        render_to_screen();
    }
    // Update the state during a game tick
    bool RunImpl() override
    {
        if (_step >= 16)
            return false;

        // merge the palette while dithering
        if (game.color_depth == 1) 
        {
            fade_interpolate(old_palette, palette, _interpal, _step * 4, 0, 255);
            set_palette_range(_interpal, 0, 255, 0);
        }
        // do the dissolving
        int maskCol = saved_viewport_bitmap->GetMaskColor();
        for (int x = 0; x < _view.GetWidth(); x += 4)
        {
            for (int y = 0; y < _view.GetHeight(); y += 4)
            {
                saved_viewport_bitmap->PutPixel(x + _pattern[_step] / 4, y + _pattern[_step] % 4, maskCol);
            }
        }
        gfxDriver->UpdateDDBFromBitmap(_shot_ddb, saved_viewport_bitmap.get(), false);

        _step = std::min(_step + 1, 16);
        return true;
    }

private:
    IDriverDependantBitmap *_shot_ddb = nullptr;
    int _step = 0;
    const int _pattern[16] = {0,4,14,9,5,11,2,8,10,3,12,7,15,6,13,1};
    Rect _view;
    SpriteTransform _sprTrans;
    // For 8-bit palette mode, will interpolate between old and new room palettes
    RGB _interpal[256]{};
};

void run_screen_transition(ScreenTransitionStyle style, bool fade_in, int speed)
{
    if (play.screen_is_faded_out == !fade_in)
        return; // already in the wanted state

    bool software_mode = gfxDriver->UsesMemoryBackBuffer();
    std::unique_ptr<ScreenTransition> scrtr;
    switch (style)
    {
    case kScrTran_Fade:
        scrtr.reset(software_mode ?
            (ScreenTransition*)new ScreenFadeSoftware(&palette, fade_in, speed, play.fade_to_red, play.fade_to_green, play.fade_to_blue) :
            (ScreenTransition*)new ScreenFade(fade_in, speed));
        break;
    case kScrTran_Boxout:
        scrtr.reset(software_mode ?
            (ScreenTransition*)new ScreenBoxOutSoftware(fade_in, speed) :
            (ScreenTransition*)new ScreenBoxOut(fade_in, speed));
        break;
    case kScrTran_Crossfade:
        scrtr.reset(new ScreenCrossfade(fade_in, speed));
        break;
    case kScrTran_Dissolve:
        scrtr.reset(new ScreenDissolve(fade_in, speed));
        break;
    default:
        return;
    }

    scrtr->Begin();
    while (scrtr->Run());
    scrtr->End();

    play.screen_is_faded_out = !fade_in;
}

void run_fade_in_effect(ScreenTransitionStyle style, int speed)
{
    const bool ignore_transition = (play.screen_tint > 0);
    if ((style == kScrTran_Instant) || ignore_transition)
    {
        set_palette_range(palette, 0, 255, 0);
    }
    else
    {
        run_screen_transition(style, true, speed);
    }

    play.screen_is_faded_out = 0; // mark screen as clear
}

// Tells if we should use instant transition effect, based certain conditions
inline bool should_instant_transition(ScreenTransitionStyle style)
{
    return (style == kScrTran_Instant) ||
        play.screen_tint > 0; // for some reason we do not play fade if screen is tinted
}

void run_fade_out_effect(ScreenTransitionStyle style, int speed)
{
    const bool instant_transition = should_instant_transition(style);
    if (instant_transition)
    {
        if (!play.keep_screen_during_instant_transition)
            set_palette_range(black_palette, 0, 255, 0);
    }
    else if (style == kScrTran_Crossfade || style == kScrTran_Dissolve)
    {
        saved_viewport_bitmap = game_frame_to_bmp(false /* fade out */);
    }
    else
    {
        run_screen_transition(style, false, speed);
    }

    // NOTE: the screen could have been faded out prior to transition out
    play.screen_is_faded_out |= (!instant_transition); // mark screen as faded
}

void current_fade_in_effect()
{
    debug_script_log("Transition-in in room %d", displayed_room);

    // determine the transition style
    ScreenTransitionStyle trans_style = static_cast<ScreenTransitionStyle>(play.fade_effect);
    // if a one-off transition was selected, then use it
    if (play.next_screen_transition >= 0)
    {
        trans_style = static_cast<ScreenTransitionStyle>(play.next_screen_transition);
        play.next_screen_transition = -1;
    }

    if (pl_run_plugin_hooks(kPluginEvt_TransitionIn, 0))
    {
        play.screen_is_faded_out = 0; // mark screen as clear
        return;
    }

    if (play.fast_forward)
    {
        play.screen_is_faded_out = 0; // mark screen as clear
        return;
    }

    const bool ignore_transition = (play.screen_tint > 0);
    if (((trans_style == kScrTran_Crossfade) || (trans_style == kScrTran_Dissolve)) &&
        (saved_viewport_bitmap == nullptr) && !ignore_transition)
    {
        // transition type was not crossfade/dissolve when the screen faded out,
        // but it is now when the screen fades in (Eg. a save game was restored
        // with a different setting). Therefore just fade normally.
        run_fade_out_effect(kScrTran_Fade, 5);
        trans_style = kScrTran_Fade;
    }

    int def_speed = 0;
    if (trans_style == kScrTran_Fade)
    {
        def_speed = 5;
    }
    else if (trans_style == kScrTran_Boxout) 
    {
        def_speed = get_fixed_pixel_size(16);
    }

    run_fade_in_effect(trans_style, def_speed);
}

void current_fade_out_effect()
{
    debug_script_log("Transition-out in room %d", displayed_room);
    if (pl_run_plugin_hooks(kPluginEvt_TransitionOut, 0))
        return;

    // get the screen transition type
    ScreenTransitionStyle trans_style = (play.next_screen_transition >= 0) ?
        // was a temporary transition selected? if so, use it
        static_cast<ScreenTransitionStyle>(play.next_screen_transition) :
        // else use default game setting
        static_cast<ScreenTransitionStyle>(play.fade_effect);

    if (play.fast_forward)
    {
        play.screen_is_faded_out |= !(should_instant_transition(trans_style)); // mark screen as faded
        return;
    }

    int def_speed = 0;
    if (trans_style == kScrTran_Fade)
    {
        def_speed = 5;
    }
    else if (trans_style == kScrTran_Boxout) 
    {
        def_speed = get_fixed_pixel_size(16);
    }

    run_fade_out_effect(trans_style, def_speed);
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
