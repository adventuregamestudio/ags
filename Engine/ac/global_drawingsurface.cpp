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

#include <stdio.h>
#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_drawingsurface.h"
#include "ac/global_translation.h"
#include "ac/roomstruct.h"
#include "ac/string.h"
#include "debug/debug_log.h"
#include "font/fonts.h"
#include "gui/guidefines.h"
#include "ac/spritecache.h"
#include "gfx/gfx_util.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;
namespace GfxUtil = AGS::Engine::GfxUtil;

extern Bitmap *raw_saved_screen;
extern roomstruct thisroom;
extern GameState play;
extern char lines[MAXLINE][200];
extern int  numlines;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern GameSetupStruct game;
extern int current_screen_resolution_multiplier;

// Raw screen writing routines - similar to old CapturedStuff
//#define RAW_START() Bitmap *oldabuf=RAW_GRAPHICS()->GetBitmap(); RAW_GRAPHICS()->GetBitmap()=thisroom.ebscene[play.bg_frame]; play.raw_modified[play.bg_frame] = 1
//#define RAW_END() RAW_GRAPHICS()->GetBitmap() = oldabuf
#define RAW_START() raw_drawing_surface = thisroom.ebscene[play.bg_frame]; play.raw_modified[play.bg_frame] = 1
#define RAW_END()
#define RAW_SURFACE() (raw_drawing_surface)

Bitmap *raw_drawing_surface;

// RawSaveScreen: copy the current screen to a backup bitmap
void RawSaveScreen () {
    if (raw_saved_screen != NULL)
        delete raw_saved_screen;
    Bitmap *source = thisroom.ebscene[play.bg_frame];
    raw_saved_screen = BitmapHelper::CreateBitmapCopy(source);
}
// RawRestoreScreen: copy backup bitmap back to screen; we
// deliberately don't free the Bitmap *cos they can multiple restore
// and it gets freed on room exit anyway
void RawRestoreScreen() {
    if (raw_saved_screen == NULL) {
        debug_log("RawRestoreScreen: unable to restore, since the screen hasn't been saved previously.");
        return;
    }
    Bitmap *deston = thisroom.ebscene[play.bg_frame];
    deston->Blit(raw_saved_screen, 0, 0, 0, 0, deston->GetWidth(), deston->GetHeight());
    invalidate_screen();
    mark_current_background_dirty();
}
// Restores the backup bitmap, but tints it to the specified level
void RawRestoreScreenTinted(int red, int green, int blue, int opacity) {
    if (raw_saved_screen == NULL) {
        debug_log("RawRestoreScreenTinted: unable to restore, since the screen hasn't been saved previously.");
        return;
    }
    if ((red < 0) || (green < 0) || (blue < 0) ||
        (red > 255) || (green > 255) || (blue > 255) ||
        (opacity < 1) || (opacity > 100))
        quit("!RawRestoreScreenTinted: invalid parameter. R,G,B must be 0-255, opacity 1-100");

    DEBUG_CONSOLE("RawRestoreTinted RGB(%d,%d,%d) %d%%", red, green, blue, opacity);

    Bitmap *deston = thisroom.ebscene[play.bg_frame];
    tint_image(deston, raw_saved_screen, red, green, blue, opacity);
    invalidate_screen();
    mark_current_background_dirty();
}

void RawDrawFrameTransparent (int frame, int translev) {
    if ((frame < 0) || (frame >= thisroom.num_bscenes) ||
        (translev < 0) || (translev > 99))
        quit("!RawDrawFrameTransparent: invalid parameter (transparency must be 0-99, frame a valid BG frame)");

    if (thisroom.ebscene[frame]->GetColorDepth() <= 8)
        quit("!RawDrawFrameTransparent: 256-colour backgrounds not supported");

    if (frame == play.bg_frame)
        quit("!RawDrawFrameTransparent: cannot draw current background onto itself");

    RAW_START();
    if (translev == 0)
    {
        // just draw it over the top, no transparency
        RAW_SURFACE()->Blit(thisroom.ebscene[frame], 0, 0, 0, 0, thisroom.ebscene[frame]->GetWidth(), thisroom.ebscene[frame]->GetHeight());
    }
    else
    {
        // Draw it transparently
        GfxUtil::DrawSpriteWithTransparency (RAW_SURFACE(), thisroom.ebscene[frame], 0, 0,
            GfxUtil::Trans100ToAlpha255(translev));
    }
    invalidate_screen();
    mark_current_background_dirty();
    RAW_END();
}

void RawClear (int clr) {
    RAW_START();
    clr = RAW_SURFACE()->GetCompatibleColor(clr);
    RAW_SURFACE()->Clear (clr);
    invalidate_screen();
    mark_current_background_dirty();
}
void RawSetColor (int clr) {
    //push_screen();
    //SetVirtualScreen(thisroom.ebscene[play.bg_frame]);
    // set the colour at the appropriate depth for the background
    play.raw_color = GetVirtualScreen()->GetCompatibleColor(clr);
    //pop_screen();
}
void RawSetColorRGB(int red, int grn, int blu) {
    if ((red < 0) || (red > 255) || (grn < 0) || (grn > 255) ||
        (blu < 0) || (blu > 255))
        quit("!RawSetColorRGB: colour values must be 0-255");

    play.raw_color = makecol_depth(thisroom.ebscene[play.bg_frame]->GetColorDepth(), red, grn, blu);
}
void RawPrint (int xx, int yy, const char *text) {
    RAW_START();
    // don't use wtextcolor because it will do a 16->32 conversion
    color_t text_color = play.raw_color;
    if ((RAW_SURFACE()->GetColorDepth() <= 8) && (play.raw_color > 255)) {
        text_color = RAW_SURFACE()->GetCompatibleColor(1);
        debug_log ("RawPrint: Attempted to use hi-color on 256-col background");
    }
    multiply_up_coordinates(&xx, &yy);
    wouttext_outline(RAW_SURFACE(), xx, yy, play.normal_font, text_color, text);
    // we must invalidate the entire screen because these are room
    // co-ordinates, not screen co-ords which it works with
    invalidate_screen();
    mark_current_background_dirty();
    RAW_END();
}
void RawPrintMessageWrapped (int xx, int yy, int wid, int font, int msgm) {
    char displbuf[3000];
    int texthit = wgetfontheight(font);
    multiply_up_coordinates(&xx, &yy);
    wid = multiply_up_coordinate(wid);

    get_message_text (msgm, displbuf);
    // it's probably too late but check anyway
    if (strlen(displbuf) > 2899)
        quit("!RawPrintMessageWrapped: message too long");
    break_up_text_into_lines (wid, font, displbuf);

    RAW_START();
    color_t text_color = play.raw_color;
    for (int i = 0; i < numlines; i++)
        wouttext_outline(RAW_SURFACE(), xx, yy + texthit*i, font, text_color, lines[i]);
    invalidate_screen();
    mark_current_background_dirty();
    RAW_END();
}

void RawDrawImageCore(int xx, int yy, int slot, int alpha) {
    if ((slot < 0) || (slot >= MAX_SPRITES) || (spriteset[slot] == NULL))
        quit("!RawDrawImage: invalid sprite slot number specified");
    RAW_START();

    if (spriteset[slot]->GetColorDepth() != RAW_SURFACE()->GetColorDepth()) {
        debug_log("RawDrawImage: Sprite %d colour depth %d-bit not same as background depth %d-bit", slot, spriteset[slot]->GetColorDepth(), RAW_SURFACE()->GetColorDepth());
    }

    draw_sprite_slot_support_alpha(RAW_SURFACE(), false, xx, yy, slot, alpha);
    invalidate_screen();
    mark_current_background_dirty();
    RAW_END();
}

void RawDrawImage(int xx, int yy, int slot) {
    multiply_up_coordinates(&xx, &yy);
    RawDrawImageCore(xx, yy, slot);
}

void RawDrawImageTrans(int xx, int yy, int slot, int alpha) {
    multiply_up_coordinates(&xx, &yy);
    RawDrawImageCore(xx, yy, slot, alpha);
}

void RawDrawImageOffset(int xx, int yy, int slot) {

    if ((current_screen_resolution_multiplier == 1) && game.IsHiRes()) {
        // running a 640x400 game at 320x200, adjust
        xx /= 2;
        yy /= 2;
    }
    else if ((current_screen_resolution_multiplier > 1) && !game.IsHiRes()) {
        // running a 320x200 game at 640x400, adjust
        xx *= 2;
        yy *= 2;
    }

    RawDrawImageCore(xx, yy, slot);
}

void RawDrawImageTransparent(int xx, int yy, int slot, int legacy_transparency) {
    if ((legacy_transparency < 0) || (legacy_transparency > 100))
        quit("!RawDrawImageTransparent: invalid transparency setting");

    // WARNING: the previous versions of AGS actually had a bug:
    // although manual stated that RawDrawImageTransparent takes % of transparency
    // as an argument, that value was used improperly when setting up an Allegro's
    // trans_blender, which caused it to act about as % of opacity instead, but
    // with a twist.
    //
    // It was converted to 255-ranged "transparency" parameter:
    // int transparency = (trans * 255) / 100;
    //
    // Note by CJ:
    // Transparency is a bit counter-intuitive
    // 0=not transparent, 255=invisible, 1..254 barely visible .. mostly visible
    //
    // In order to support this backward-compatible behavior, we convert the
    // opacity into proper alpha this way:
    // 0      => alpha 255
    // 100    => alpha 0
    // 1 - 99 => alpha 1 - 244
    // 
    RawDrawImageTrans(xx, yy, slot, GfxUtil::LegacyTrans100ToAlpha255(legacy_transparency));

    update_polled_stuff_if_runtime();  // this operation can be slow so stop music skipping
}
void RawDrawImageResized(int xx, int yy, int gotSlot, int width, int height) {
    if ((gotSlot < 0) || (gotSlot >= MAX_SPRITES) || (spriteset[gotSlot] == NULL))
        quit("!RawDrawImageResized: invalid sprite slot number specified");
    // very small, don't draw it
    if ((width < 1) || (height < 1))
        return;

    multiply_up_coordinates(&xx, &yy);
    multiply_up_coordinates(&width, &height);

    // resize the sprite to the requested size
    Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, spriteset[gotSlot]->GetColorDepth());
    newPic->StretchBlt(spriteset[gotSlot],
        RectWH(0, 0, spritewidth[gotSlot], spriteheight[gotSlot]),
        RectWH(0, 0, width, height));

    RAW_START();
    if (newPic->GetColorDepth() != RAW_SURFACE()->GetColorDepth())
        quit("!RawDrawImageResized: image colour depth mismatch: the background image must have the same colour depth as the sprite being drawn");

    GfxUtil::DrawSpriteWithTransparency(RAW_SURFACE(), newPic, xx, yy);
    delete newPic;
    invalidate_screen();
    mark_current_background_dirty();
    update_polled_stuff_if_runtime();  // this operation can be slow so stop music skipping
    RAW_END();
}
void RawDrawLine (int fromx, int fromy, int tox, int toy) {
    multiply_up_coordinates(&fromx, &fromy);
    multiply_up_coordinates(&tox, &toy);

    play.raw_modified[play.bg_frame] = 1;
    int ii,jj;
    // draw a line thick enough to look the same at all resolutions
    Bitmap *bg_frame = thisroom.ebscene[play.bg_frame];
    color_t draw_color = play.raw_color;
    for (ii = 0; ii < get_fixed_pixel_size(1); ii++) {
        for (jj = 0; jj < get_fixed_pixel_size(1); jj++)
            bg_frame->DrawLine (Line(fromx+ii, fromy+jj, tox+ii, toy+jj), draw_color);
    }
    invalidate_screen();
    mark_current_background_dirty();
}
void RawDrawCircle (int xx, int yy, int rad) {
    multiply_up_coordinates(&xx, &yy);
    rad = multiply_up_coordinate(rad);

    play.raw_modified[play.bg_frame] = 1;
    Bitmap *bg_frame = thisroom.ebscene[play.bg_frame];
    bg_frame->FillCircle(Circle (xx, yy, rad), play.raw_color);
    invalidate_screen();
    mark_current_background_dirty();
}
void RawDrawRectangle(int x1, int y1, int x2, int y2) {
    play.raw_modified[play.bg_frame] = 1;
    multiply_up_coordinates(&x1, &y1);
    multiply_up_coordinates_round_up(&x2, &y2);

    Bitmap *bg_frame = thisroom.ebscene[play.bg_frame];
    bg_frame->FillRect(Rect(x1,y1,x2,y2), play.raw_color);
    invalidate_screen();
    mark_current_background_dirty();
}
void RawDrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3) {
    play.raw_modified[play.bg_frame] = 1;
    multiply_up_coordinates(&x1, &y1);
    multiply_up_coordinates(&x2, &y2);
    multiply_up_coordinates(&x3, &y3);

    Bitmap *bg_frame = thisroom.ebscene[play.bg_frame];
    bg_frame->DrawTriangle(Triangle (x1,y1,x2,y2,x3,y3), play.raw_color);
    invalidate_screen();
    mark_current_background_dirty();
}
