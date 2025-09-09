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
#include "ac/display.h"
#include <math.h>
#include <stdio.h>
#include "ac/display.h"
#include "ac/common.h"
#include "ac/character.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/global_game.h"
#include "ac/gui.h"
#include "ac/joystick.h"
#include "ac/mouse.h"
#include "ac/overlay.h"
#include "ac/sys_events.h"
#include "ac/screenoverlay.h"
#include "ac/speech.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/timer.h"
#include "ac/touch.h"
#include "debug/debug_log.h"
#include "font/agsfontrenderer.h"
#include "font/fonts.h"
#include "gfx/blender.h"
#include "gfx/gfx_util.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::Common::BitmapHelper;

extern GameSetupStruct game;
extern int longestline;
extern AGSPlatformDriver *platform;
extern int loops_per_character;
extern SpriteCache spriteset;

bool display_check_user_input(int skip);

// Game state of a displayed blocking message
class DisplayMessageState : public GameState
{
public:
    DisplayMessageState(int over_type, int timer, int skip_style)
        : _overType(over_type), _timer(timer), _skipStyle(skip_style) {}

    // Begin the state, initialize and prepare any resources
    void Begin() override
    {
    }
    // End the state, release all resources
    void End() override
    {
        remove_screen_overlay(_overType);
        invalidate_screen();
    }
    // Draw the state
    void Draw() override
    {
        render_graphics();
    }
    // Update the state during a game tick
    bool Run() override
    {
        sys_evt_process_pending();

        update_audio_system_on_game_loop();
        UpdateCursorAndDrawables();

        Draw();

        // Handle player's input, break the loop if requested
        bool do_break = display_check_user_input(_skipStyle);
        if (do_break)
            return false;
            
        update_polled_stuff();

        if (play.fast_forward == 0)
        {
            WaitForNextFrame();
        }

        _timer--;

        // Special behavior when coupled with a voice-over
        if (play.speech_has_voice) {
            // extend life of text if the voice hasn't finished yet
            if (AudioChans::ChannelIsPlaying(SCHAN_SPEECH) && (play.fast_forward == 0)) {
                if (_timer <= 1)
                    _timer = 1;
            }
            else { // if the voice has finished, remove the speech
                _timer = 0;
            }
        }
        // Test for the timed auto-skip
        if ((_timer < 1) && (_skipStyle & SKIP_AUTOTIMER))
        {
            play.SetWaitSkipResult(SKIP_AUTOTIMER);
            play.SetIgnoreInput(play.ignore_user_input_after_text_timeout_ms);
            return false;
        }
        // if skipping cutscene, don't get stuck on No Auto Remove text boxes
        if ((_timer < 1) && (play.fast_forward))
            return false;

        return true; // continue running
    }

private:
    int _overType = 0;
    int _timer = 0;
    int _skipStyle = 0;
};


DisplayTextPosition get_textpos_from_scriptcoords(int x, int y, bool for_speech)
{
    int text_pos = kDisplayTextPos_Normal;
    if (for_speech)
    {
        if (x < 0) text_pos |= kDisplayTextPos_OvercharX;
        if (y < 0) text_pos |= kDisplayTextPos_OvercharY;
    }
    else
    {
        if (x < 0) text_pos |= kDisplayTextPos_ScreenCenterX;
        if (y < 0) text_pos |= kDisplayTextPos_ScreenCenterY;
    }
    return (DisplayTextPosition)text_pos;
}

Bitmap *create_textual_image(const char *text, const DisplayTextLooks &look, color_t text_color,
    int &xx, int &yy, int &adjustedXX, int &adjustedYY, int wii, int usingfont,
    const TopBarSettings *topbar)
{
    //
    // Configure the textual image
    //

    const bool use_speech_textwindow = (look.Style == kDisplayTextStyle_TextWindow)
        && (game.options[OPT_SPEECHTYPE] >= kSpeechStyle_SierraBackground);
    const bool use_thought_gui = (look.AsThought) && (game.options[OPT_THOUGHTGUI] > 0);

    int usingGui = -1;
    if (use_speech_textwindow)
        usingGui = play.speech_textwindow_gui;
    else if (use_thought_gui)
        usingGui = game.options[OPT_THOUGHTGUI];

    const int screen_padding = 5; // historical limit of text placement from any screen border
    const int padding = get_textwindow_padding(usingGui);
    const int paddingScaled = padding;
    const int paddingDoubledScaled = padding * 2; // Just in case screen size does is not neatly divisible by 320x200

    break_up_text_into_lines(text, Lines, wii - 2 * padding, usingfont);
    DisplayVars disp(
        get_font_linespacing(usingfont),
        get_text_lines_height(usingfont, Lines.Count()));

    if (topbar)
    {
        // ensure that the window is wide enough to display any top bar text
        int topBarWid = get_text_width_outlined(topbar->Text.GetCStr(), topbar->Font);
        topBarWid += (play.top_bar_borderwidth + 2) * 2;
        if (longestline < topBarWid)
            longestline = topBarWid;
    }

    const Rect &ui_view = play.GetUIViewport();
    // Center the text in the middle of the screen
    if ((look.Position & kDisplayTextPos_ScreenCenterY) != 0)
    {
        yy = ui_view.GetHeight() / 2 - disp.FullTextHeight / 2 - padding;
    }
    // If ordered to place around the character, then align by the text's bottom and clamp to screen
    else if ((look.Position & kDisplayTextPos_OvercharY) != 0)
    {
        yy -= disp.FullTextHeight;
        yy = adjust_y_for_guis(yy);
        yy = Math::Clamp(yy, screen_padding, ui_view.GetHeight() - screen_padding - disp.FullTextHeight);
    }
    else
    {
        // NOTE: this is possibly an accidental mistake, but historically
        // this Y pos fixup is also applied for SayAt, which results in
        // text's origin being a left-bottom rather than left-top.
        // Maybe this could be fixed in some future versions...
        if (look.Style == kDisplayTextStyle_Overchar)
        {
            yy -= disp.FullTextHeight;
            yy = adjust_y_for_guis(yy);
            yy = std::max(yy, screen_padding); // lower if beyond upper screen edge
        }

        // If ordered to clamp to the screen, then do so
        if ((look.Position & kDisplayTextPos_ClampToScreenHeight) != 0)
        {
            yy = Math::Clamp(yy, screen_padding, ui_view.GetHeight() - screen_padding - disp.FullTextHeight);
        }
    }

    // If longest line is shorter than the requested width,
    // and shrink is allowed, then shrink the text window
    if ((look.AllowShrink != kDisplayTextShrink_None)
        && (longestline < wii - paddingDoubledScaled))
    {
        // shrink the width of the dialog box to fit the text
        const int old_wid = wii;
        wii = longestline + paddingDoubledScaled;
        // shift the dialog box right to align it, if necessary
        if ((look.AllowShrink == kDisplayTextShrink_Right) && (xx >= 0))
            xx += (old_wid - wii);
    }

    if ((look.Position & kDisplayTextPos_ScreenCenterX) != 0)
    // Center the text in the middle of the screen
    {
        xx = ui_view.GetWidth() / 2 - wii / 2;
    }
    // If ordered to place around the character, then center-align, and clamp to the screen
    else if ((look.Position & kDisplayTextPos_OvercharX) != 0)
    {
        xx -= wii / 2;
        xx = adjust_x_for_guis(xx, yy);
        xx = Math::Clamp(xx, screen_padding, ui_view.GetWidth() - screen_padding - wii);
    }
    // Finally, if only ordered to clamp to the screen, then do so
    else if ((look.Position & kDisplayTextPos_ClampToScreenWidth) != 0)
    {
        xx = Math::Clamp(xx, screen_padding, ui_view.GetWidth() - screen_padding - wii);
    }

    const int extraHeight = paddingDoubledScaled;
    const int bmp_width = std::max(2, wii);
    const int bmp_height = std::max(2, disp.FullTextHeight + extraHeight);
    Bitmap *text_window_ds = BitmapHelper::CreateTransparentBitmap(bmp_width, bmp_height, game.GetColorDepth());

    // inform draw_text_window to free the old bitmap
    const bool wantFreeScreenop = true;

    //
    // Create the textual image (may also adjust some params in the process)
    //

    // may later change if usingGUI, needed to avoid changing original coordinates
    adjustedXX = xx;
    adjustedYY = yy;

    // if it's an empty speech line, don't draw anything
    if ((strlen(text) < 1) || (strcmp(text, "  ") == 0) || (wii == 0))
        return text_window_ds;

    if (look.Style != kDisplayTextStyle_MessageBox)
    {
        // Textual overlay purposed for character speech
        int ttxleft = 0, ttxtop = paddingScaled, oriwid = wii - padding * 2;
        int drawBackground = 0;

        DisplayTextLooks fix_look = look;
        if (use_speech_textwindow)
        {
            drawBackground = 1;
        }
        else if (use_thought_gui)
        {
            // make it treat it as drawing inside a window now
            fix_look.Style = kDisplayTextStyle_TextWindow;
            drawBackground = 1;
        }

        if (drawBackground)
        {
            text_color = GUI::GetStandardColorForBitmap(15); // use fixed standard color here
            draw_text_window_and_bar(&text_window_ds, wantFreeScreenop, topbar, disp,
                &ttxleft, &ttxtop, &adjustedXX, &adjustedYY, &wii, &text_color, 0, usingGui);
        }
        {
        }

        // Assign final text color, either use passed parameter, or TextWindow property
        if (fix_look.Style == kDisplayTextStyle_TextWindow)
        {
            if ((usingGui >= 0) &&
               ((game.options[OPT_SPEECHTYPE] >= kSpeechStyle_SierraBackground) || (fix_look.AsThought)))
                text_color = text_window_ds->GetCompatibleColor(guis[usingGui].GetFgColor());
            else
                text_color = text_window_ds->GetCompatibleColor(text_color);
        }
        else
        {
            text_color = text_window_ds->GetCompatibleColor(text_color);
        }

        // Print the lines of text
        for (size_t i = 0; i < Lines.Count(); ++i)
        {
            int ttyp = ttxtop + static_cast<int>(i) * disp.Linespacing;
            // if it's inside a text box then don't centre the text
            if (fix_look.Style == kDisplayTextStyle_TextWindow)
            {
                wouttext_aligned(text_window_ds, ttxleft, ttyp, oriwid, usingfont, text_color, Lines[i].GetCStr(), play.text_align);
            }
            else
            {
                wouttext_aligned(text_window_ds, ttxleft, ttyp, wii, usingfont, text_color, Lines[i].GetCStr(), play.speech_text_align);
            }
        }
    }
    else
    {
        // Textual overlay purposed for the standard message box
        int xoffs, yoffs, oriwid = wii - padding * 2;
        text_color = GUI::GetStandardColorForBitmap(15); // use fixed standard color here
        draw_text_window_and_bar(&text_window_ds, wantFreeScreenop, topbar, disp, &xoffs, &yoffs, &adjustedXX, &adjustedYY, &wii, &text_color);
        text_color = text_window_ds->GetCompatibleColor(text_color);

        adjust_y_coordinate_for_text(&yoffs, usingfont);

        for (size_t i = 0; i < Lines.Count(); ++i)
            wouttext_aligned(text_window_ds, xoffs, yoffs + static_cast<int>(i) * disp.Linespacing, oriwid, usingfont, text_color, Lines[i].GetCStr(), play.text_align);
    }
    return text_window_ds;
}

// Handles player's input during blocking display() call;
// returns if the display loop should break.
bool display_check_user_input(int skip)
{
    bool state_handled = false;
    for (InputType type = ags_inputevent_ready(); type != kInputNone; type = ags_inputevent_ready())
    { // NOTE: must handle them all in case there were engine's hotkeys too
        switch (type)
        {
        case kInputKeyboard:
        {
            KeyInput ki;
            if (!run_service_key_controls(ki) || play.fast_forward || state_handled)
                continue; // handled by engine layer, or fast-forwarded, or resolved
            if (check_skip_cutscene_keypress(ki.Key))
            {
                state_handled = true;
            }
            else if ((skip & SKIP_KEYPRESS) != 0 && !play.IsIgnoringInput() && !IsAGSServiceKey(ki.Key))
            {
                play.SetWaitKeySkip(ki);
                state_handled = true; // stop display
            }
            break;
        }
        case kInputMouse:
        {
            eAGSMouseButton mbut;
            if (!run_service_mb_controls(mbut) || play.fast_forward || state_handled)
                continue; // handled by engine layer, or fast-forwarded, or resolved
            if (check_skip_cutscene_mclick(mbut))
            {
                state_handled = true;
            }
            else if ((skip & SKIP_MOUSECLICK) != 0 && !play.IsIgnoringInput())
            {
                play.SetWaitSkipResult(SKIP_MOUSECLICK, mbut);
                state_handled = true; // stop display
            }
            break;
        }
        case kInputGamepad:
        {
            GamepadInput gbut;
            if (!run_service_gamepad_controls(gbut) || play.fast_forward || state_handled)
                continue; // handled by engine layer, or fast-forwarded, or resolved
            eAGSGamepad_Button gbn = gbut.Button;
            if (check_skip_cutscene_gamepad(gbn))
            {
                state_handled = true; // stop display
            }
            else if ((skip & SKIP_GAMEPAD) != 0 && !play.IsIgnoringInput() &&
                    is_default_gamepad_skip_button_pressed(gbn))
            {
                play.SetWaitSkipResult(SKIP_GAMEPAD, gbn);
                state_handled = true; // stop display
            }
            break;
        }
        case kInputTouch:
        {
            TouchInput ti;
            if (!run_service_touch_controls(ti) || play.fast_forward || state_handled)
                continue; // handled by engine layer, or fast-forwarded, or resolved
            // TODO: check skip cutscene? we might check if it's "skip by mouse" here
            if ((skip & SKIP_TOUCH) != 0 && (ti.Phase == TouchPhase::Down) && !play.IsIgnoringInput())
            {
                play.SetWaitSkipResult(SKIP_TOUCH);
                state_handled = true; // stop display
            }
            break;
        }
        default:
            ags_drop_next_inputevent();
            break;
        }
    }
    ags_check_mouse_wheel(); // poll always, otherwise it accumulates
    return state_handled;
}

ScreenOverlay *display_main(int xx, int yy, int wii, const char *text,
    const TopBarSettings *topbar, DisplayTextType disp_type, int over_id,
    const DisplayTextLooks &look, int usingfont, color_t text_color,
    int autoplace_at_char, bool roomlayer)
{
    //
    // Prepare for the message display
    //

    // if it's a normal message box and the game was being skipped,
    // ensure that the screen is up to date before the message box
    // is drawn on top of it
    // TODO: is this really necessary anymore?
    if ((play.skip_until_char_stops >= 0) && (disp_type == kDisplayText_MessageBox))
        render_graphics();

    // TODO: should this really be called regardless of message type?
    // _display_main may be called even for custom textual overlays
    EndSkippingUntilCharStops();

    if (topbar)
    {
        // the top bar should behave like DisplaySpeech wrt blocking (???)
        disp_type = kDisplayText_Speech;
    }

    if ((look.Style == kDisplayTextStyle_PlainText || look.Style == kDisplayTextStyle_Overchar)
        && (disp_type < kDisplayText_NormalOverlay))
    {
        // Update the GUI disabled state in advance of the adjust_x/y_for_guis calls;
        // this is done to avoid display box moved away from GUIs that will be hidden
        // FIXME: this is a misleading hack, find a way to do this without tweaking
        // play.disabled_user_interface.
        play.disabled_user_interface++;
        update_gui_disabled_status();
        play.disabled_user_interface--;
    }

    // Remove any previous blocking texts if necessary
    if (disp_type < kDisplayText_NormalOverlay)
        remove_screen_overlay(play.text_overlay_on);

    // If fast-forwarding, then skip any blocking message immediately
    if (play.fast_forward && (disp_type < kDisplayText_NormalOverlay)) {
        play.SetWaitSkipResult(SKIP_AUTOTIMER);
        post_display_cleanup();
        return nullptr;
    }

    //
    // Configure and create an overlay object
    //

    switch (disp_type)
    {
    case kDisplayText_MessageBox:    over_id = OVER_TEXTMSG; break;
    case kDisplayText_Speech:        over_id = OVER_TEXTSPEECH; break;
    case kDisplayText_NormalOverlay:
         // must be either OVER_CUSTOM flag or precreated overlay id
        assert(over_id == OVER_CUSTOM || over_id >= OVER_FIRSTFREE);
        if (over_id != OVER_CUSTOM && OVER_CUSTOM < OVER_FIRSTFREE)
            over_id = OVER_CUSTOM;
        break;
    }

    int adjustedXX, adjustedYY;
    Bitmap *text_window_ds = create_textual_image(text, look, text_color,
        xx, yy, adjustedXX, adjustedYY, wii, usingfont, topbar);

    size_t nse = add_screen_overlay(roomlayer, xx, yy, over_id, text_window_ds, adjustedXX - xx, adjustedYY - yy);
    auto *over = get_overlay(nse); // FIXME: optimize return value
    // we should not delete text_window_ds here, because it is now owned by Overlay

    if (autoplace_at_char >= 0) {
        over->SetAutoPosition(autoplace_at_char);
        autoposition_overlay(*over);
    }

    // If it's a non-blocking overlay type, then we're done here
    if (disp_type == kDisplayText_NormalOverlay) {
        return over;
    }

    //
    // Wait for the blocking text to timeout or until skipped by another command
    //

    if (disp_type == kDisplayText_MessageBox) {
        int countdown = GetTextDisplayTime(text);
        int skip_setting = user_to_internal_skip_speech((SkipSpeechStyle)play.skip_display);

        DisplayMessageState disp_state(OVER_TEXTMSG, countdown, skip_setting);
        disp_state.Begin();
        while (disp_state.Run());
        disp_state.End();
    }
    else { /* kDisplayText_Speech */
        // Run full game updates until overlay is removed
        GameLoopUntilNoOverlay();
    }

    //
    // Post-message cleanup
    //
    post_display_cleanup();
    return nullptr;
}

void display_at(int xx, int yy, int wii, const char *text, const TopBarSettings *topbar)
{
    EndSkippingUntilCharStops();
    // Start voice-over, if requested by the tokens in speech text
    try_auto_play_speech(text, text, play.narrator_speech);

    display_main(xx, yy, wii, text, topbar, kDisplayText_MessageBox, 0 /* no overid */,
        DisplayTextLooks(kDisplayTextStyle_MessageBox, get_textpos_from_scriptcoords(xx, yy, false), kDisplayTextShrink_Left),
        FONT_NORMAL, 0, -1 /* no autoplace */, false /* screen layer */);

    // Stop any blocking voice-over, if was started by this function
    if (play.IsBlockingVoiceSpeech())
        stop_voice_speech();
}

void post_display_cleanup()
{
    play.messagetime = -1;
    play.speech_in_post_state = false;
}

bool try_auto_play_speech(const char *text, const char *&replace_text, int charid)
{
    int voice_num;
    const char *src = parse_voiceover_token(text, &voice_num);
    if (src == text)
        return false; // no token
    
    if (voice_num <= 0)
        quit("DisplaySpeech: auto-voice symbol '&' not followed by valid integer");

    replace_text = src; // skip voice tag
    if (play_voice_speech(charid, voice_num))
    {
        // if Voice Only, then blank out the text
        if (play.speech_mode == kSpeech_VoiceOnly)
            replace_text = "  ";
        return true;
    }
    return false;
}

// TODO: refactor this global variable out; currently it is set at the every get_translation call.
// Be careful: a number of Say/Display functions expect it to be set beforehand.
int source_text_length = -1;

int GetTextDisplayLength(const char *text)
{
    // Skip voice-over token from the length calculation if required
    if (play.unfactor_speech_from_textlength != 0)
        text = skip_voiceover_token(text);
    return static_cast<int>(strlen(text));
}

// Calculates lipsync frame duration (or duration per character) in game loops.
// NOTE: historical formula was this:
//   loops_per_character = (((text_len / play.lipsync_speed) + 1) * fps) / text_len;
// But because of a precision loss due integer division this resulted in "jumping" values.
// The new formula uses float division, and coefficent found experimentally to make
// results match the old formula in certain key text lengths, for backwards compatibility.
int CalcLipsyncFrameDuration(int text_len, int fps)
{
    return static_cast<int>((((static_cast<float>(text_len) / play.lipsync_speed) + 0.75f) * fps) / text_len);
}

int GetTextDisplayTime(const char *text, int canberel) {
    int uselen = 0;
    auto fpstimer = ::lround(get_game_fps());

    // if it's background speech, make it stay relative to game speed
    if ((canberel == 1) && (play.bgspeech_game_speed == 1))
        fpstimer = 40; // NOTE: should be a fixed constant here, not game speed value

    if (source_text_length >= 0) {
        // sync to length of original text, to make sure any animations
        // and music sync up correctly
        uselen = source_text_length;
        source_text_length = -1;
    }
    else {
        uselen = GetTextDisplayLength(text);
    }

    if (uselen <= 0)
        return 0;

    if (play.text_speed + play.text_speed_modifier <= 0)
        quit("!Text speed is zero; unable to display text. Check your game.text_speed settings.");

    // Store how many game loops per character of text
    loops_per_character = CalcLipsyncFrameDuration(uselen, fpstimer);

    int textDisplayTimeInMS = ((uselen / (play.text_speed + play.text_speed_modifier)) + 1) * 1000;
    if (textDisplayTimeInMS < play.text_min_display_time_ms)
        textDisplayTimeInMS = play.text_min_display_time_ms;

    return (textDisplayTimeInMS * fpstimer) / 1000;
}

bool ShouldAntiAliasText()
{
    return (game.GetColorDepth() >= 24) && (game.options[OPT_ANTIALIASFONTS] != 0);
}

#if defined (AGS_FONTOUTLINE_MOREOPAQUE)
// TODO: was suggested by fernewelten, but it's unclear whether is necessary
// Make semi-transparent bits much more opaque
void wouttextxy_AutoOutline_Semitransparent2Opaque(Bitmap *map)
{
    if (map->GetColorDepth() < 32)
        return; // such maps don't feature partial transparency
    size_t const width = map->GetWidth();
    size_t const height = map->GetHeight();

    for (size_t y = 0; y < height; y++)
    {
        int32 *sc_line = reinterpret_cast<int32 *>(map->GetScanLineForWriting(y));
        for (size_t x = 0; x < width; x++)
        {
            int32 &px = sc_line[x];
            int const transparency = geta(px);
            if (0 < transparency && transparency < 255)
                px = makeacol32(
                    getr32(px), 
                    getg32(px), 
                    getb32(px), 
                    std::min(85 + transparency * 2, 255));
        }
    }
}
#endif

// Draw outline that is calculated from the text font, not derived from an outline font
static void wouttextxy_AutoOutline(Bitmap *ds, size_t font, int32_t color, BlendMode blend_mode, const char *text, int &x, int &y)
{
    const FontInfo &finfo = get_fontinfo(font);
    int const thickness = finfo.AutoOutlineThickness;
    auto const style = finfo.AutoOutlineStyle;
    if (thickness <= 0)
        return;

    // We use 32-bit stencils in any case when alpha-blending is required
    // because blending works correctly if there's an actual color
    // on the destination bitmap (and our intermediate bitmaps are transparent).
    int const  ds_cd = ds->GetColorDepth();
    bool const alpha_blend = (ds_cd == 32) && !is_bitmap_font(font) &&
        ((game.options[OPT_ANTIALIASFONTS] != 0) || (geta32(color) < 255) || (blend_mode != kBlend_Normal));
    int const  stencil_cd = alpha_blend ? 32 : ds_cd;

    const int t_width = get_text_width(text, font);
    const auto t_extent = get_font_surface_extent(font);
    const int t_height = t_extent.second - t_extent.first + 1;
    if (t_width == 0 || t_height == 0)
        return;
    // Prepare stencils
    const int t_yoff = t_extent.first;
    Bitmap *texx_stencil, *outline_stencil;
    alloc_font_outline_buffers(font, &texx_stencil, &outline_stencil,
        t_width, t_height, stencil_cd);
    texx_stencil->ClearTransparent();
    outline_stencil->ClearTransparent();
    // Ready text stencil
    // Note we are drawing with y off, in case some font's glyphs exceed font's ascender
    wouttextxy(texx_stencil, 0, -t_yoff, font, color, blend_mode, text);
#if defined (AGS_FONTOUTLINE_MOREOPAQUE)
    wouttextxy_AutoOutline_Semitransparent2Opaque(texx_stencil);
#endif
    // Anti-aliased TTFs require to be alpha-blended, not blit,
    // or the alpha values will be plain copied and final image will be broken.
    void(Bitmap::*pfn_drawstencil)(const Bitmap *src, int dst_x, int dst_y);
    if (alpha_blend)
    { // NOTE: we must set out blender AFTER wouttextxy, or it will be overidden
        SetBlender(blend_mode, 0xFF);
        pfn_drawstencil = &Bitmap::TransBlendBlt;
    }
    else
    {
        pfn_drawstencil = &Bitmap::MaskedBlit;
    }

    // move start of text so that the outline doesn't drop off the bitmap
    x += thickness;
    int const outline_y = y + t_yoff;
    y += thickness;

    // What we do here: first we paint text onto outline_stencil offsetting vertically;
    // then we paint resulting outline_stencil onto final dest offsetting horizontally.
    int largest_y_diff_reached_so_far = -1;
    for (int x_diff = thickness; x_diff >= 0; x_diff--)
    {
        // Integer arithmetics: In the following, we use terms k*(k + 1) to account for rounding.
        //     (k + 0.5)^2 == k*k + 2*k*0.5 + 0.5^2 == k*k + k + 0.25 ==approx. k*(k + 1)
        int y_term_limit = thickness * (thickness + 1);
        if (FontInfo::kRounded == style)
            y_term_limit -= x_diff * x_diff;

        // extend the outline stencil to the top and bottom
        for (int y_diff = largest_y_diff_reached_so_far + 1;
            y_diff <= thickness && y_diff * y_diff <= y_term_limit;
            y_diff++)
        {
            (outline_stencil->*pfn_drawstencil)(texx_stencil, 0, thickness - y_diff);
            if (y_diff > 0)
                (outline_stencil->*pfn_drawstencil)(texx_stencil, 0, thickness + y_diff);
            largest_y_diff_reached_so_far = y_diff;
        }

        // stamp the outline stencil to the left and right of the text
        (ds->*pfn_drawstencil)(outline_stencil, x - x_diff, outline_y);
        if (x_diff > 0)
            (ds->*pfn_drawstencil)(outline_stencil, x + x_diff, outline_y);
    }
}

// Draw an outline if requested, then draw the text on top
void wouttext_outline(Bitmap *ds, int x, int y, int font, color_t text_color, color_t outline_color, BlendMode blend_mode, const char *text)
{    
    size_t const text_font = static_cast<size_t>(font);
    // Draw outline (a backdrop) if requested
    int const outline_font = get_font_outline(font);
    if (outline_font >= 0)
        wouttextxy(ds, x, y, static_cast<size_t>(outline_font), outline_color, blend_mode, text);
    else if (outline_font == FONT_OUTLINE_AUTO)
        wouttextxy_AutoOutline(ds, text_font, outline_color, blend_mode, text, x, y);
    else
        ; // no outline

    // FIXME: there's a problem with auto-outlining while using translucent text color:
    // because auto-outlining is implemented by drawing same text with another color,
    // and then drawing actual text on top, the translucent text will appear not above
    // background, but above *outline*, which results in combining text + outline alphas.
    // For a correct *automatic* behavior, we'd have to somehow clear the outline pixels
    // right below the text pixels, or use blender variants that discard dest alpha.

    // Draw text on top
    wouttextxy(ds, x, y, text_font, text_color, blend_mode, text);
}

void wouttext_outline(Common::Bitmap *ds, int x, int y, int font, color_t text_color, const char *text)
{
    const color_t outline_color = ds->GetCompatibleColor(play.speech_text_shadow);
    wouttext_outline(ds, x, y, font, text_color, outline_color, kBlend_Normal, text);
}

void wouttext_outline(Bitmap *ds, int x, int y, int font, color_t text_color, BlendMode blend_mode, const char *text)
{
    const color_t outline_color = ds->GetCompatibleColor(play.speech_text_shadow);
    wouttext_outline(ds, x, y, font, text_color, outline_color, blend_mode, text);
}

void wouttext_aligned(Bitmap *ds, int x, int y, int oriwid, int usingfont, color_t text_color, color_t outline_color, const char *text, HorAlignment align)
{
    if (align & kMAlignHCenter)
        x = x + (oriwid / 2) - (get_text_width_outlined(text, usingfont) / 2);
    else if (align & kMAlignRight)
        x = x + (oriwid - get_text_width_outlined(text, usingfont));

    wouttext_outline(ds, x, y, usingfont, text_color, outline_color, kBlend_Normal, text);
}

void wouttext_aligned(Bitmap *ds, int x, int y, int oriwid, int usingfont, color_t text_color, const char *text, HorAlignment align)
{
    int const outline_color = ds->GetCompatibleColor(play.speech_text_shadow);
    wouttext_aligned(ds, x, y, oriwid, usingfont, text_color, outline_color, text, align);
}

void do_corner(Bitmap *ds, int sprn, int x, int y, int offx, int offy) {
    if (sprn<0) return;
    if (!spriteset.DoesSpriteExist(sprn))
    {
        sprn = 0;
    }

    x = x + offx * game.SpriteInfos[sprn].Width;
    y = y + offy * game.SpriteInfos[sprn].Height;
    draw_gui_sprite(ds, sprn, x, y);
}

int get_but_pic(GUIMain*guo, GUITextWindowBorder indx)
{
    int butid = guo->GetControlID(indx);
    return butid >= 0 ? guibuts[butid].GetNormalImage() : 0;
}

void draw_button_background(Bitmap *ds, int xx1,int yy1,int xx2,int yy2,GUIMain*iep)
{
    if (iep == nullptr)
    {
        // Standard window
        color_t draw_color = GUI::GetStandardColorForBitmap(15);
        ds->FillRect(Rect(xx1,yy1,xx2,yy2), draw_color);
        draw_color = GUI::GetStandardColorForBitmap(16);
        ds->DrawRect(Rect(xx1,yy1,xx2,yy2), draw_color);
    }
    else
    {
        // Custom text window
        if (iep->GetBgColor() != 0)
            ds->FillRect(Rect(xx1,yy1,xx2,yy2), MakeColor(iep->GetBgColor()));

        const int leftRightWidth = game.SpriteInfos[get_but_pic(iep, kTW_Left)].Width;
        const int topBottomHeight = game.SpriteInfos[get_but_pic(iep, kTW_Top)].Height;
        // GUI middle space
        if (iep->GetBgImage()>0)
        {
            // offset the background image and clip it so that it is drawn
            // such that the border graphics can have a transparent outside
            // edge
            int bgoffsx = xx1 - leftRightWidth / 2;
            int bgoffsy = yy1 - topBottomHeight / 2;
            ds->SetClip(Rect(bgoffsx, bgoffsy, xx2 + leftRightWidth / 2, yy2 + topBottomHeight / 2));
            int bgfinishx = xx2;
            int bgfinishy = yy2;
            int bgoffsyStart = bgoffsy;
            while (bgoffsx <= bgfinishx)
            {
                bgoffsy = bgoffsyStart;
                while (bgoffsy <= bgfinishy)
                {
                    draw_gui_sprite(ds, iep->GetBgImage(), bgoffsx, bgoffsy);
                    bgoffsy += game.SpriteInfos[iep->GetBgImage()].Height;
                }
                bgoffsx += game.SpriteInfos[iep->GetBgImage()].Width;
            }
            // return to normal clipping rectangle
            ds->ResetClip();
        }
        // Vertical borders
        ds->SetClip(Rect(xx1 - leftRightWidth, yy1, xx2 + 1 + leftRightWidth, yy2));
        for (int uu=yy1;uu <= yy2;uu+= game.SpriteInfos[get_but_pic(iep, kTW_Left)].Height)
        {
            do_corner(ds, get_but_pic(iep, kTW_Left),xx1,uu,-1,0);   // left side
            do_corner(ds, get_but_pic(iep, kTW_Right),xx2+1,uu,0,0);  // right side
        }
        // Horizontal borders
        ds->SetClip(Rect(xx1, yy1 - topBottomHeight, xx2, yy2 + 1 + topBottomHeight));
        for (int uu=xx1;uu <= xx2;uu+=game.SpriteInfos[get_but_pic(iep, kTW_Top)].Width)
        {
            do_corner(ds, get_but_pic(iep, kTW_Top),uu,yy1,0,-1);  // top side
            do_corner(ds, get_but_pic(iep, kTW_Bottom),uu,yy2+1,0,0); // bottom side
        }
        ds->ResetClip();
        // Four corners
        do_corner(ds, get_but_pic(iep, kTW_TopLeft),xx1,yy1,-1,-1);
        do_corner(ds, get_but_pic(iep, kTW_BottomLeft),xx1,yy2+1,-1,0);
        do_corner(ds, get_but_pic(iep, kTW_TopRight),xx2+1,yy1,0,-1);
        do_corner(ds, get_but_pic(iep, kTW_BottomRight),xx2+1,yy2+1,0,0);
    }
}

// Calculate the width that the left and right border of the textwindow
// GUI take up
int get_textwindow_border_width(int twgui)
{
    if (twgui < 0)
        return 0;

    if (!guis[twgui].IsTextWindow())
    {
        debug_script_warn("GUI %d is set as text window but is not actually a text window GUI", twgui);
        return 0;
    }

    int borwid = game.SpriteInfos[get_but_pic(&guis[twgui], kTW_Left)].Width +
        game.SpriteInfos[get_but_pic(&guis[twgui], kTW_Right)].Width;

    return borwid;
}

// get the hegiht of the text window's top border
int get_textwindow_top_border_height(int twgui)
{
    if (twgui < 0)
        return 0;

    if (!guis[twgui].IsTextWindow())
    {
        debug_script_warn("GUI %d is set as text window but is not actually a text window GUI", twgui);
        return 0;
    }

    return game.SpriteInfos[get_but_pic(&guis[twgui], kTW_Top)].Height;
}

// Get the padding for a text window
// -1 for the game's custom text window
int get_textwindow_padding(int ifnum) {
    int result;

    if (ifnum < 0)
        ifnum = game.options[OPT_TWCUSTOM];
    if (ifnum > 0 && ifnum < game.numgui)
        result = guis[ifnum].GetPadding();
    else
        result = TEXTWINDOW_PADDING_DEFAULT;

    return result;
}

void draw_text_window(Bitmap **text_window_ds, bool should_free_ds,
                      int*xins,int*yins,int*xx,int*yy,int*wii, color_t *set_text_color,
                      int ovrheight, int ifnum, const DisplayVars &disp)
{
    assert(text_window_ds);
    Bitmap *ds = *text_window_ds;
    if (ifnum < 0)
        ifnum = game.options[OPT_TWCUSTOM];

    // Assertions
    if (ifnum >= game.numgui)
    {
        debug_script_warn("Invalid GUI %d specified as text window (valid range: 1..%d)", ifnum, game.numgui);
        ifnum = 0;
    }
    else if (!guis[ifnum].IsTextWindow())
    {
        debug_script_warn("GUI %d is set as text window but is not actually a text window GUI", ifnum);
        ifnum = 0;
    }

    if (ifnum <= 0)
    {
        draw_button_background(ds, 0,0,ds->GetWidth() - 1,ds->GetHeight() - 1,nullptr);
        if (set_text_color)
            *set_text_color = GUI::GetStandardColor(16);
        xins[0]=3;
        yins[0]=3;
    }
    else
    {
        int tbnum = get_but_pic(&guis[ifnum], kTW_TopLeft);

        wii[0] += get_textwindow_border_width (ifnum);
        xx[0]-=game.SpriteInfos[tbnum].Width;
        yy[0]-=game.SpriteInfos[tbnum].Height;
        if (ovrheight == 0)
            ovrheight = disp.FullTextHeight;

        if (should_free_ds)
            delete *text_window_ds;
        int padding = get_textwindow_padding(ifnum);
        *text_window_ds = BitmapHelper::CreateTransparentBitmap(wii[0],ovrheight+(padding*2)+ game.SpriteInfos[tbnum].Height*2,game.GetColorDepth());
        ds = *text_window_ds;
        int xoffs=game.SpriteInfos[tbnum].Width,yoffs= game.SpriteInfos[tbnum].Height;
        draw_button_background(ds, xoffs,yoffs,(ds->GetWidth() - xoffs) - 1,(ds->GetHeight() - yoffs) - 1,&guis[ifnum]);
        if (set_text_color)
            *set_text_color = guis[ifnum].GetFgColor();
        xins[0]=xoffs+padding;
        yins[0]=yoffs+padding;
    }
}

void draw_text_window_and_bar(Bitmap **text_window_ds, bool should_free_ds,
                              const TopBarSettings *topbar, const DisplayVars &disp,
                              int*xins,int*yins,int*xx,int*yy,int*wii,color_t *set_text_color,int ovrheight, int ifnum) {

    assert(text_window_ds);
    draw_text_window(text_window_ds, should_free_ds, xins, yins, xx, yy, wii, set_text_color, ovrheight, ifnum, disp);

    if ((topbar) && (text_window_ds && *text_window_ds)) {
        // top bar on the dialog window with character's name
        // create an enlarged window, then free the old one
        Bitmap *ds = *text_window_ds;
        Bitmap *newScreenop = BitmapHelper::CreateBitmap(ds->GetWidth(), ds->GetHeight() + topbar->Height, game.GetColorDepth());
        newScreenop->Blit(ds, 0, 0, 0, topbar->Height, ds->GetWidth(), ds->GetHeight());
        delete *text_window_ds;
        *text_window_ds = newScreenop;
        ds = *text_window_ds;

        // draw the top bar
        color_t draw_color = ds->GetCompatibleColor(play.top_bar_backcolor);
        ds->FillRect(Rect(0, 0, ds->GetWidth() - 1, topbar->Height - 1), draw_color);
        if (play.top_bar_backcolor != play.top_bar_bordercolor) {
            // draw the border
            draw_color = ds->GetCompatibleColor(play.top_bar_bordercolor);
            for (int j = 0; j < play.top_bar_borderwidth; j++)
                ds->DrawRect(Rect(j, j, ds->GetWidth() - (j + 1), topbar->Height - (j + 1)), draw_color);
        }

        // draw the text
        int textx = (ds->GetWidth() / 2) - get_text_width_outlined(topbar->Text.GetCStr(), topbar->Font) / 2;
        color_t text_color = ds->GetCompatibleColor(play.top_bar_textcolor);
        wouttext_outline(ds, textx, play.top_bar_borderwidth + 1, topbar->Font, text_color, topbar->Text.GetCStr());

        // adjust the text Y position
        yins[0] += topbar->Height;
    }
}
