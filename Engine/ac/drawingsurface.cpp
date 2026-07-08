//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/drawingsurface.h"
#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/walkbehind.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "debug/debug_log.h"
#include "font/fonts.h"
#include "gfx/gfx_def.h"
#include "gfx/gfx_util.h"
#include "gui/guimain.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern RoomStatus*croom;
extern RoomObject*objs;
extern SpriteCache spriteset;

// ** SCRIPT DRAWINGSURFACE OBJECT

void DrawingSurface_Release(ScriptDrawingSurface* sds)
{
    if (sds->roomBackgroundNumber >= 0)
    {
        on_room_bg_surface_release(sds->roomBackgroundNumber, sds->modified);
        sds->roomBackgroundNumber = -1;
    }
    else if (sds->roomMaskType > kRoomAreaNone)
    {
        on_room_mask_surface_release(sds->roomMaskType, sds->modified);
        sds->roomMaskType = kRoomAreaNone;
    }
    else if (sds->dynamicSpriteNumber >= 0)
    {
        on_dynsprite_surface_release(sds->dynamicSpriteNumber, sds->modified);
        sds->dynamicSpriteNumber = -1;
    }
    else if (sds->dynamicSurfaceNumber >= 0)
    {
        dynamicallyCreatedSurfaces[sds->dynamicSurfaceNumber] = nullptr;
        sds->dynamicSurfaceNumber = -1;
    }
    sds->modified = 0;
}

void ScriptDrawingSurface::PointToGameResolution(int *xcoord, int *ycoord)
{
    ctx_data_to_game_coord(*xcoord, *ycoord, highResCoordinates != 0);
}

void ScriptDrawingSurface::SizeToGameResolution(int *width, int *height)
{
    ctx_data_to_game_size(*width, *height, highResCoordinates != 0);
}

void ScriptDrawingSurface::SizeToGameResolution(int *valueToAdjust)
{
    *valueToAdjust = ctx_data_to_game_size(*valueToAdjust, highResCoordinates != 0);
}

// convert actual co-ordinate back to what the script is expecting
void ScriptDrawingSurface::SizeToDataResolution(int *valueToAdjust)
{
    *valueToAdjust = game_to_ctx_data_size(*valueToAdjust, highResCoordinates != 0);
}

Bitmap *GetAndAssertBitmapSurface(ScriptDrawingSurface *sds, const char *api_name)
{
    Bitmap *bmp = sds->GetBitmapSurface();
    if (!bmp)
    {
        debug_script_warn("%s: attempted to use surface after its source image was disposed or DrawingSurface.Release() was called.",
            api_name);
    }
    return bmp;
}

Bitmap *TryStartDrawing(ScriptDrawingSurface* sds, const char *api_name)
{
    Bitmap *bmp = sds->StartDrawing();
    if (!bmp)
    {
        debug_script_warn("%s: attempted to use surface after its source image was disposed or DrawingSurface.Release() was called.",
            api_name);
    }
    return bmp;
}

ScriptDrawingSurface* DrawingSurface_CreateCopy(ScriptDrawingSurface *sds)
{
    Bitmap *sourceBitmap = GetAndAssertBitmapSurface(sds, "DrawingSurface.CreateCopy");
    if (!sourceBitmap)
        return nullptr;

    for (int i = 0; i < MAX_DYNAMIC_SURFACES; i++)
    {
        if (dynamicallyCreatedSurfaces[i] == nullptr)
        {
            dynamicallyCreatedSurfaces[i].reset(BitmapHelper::CreateBitmapCopy(sourceBitmap));
            ScriptDrawingSurface *newSurface = new ScriptDrawingSurface();
            newSurface->dynamicSurfaceNumber = i;
            newSurface->hasAlphaChannel = sds->hasAlphaChannel;
            ccRegisterManagedObject(newSurface, newSurface);
            return newSurface;
        }
    }

    debug_script_warn("!DrawingSurface.CreateCopy: too many surface copies created (max is %d)", MAX_DYNAMIC_SURFACES);
    return nullptr;
}

void DrawingSurface_DrawImageImpl(ScriptDrawingSurface* sds, Bitmap* src,
    int dst_x, int dst_y, int trans, int dst_width, int dst_height,
    int src_x, int src_y, int src_width, int src_height, int sprite_id, bool src_has_alpha)
{
    Bitmap *ds = GetAndAssertBitmapSurface(sds, "DrawingSurface.DrawImage");
    if (!ds)
        return;

    if (src == ds) {} // ignore for now; bitmap lib supports, and may be used for effects
        /* debug_script_warn("DrawingSurface.DrawImage: drawing onto itself"); */
    if (src->GetColorDepth() != ds->GetColorDepth())
    {
        if (sprite_id >= 0)
            debug_script_warn("DrawImage: Sprite %d colour depth %d-bit not same as destination depth %d-bit", sprite_id, src->GetColorDepth(), ds->GetColorDepth());
        else
            debug_script_warn("DrawImage: Source image colour depth %d-bit not same as destination depth %d-bit", src->GetColorDepth(), ds->GetColorDepth());
    }
    if ((trans < 0) || (trans > 100))
        debug_script_warn("DrawingSurface.DrawImage: invalid transparency %d, range is %d - %d", trans, 0, 100);
    trans = Math::Clamp(trans, 0, 100);

    if (trans == 100)
        return; // fully transparent
    if (dst_width < 1 || dst_height < 1 || src_width < 1 || src_height < 1)
        return; // invalid src or dest rectangles

    // Setup uninitialized arguments; convert coordinates for legacy script mode
    if (dst_width == SCR_NO_VALUE) { dst_width = src->GetWidth(); }
    else { sds->SizeToGameResolution(&dst_width); }
    if (dst_height == SCR_NO_VALUE) { dst_height = src->GetHeight(); }
    else { sds->SizeToGameResolution(&dst_height); }

    if (src_x == SCR_NO_VALUE) { src_x = 0; }
    if (src_y == SCR_NO_VALUE) { src_y = 0; }
    sds->PointToGameResolution(&src_x, &src_y);
    if (src_width == SCR_NO_VALUE) { src_width = src->GetWidth(); }
    else { sds->SizeToGameResolution(&src_width); }
    if (src_height == SCR_NO_VALUE) { src_height = src->GetHeight(); }
    else { sds->SizeToGameResolution(&src_height); }

    sds->PointToGameResolution(&dst_x, &dst_y);

    if (dst_x >= ds->GetWidth() || dst_x + dst_width <= 0 || dst_y >= ds->GetHeight() || dst_y + dst_height <= 0 ||
        src_x >= src->GetWidth() || src_x + src_width <= 0 || src_y >= src->GetHeight() || src_y + src_height <= 0)
        return; // source or destination rects lie completely off surface
    // Clamp the source rect to the valid limits to prevent exceptions (ignore dest, bitmap drawing deals with that)
    Math::ClampLength(src_x, src_width, 0, src->GetWidth());
    Math::ClampLength(src_y, src_height, 0, src->GetHeight());

    // TODO: possibly optimize by not making a stretched intermediate bitmap
    // if simplier blit/draw_sprite could be called (no translucency with alpha channel).
    std::unique_ptr<Bitmap> conv_src;
    if (dst_width != src->GetWidth() || dst_height != src->GetHeight() ||
        src_width != src->GetWidth() || src_height != src->GetHeight())
    {
        // Resize and/or partial copy specified
        conv_src.reset(BitmapHelper::CreateBitmap(dst_width, dst_height, src->GetColorDepth()));
        conv_src->StretchBlt(src,
            RectWH(src_x, src_y, src_width, src_height),
            RectWH(0, 0, dst_width, dst_height));

        src = conv_src.get();
    }

    ds = sds->StartDrawing();

    draw_sprite_support_alpha(ds, sds->hasAlphaChannel != 0, dst_x, dst_y, src, src_has_alpha,
        kBlendMode_Alpha, GfxDef::Trans100ToAlpha255(trans));

    sds->FinishedDrawing();
}

void DrawingSurface_DrawImage(ScriptDrawingSurface* sds,
    int dst_x, int dst_y, int slot, int trans,
    int dst_width, int dst_height,
    int src_x, int src_y, int src_width, int src_height)
{
    if (!spriteset.DoesSpriteExist(slot))
    {
        debug_script_warn("DrawingSurface.DrawImage: invalid sprite slot number specified: %d", slot);
        slot = 0;
    }
    DrawingSurface_DrawImageImpl(sds, spriteset[slot], dst_x, dst_y, trans, dst_width, dst_height,
        src_x, src_y, src_width, src_height, slot, (game.SpriteInfos[slot].Flags & SPF_ALPHACHANNEL) != 0);
}

void DrawingSurface_DrawImage6(ScriptDrawingSurface* sds, int xx, int yy, int slot, int trans, int width, int height)
{
    DrawingSurface_DrawImage(sds, xx, yy, slot, trans, width, height, 0, 0, SCR_NO_VALUE, SCR_NO_VALUE);
}

void DrawingSurface_DrawSurface(ScriptDrawingSurface* target, ScriptDrawingSurface* source, int trans,
    int dst_x, int dst_y, int dst_width, int dst_height,
    int src_x, int src_y, int src_width, int src_height)
{
    Bitmap *source_bmp = GetAndAssertBitmapSurface(source, "DrawingSurface.DrawSurface");
    if (!source_bmp)
        return;

    DrawingSurface_DrawImageImpl(target, source_bmp, dst_x, dst_y, trans, dst_width, dst_height,
        src_x, src_y, src_width, src_height, -1, source->hasAlphaChannel != 0);
}

void DrawingSurface_DrawSurface2(ScriptDrawingSurface* target, ScriptDrawingSurface* source, int trans)
{
    DrawingSurface_DrawSurface(target, source, trans, 0, 0, SCR_NO_VALUE, SCR_NO_VALUE, 0, 0, SCR_NO_VALUE, SCR_NO_VALUE);
}

void DrawingSurface_SetDrawingColor(ScriptDrawingSurface *sds, int newColour) 
{
    Bitmap *ds = GetAndAssertBitmapSurface(sds, "DrawingSurface.DrawingColor");
    if (!ds)
        return;
    sds->currentColourScript = newColour;
    if (newColour == SCR_COLOR_TRANSPARENT)
    {
        sds->currentColour = ds->GetMaskColor();
    }
    else
    {
        sds->currentColour = ds->GetCompatibleColor(newColour);
    }
}

int DrawingSurface_GetDrawingColor(ScriptDrawingSurface *sds)
{
    return sds->currentColourScript;
}

void DrawingSurface_SetUseHighResCoordinates(ScriptDrawingSurface *sds, int highRes) 
{
    if (game.AllowRelativeRes())
        sds->highResCoordinates = (highRes) ? 1 : 0;
}

int DrawingSurface_GetUseHighResCoordinates(ScriptDrawingSurface *sds) 
{
    return sds->highResCoordinates;
}

int DrawingSurface_GetHeight(ScriptDrawingSurface *sds) 
{
    Bitmap *ds = GetAndAssertBitmapSurface(sds, "DrawingSurface.Height");
    if (!ds)
        return 0;
    int height = ds->GetHeight();
    sds->SizeToGameResolution(&height);
    return height;
}

bool DrawingSurface_GetValid(ScriptDrawingSurface *sds) 
{
    return sds->IsValid();
}

int DrawingSurface_GetWidth(ScriptDrawingSurface *sds) 
{
    Bitmap *ds = GetAndAssertBitmapSurface(sds, "DrawingSurface.Width");
    if (!ds)
        return 0;
    int width = ds->GetWidth();
    sds->SizeToGameResolution(&width);
    return width;
}

int DrawingSurface_GetColorDepth(ScriptDrawingSurface *sds) 
{
    Bitmap *ds = GetAndAssertBitmapSurface(sds, "DrawingSurface.ColorDepth");
    return ds ? ds->GetColorDepth() : 0;
}

void DrawingSurface_Clear(ScriptDrawingSurface *sds, int colour)
{
    Bitmap *ds = TryStartDrawing(sds, "DrawingSurface.Clear");
    if (!ds)
        return;
    int allegroColor;
    if ((colour == -SCR_NO_VALUE) || (colour == SCR_COLOR_TRANSPARENT))
    {
        allegroColor = ds->GetMaskColor();
    }
    else
    {
        allegroColor = ds->GetCompatibleColor(colour);
    }
    ds->Fill(allegroColor);
    sds->FinishedDrawing();
}

void DrawingSurface_DrawCircle(ScriptDrawingSurface *sds, int x, int y, int radius)
{
    sds->PointToGameResolution(&x, &y);
    sds->SizeToGameResolution(&radius);

    Bitmap *ds = TryStartDrawing(sds, "DrawingSurface.DrawCircle");
    if (!ds)
        return;
    ds->FillCircle(Circle(x, y, radius), sds->currentColour);
    sds->FinishedDrawing();
}

void DrawingSurface_DrawRectangle(ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2)
{
    sds->PointToGameResolution(&x1, &y1);
    sds->PointToGameResolution(&x2, &y2);

    Bitmap *ds = TryStartDrawing(sds, "DrawingSurface.DrawRectangle");
    if (!ds)
        return;
    ds->FillRect(Rect(x1,y1,x2,y2), sds->currentColour);
    sds->FinishedDrawing();
}

void DrawingSurface_DrawTriangle(ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2, int x3, int y3)
{
    sds->PointToGameResolution(&x1, &y1);
    sds->PointToGameResolution(&x2, &y2);
    sds->PointToGameResolution(&x3, &y3);

    Bitmap *ds = TryStartDrawing(sds, "DrawingSurface.DrawTriangle");
    if (!ds)
        return;
    ds->DrawTriangle(Triangle(x1,y1,x2,y2,x3,y3), sds->currentColour);
    sds->FinishedDrawing();
}

void DrawingSurface_DrawString(ScriptDrawingSurface *sds, int xx, int yy, int font, const char* text)
{
    sds->PointToGameResolution(&xx, &yy);
    Bitmap *ds = TryStartDrawing(sds, "DrawingSurface.DrawString");
    if (!ds)
        return;
    color_t text_color = sds->currentColour;
    if ((ds->GetColorDepth() <= 8) && (text_color > 255)) {
        text_color = ds->GetCompatibleColor(1);
        debug_script_warn ("DrawingSurface.DrawString: Attempted to use color %d on 256-col surface", text_color);
    }
    String res_str = GUI::ApplyTextDirection(text);
    wouttext_outline(ds, xx, yy, font, text_color, res_str.GetCStr());
    sds->FinishedDrawing();
}

void DrawingSurface_DrawStringWrapped_Old(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int alignment, const char *msg) {
    DrawingSurface_DrawStringWrapped(sds, xx, yy, wid, font, ConvertLegacyScriptAlignment((LegacyScriptAlignment)alignment), msg);
}

void DrawingSurface_DrawStringWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int alignment, const char *msg) {
    int linespacing = get_font_linespacing(font);
    sds->PointToGameResolution(&xx, &yy);
    sds->SizeToGameResolution(&wid);

    const char *draw_text = skip_voiceover_token(msg);
    if (break_up_text_into_lines(draw_text, Lines, wid, font) == 0)
        return;

    Bitmap *ds = TryStartDrawing(sds, "DrawingSurface.DrawStringWrapped");
    if (!ds)
        return;
    color_t text_color = sds->currentColour;
    color_t outline_color = ds->GetCompatibleColor(play.speech_text_shadow);

    for (size_t i = 0; i < Lines.Count(); i++)
    {
        GUI::DrawTextAlignedHor(ds, Lines[i], font, text_color, outline_color,
            xx, xx + wid - 1, yy + linespacing*i, (FrameAlignment)alignment);
    }

    sds->FinishedDrawing();
}

void DrawingSurface_DrawMessageWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int msgm)
{
    char displbuf[3000];
    get_message_text(msgm, displbuf);
    // it's probably too late but check anyway
    if (strlen(displbuf) > 2899)
        quit("!RawPrintMessageWrapped: message too long");

    DrawingSurface_DrawStringWrapped_Old(sds, xx, yy, wid, font, kLegacyScAlignLeft, displbuf);
}

void DrawingSurface_DrawLine(ScriptDrawingSurface *sds, int fromx, int fromy, int tox, int toy, int thickness) {
    sds->PointToGameResolution(&fromx, &fromy);
    sds->PointToGameResolution(&tox, &toy);
    sds->SizeToGameResolution(&thickness);
    int ii,jj,xx,yy;
    Bitmap *ds = TryStartDrawing(sds, "DrawingSurface.DrawLine");
    if (!ds)
        return;
    // draw several lines to simulate the thickness
    color_t draw_color = sds->currentColour;
    for (ii = 0; ii < thickness; ii++) 
    {
        xx = (ii - (thickness / 2));
        for (jj = 0; jj < thickness; jj++)
        {
            yy = (jj - (thickness / 2));
            ds->DrawLine (Line(fromx + xx, fromy + yy, tox + xx, toy + yy), draw_color);
        }
    }
    sds->FinishedDrawing();
}

void DrawingSurface_DrawPixel(ScriptDrawingSurface *sds, int x, int y) {
    sds->PointToGameResolution(&x, &y);
    int thickness = 1;
    sds->SizeToGameResolution(&thickness);
    int ii,jj;
    Bitmap *ds = TryStartDrawing(sds, "DrawingSurface.DrawPixel");
    if (!ds)
        return;
    // draw several pixels to simulate the thickness
    color_t draw_color = sds->currentColour;
    for (ii = 0; ii < thickness; ii++) 
    {
        for (jj = 0; jj < thickness; jj++)
        {
            ds->PutPixel(x + ii, y + jj, draw_color);
        }
    }
    sds->FinishedDrawing();
}

int DrawingSurface_GetPixel(ScriptDrawingSurface *sds, int x, int y) {
    sds->PointToGameResolution(&x, &y);
    Bitmap *ds = GetAndAssertBitmapSurface(sds, "DrawingSurface.GetPixel");
    if (!ds)
        return 0;
    int rawPixel = ds->GetPixel(x, y);
    int maskColor = ds->GetMaskColor();
    int colDepth = ds->GetColorDepth();

    if (rawPixel == maskColor)
    {
        rawPixel = SCR_COLOR_TRANSPARENT;
    }
    else if (colDepth > 8)
    {
        int r = getr_depth(colDepth, rawPixel);
        int g = getg_depth(colDepth, rawPixel);
        int b = getb_depth(colDepth, rawPixel);
        rawPixel = Game_GetColorFromRGB(r, g, b);
    }

    return rawPixel;
}

void *DrawingSurface_GetPixelsCopyImpl(ScriptDrawingSurface *sds, int x, int y, int width, int height, uint8_t dest_elem_size)
{
    Bitmap *ds = GetAndAssertBitmapSurface(sds, "DrawingSurface.GetPixelsCopy");
    if (!ds)
        return nullptr;

    if (dest_elem_size != sizeof(uint8_t) && dest_elem_size != ds->GetBPP())
    {
        debug_script_warn("DrawingSurface.GetPixelsCopy: invalid destination array type for %d-bit image", ds->GetColorDepth());
        return nullptr;
    }
    if (x < 0 || y < 0 || x >= ds->GetWidth() || y >= ds->GetHeight())
    {
        debug_script_warn("DrawingSurface.GetPixelsCopy: invalid region %d,%d %dx%d", x, y, width, height);
        return nullptr;
    }

    if (width < 0)
        width = ds->GetWidth();
    if (height < 0)
        height = ds->GetHeight();
    width = std::min(width, ds->GetWidth() - x);
    height = std::min(height, ds->GetHeight() - y);
    auto arr = DynamicArrayHelpers::CreateArray(dest_elem_size, (width * height * ds->GetBPP()) / dest_elem_size);
    if (arr)
    {
        const int src_pitch = ds->GetWidth() * ds->GetBPP();
        const int dst_pitch = width * ds->GetBPP();
        PixelOp::CopyPixelsRegion(ds->GetData() + y * src_pitch, ds->GetBPP(), src_pitch,
            x, width, height, static_cast<uint8_t*>(arr.Obj()), dst_pitch, 0u);
    }
    return arr.Obj();
}

void *DrawingSurface_GetPixelsCopy(ScriptDrawingSurface *sds, int x, int y, int width, int height)
{
    return DrawingSurface_GetPixelsCopyImpl(sds, x, y, width, height, sizeof(uint8_t));
}

void *DrawingSurface_GetPixelsCopy16(ScriptDrawingSurface *sds, int x, int y, int width, int height)
{
    return DrawingSurface_GetPixelsCopyImpl(sds, x, y, width, height, sizeof(uint16_t));
}

void *DrawingSurface_GetPixelsCopy32(ScriptDrawingSurface *sds, int x, int y, int width, int height)
{
    return DrawingSurface_GetPixelsCopyImpl(sds, x, y, width, height, sizeof(uint32_t));
}

void DrawingSurface_SetPixelsImpl(ScriptDrawingSurface *sds, void *arrobj, int x, int y, int width, int height, uint8_t dest_elem_size)
{
    Bitmap *ds = GetAndAssertBitmapSurface(sds, "DrawingSurface.GetPixels");
    if (!ds)
        return;
    if (arrobj == nullptr)
        return;

    if (dest_elem_size != sizeof(uint8_t) && dest_elem_size != ds->GetBPP())
    {
        debug_script_warn("DrawingSurface.SetPixels: invalid source array type for %d-bit image", ds->GetColorDepth());
        return;
    }
    if (x < 0 || y < 0 || x >= ds->GetWidth() || y >= ds->GetHeight())
    {
        debug_script_warn("DrawingSurface.SetPixels: invalid region %d,%d", x, y);
        return;
    }

    if (width < 0)
        width = ds->GetWidth();
    if (height < 0)
        height = ds->GetHeight();
    const auto &arr_header = CCDynamicArray::GetHeader(arrobj);
    const uint8_t *arr_ptr = static_cast<uint8_t*>(arrobj);
    const int src_pitch = width * ds->GetBPP();
    const int dst_pitch = ds->GetWidth() * ds->GetBPP();
    const int copy_width = std::min(width, ds->GetWidth() - x);
    const int copy_height = std::min(height, std::min<int>(arr_header.TotalSize / (src_pitch), ds->GetHeight()));
    PixelOp::CopyPixelsRegion(arr_ptr, ds->GetBPP(), src_pitch, 0u, copy_width, copy_height,
        ds->GetDataForWriting() + y * dst_pitch, dst_pitch, x);
    sds->FinishedDrawing();
}

void DrawingSurface_SetPixels(ScriptDrawingSurface *sds, void *arrobj, int x, int y, int width, int height)
{
    DrawingSurface_SetPixelsImpl(sds, arrobj, x, y, width, height, sizeof(uint8_t));
}

void DrawingSurface_SetPixels16(ScriptDrawingSurface *sds, void *arrobj, int x, int y, int width, int height)
{
    DrawingSurface_SetPixelsImpl(sds, arrobj, x, y, width, height, sizeof(uint16_t));
}

void DrawingSurface_SetPixels32(ScriptDrawingSurface *sds, void *arrobj, int x, int y, int width, int height)
{
    DrawingSurface_SetPixelsImpl(sds, arrobj, x, y, width, height, sizeof(uint32_t));
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// void (ScriptDrawingSurface *sds, int colour)
RuntimeScriptValue Sc_DrawingSurface_Clear(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDrawingSurface, DrawingSurface_Clear);
}

// ScriptDrawingSurface* (ScriptDrawingSurface *sds)
RuntimeScriptValue Sc_DrawingSurface_CreateCopy(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptDrawingSurface, ScriptDrawingSurface, DrawingSurface_CreateCopy);
}

// void (ScriptDrawingSurface *sds, int x, int y, int radius)
RuntimeScriptValue Sc_DrawingSurface_DrawCircle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptDrawingSurface, DrawingSurface_DrawCircle);
}

// void (ScriptDrawingSurface* sds, int xx, int yy, int slot, int trans, int width, int height)
RuntimeScriptValue Sc_DrawingSurface_DrawImage6(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT6(ScriptDrawingSurface, DrawingSurface_DrawImage6);
}

RuntimeScriptValue Sc_DrawingSurface_DrawImage(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_OBJ_PARAM_COUNT(METHOD, 10);
    DrawingSurface_DrawImage((ScriptDrawingSurface*)self, params[0].IValue, params[1].IValue, params[2].IValue, params[3].IValue, params[4].IValue, params[5].IValue,
        params[6].IValue, params[7].IValue, params[8].IValue, params[9].IValue);
    return RuntimeScriptValue((int32_t)0);
}

// void (ScriptDrawingSurface *sds, int fromx, int fromy, int tox, int toy, int thickness)
RuntimeScriptValue Sc_DrawingSurface_DrawLine(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptDrawingSurface, DrawingSurface_DrawLine);
}

// void (ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int msgm)
RuntimeScriptValue Sc_DrawingSurface_DrawMessageWrapped(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptDrawingSurface, DrawingSurface_DrawMessageWrapped);
}

// void (ScriptDrawingSurface *sds, int x, int y)
RuntimeScriptValue Sc_DrawingSurface_DrawPixel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptDrawingSurface, DrawingSurface_DrawPixel);
}

// void (ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2)
RuntimeScriptValue Sc_DrawingSurface_DrawRectangle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptDrawingSurface, DrawingSurface_DrawRectangle);
}

// void (ScriptDrawingSurface *sds, int xx, int yy, int font, const char* texx, ...)
RuntimeScriptValue Sc_DrawingSurface_DrawString(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(DrawingSurface_DrawString, 4);
    DrawingSurface_DrawString((ScriptDrawingSurface*)self, params[0].IValue, params[1].IValue, params[2].IValue, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

// void (ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int alignment, const char *msg)
RuntimeScriptValue Sc_DrawingSurface_DrawStringWrapped_Old(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5_POBJ(ScriptDrawingSurface, DrawingSurface_DrawStringWrapped_Old, const char);
}

RuntimeScriptValue Sc_DrawingSurface_DrawStringWrapped(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(DrawingSurface_DrawString, 6);
    DrawingSurface_DrawStringWrapped((ScriptDrawingSurface*)self, params[0].IValue, params[1].IValue, params[2].IValue,
        params[3].IValue, params[4].IValue, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

// void (ScriptDrawingSurface* target, ScriptDrawingSurface* source, int translev)
RuntimeScriptValue Sc_DrawingSurface_DrawSurface2(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT(ScriptDrawingSurface, DrawingSurface_DrawSurface2, ScriptDrawingSurface);
}

RuntimeScriptValue Sc_DrawingSurface_DrawSurface(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_OBJ_PARAM_COUNT(METHOD, 10);
    DrawingSurface_DrawSurface((ScriptDrawingSurface*)self, (ScriptDrawingSurface*)params[0].Ptr,
        params[1].IValue, params[2].IValue, params[3].IValue, params[4].IValue, params[5].IValue,
        params[6].IValue, params[7].IValue, params[8].IValue, params[9].IValue);
    return RuntimeScriptValue((int32_t)0);
}

// void (ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2, int x3, int y3)
RuntimeScriptValue Sc_DrawingSurface_DrawTriangle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT6(ScriptDrawingSurface, DrawingSurface_DrawTriangle);
}

// int (ScriptDrawingSurface *sds, int x, int y)
RuntimeScriptValue Sc_DrawingSurface_GetPixel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT2(ScriptDrawingSurface, DrawingSurface_GetPixel);
}

RuntimeScriptValue Sc_DrawingSurface_GetPixelsCopy(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT4(ScriptDrawingSurface, void, globalDynamicArray, DrawingSurface_GetPixelsCopy);
}

RuntimeScriptValue Sc_DrawingSurface_GetPixelsCopy16(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT4(ScriptDrawingSurface, void, globalDynamicArray, DrawingSurface_GetPixelsCopy16);
}

RuntimeScriptValue Sc_DrawingSurface_GetPixelsCopy32(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT4(ScriptDrawingSurface, void, globalDynamicArray, DrawingSurface_GetPixelsCopy32);
}

RuntimeScriptValue Sc_DrawingSurface_SetPixels(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT4(ScriptDrawingSurface, DrawingSurface_SetPixels, void);
}

RuntimeScriptValue Sc_DrawingSurface_SetPixels16(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT4(ScriptDrawingSurface, DrawingSurface_SetPixels16, void);
}

RuntimeScriptValue Sc_DrawingSurface_SetPixels32(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT4(ScriptDrawingSurface, DrawingSurface_SetPixels32, void);
}

// void (ScriptDrawingSurface* sds)
RuntimeScriptValue Sc_DrawingSurface_Release(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptDrawingSurface, DrawingSurface_Release);
}

// int (ScriptDrawingSurface *sds)
RuntimeScriptValue Sc_DrawingSurface_GetDrawingColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetDrawingColor);
}

// void (ScriptDrawingSurface *sds, int newColour)
RuntimeScriptValue Sc_DrawingSurface_SetDrawingColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDrawingSurface, DrawingSurface_SetDrawingColor);
}

// int (ScriptDrawingSurface *sds)
RuntimeScriptValue Sc_DrawingSurface_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetHeight);
}

// int (ScriptDrawingSurface *sds)
RuntimeScriptValue Sc_DrawingSurface_GetUseHighResCoordinates(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetUseHighResCoordinates);
}

// void (ScriptDrawingSurface *sds, int highRes)
RuntimeScriptValue Sc_DrawingSurface_SetUseHighResCoordinates(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDrawingSurface, DrawingSurface_SetUseHighResCoordinates);
}

RuntimeScriptValue Sc_DrawingSurface_GetValid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptDrawingSurface, DrawingSurface_GetValid);
}

// int (ScriptDrawingSurface *sds)
RuntimeScriptValue Sc_DrawingSurface_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetWidth);
}

RuntimeScriptValue Sc_DrawingSurface_GetColorDepth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetColorDepth);
}

//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

void ScPl_DrawingSurface_DrawString(ScriptDrawingSurface *sds, int xx, int yy, int font, const char* texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    DrawingSurface_DrawString(sds, xx, yy, font, scsf_buffer);
}

void ScPl_DrawingSurface_DrawStringWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int alignment, const char *msg, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(msg);
    DrawingSurface_DrawStringWrapped(sds, xx, yy, wid, font, alignment, scsf_buffer);
}

void RegisterDrawingSurfaceAPI(ScriptAPIVersion base_api, ScriptAPIVersion /*compat_api*/)
{
    ScFnRegister drawsurf_api[] = {
        { "DrawingSurface::Clear^1",              API_FN_PAIR(DrawingSurface_Clear) },
        { "DrawingSurface::CreateCopy^0",         API_FN_PAIR(DrawingSurface_CreateCopy) },
        { "DrawingSurface::DrawCircle^3",         API_FN_PAIR(DrawingSurface_DrawCircle) },
        { "DrawingSurface::DrawImage^6",          API_FN_PAIR(DrawingSurface_DrawImage6) },
        { "DrawingSurface::DrawImage^10",         API_FN_PAIR(DrawingSurface_DrawImage) },
        { "DrawingSurface::DrawLine^5",           API_FN_PAIR(DrawingSurface_DrawLine) },
        { "DrawingSurface::DrawMessageWrapped^5", API_FN_PAIR(DrawingSurface_DrawMessageWrapped) },
        { "DrawingSurface::DrawPixel^2",          API_FN_PAIR(DrawingSurface_DrawPixel) },
        { "DrawingSurface::DrawRectangle^4",      API_FN_PAIR(DrawingSurface_DrawRectangle) },
        { "DrawingSurface::DrawString^104",       Sc_DrawingSurface_DrawString, ScPl_DrawingSurface_DrawString },
        { "DrawingSurface::DrawSurface^2",        API_FN_PAIR(DrawingSurface_DrawSurface2) },
        { "DrawingSurface::DrawSurface^10",       API_FN_PAIR(DrawingSurface_DrawSurface) },
        { "DrawingSurface::DrawTriangle^6",       API_FN_PAIR(DrawingSurface_DrawTriangle) },
        { "DrawingSurface::GetPixel^2",           API_FN_PAIR(DrawingSurface_GetPixel) },
        { "DrawingSurface::GetPixelsCopy^4",      API_FN_PAIR(DrawingSurface_GetPixelsCopy) },
        { "DrawingSurface::GetPixelsCopy16^4",    API_FN_PAIR(DrawingSurface_GetPixelsCopy16) },
        { "DrawingSurface::GetPixelsCopy32^4",    API_FN_PAIR(DrawingSurface_GetPixelsCopy32) },
        { "DrawingSurface::SetPixels^5",          API_FN_PAIR(DrawingSurface_SetPixels) },
        { "DrawingSurface::SetPixels16^5",        API_FN_PAIR(DrawingSurface_SetPixels16) },
        { "DrawingSurface::SetPixels32^5",        API_FN_PAIR(DrawingSurface_SetPixels32) },
        { "DrawingSurface::Release^0",            API_FN_PAIR(DrawingSurface_Release) },
        { "DrawingSurface::get_ColorDepth",       API_FN_PAIR(DrawingSurface_GetColorDepth) },
        { "DrawingSurface::get_DrawingColor",     API_FN_PAIR(DrawingSurface_GetDrawingColor) },
        { "DrawingSurface::set_DrawingColor",     API_FN_PAIR(DrawingSurface_SetDrawingColor) },
        { "DrawingSurface::get_Height",           API_FN_PAIR(DrawingSurface_GetHeight) },
        { "DrawingSurface::get_UseHighResCoordinates", API_FN_PAIR(DrawingSurface_GetUseHighResCoordinates) },
        { "DrawingSurface::set_UseHighResCoordinates", API_FN_PAIR(DrawingSurface_SetUseHighResCoordinates) },
        { "DrawingSurface::get_Valid",            API_FN_PAIR(DrawingSurface_GetValid) },
        { "DrawingSurface::get_Width",            API_FN_PAIR(DrawingSurface_GetWidth) },
    };

    ccAddExternalFunctions(drawsurf_api);

    // Few functions have to be selected based on API level
    if (base_api < kScriptAPI_v350)
    {
        ccAddExternalObjectFunction("DrawingSurface::DrawStringWrapped^6", API_FN_PAIR(DrawingSurface_DrawStringWrapped_Old));
    }
    else
    { // old non-variadic and new variadic variants
        ccAddExternalObjectFunction("DrawingSurface::DrawStringWrapped^6", API_FN_PAIR(DrawingSurface_DrawStringWrapped));
        ccAddExternalObjectFunction("DrawingSurface::DrawStringWrapped^106", Sc_DrawingSurface_DrawStringWrapped, (void*)ScPl_DrawingSurface_DrawStringWrapped);
    }
}
