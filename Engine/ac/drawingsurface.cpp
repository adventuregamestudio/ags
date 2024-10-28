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
#include "ac/draw.h"
#include "ac/drawingsurface.h"
#include "ac/common.h"
#include "ac/display.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/string.h"
#include "ac/walkbehind.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/debug_log.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "ac/spritecache.h"
#include "script/runtimescriptvalue.h"
#include "gfx/gfx_def.h"
#include "gfx/gfx_util.h"

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
        if (sds->modified)
        {
            if (sds->roomBackgroundNumber == play.bg_frame)
            {
                invalidate_screen();
                mark_current_background_dirty();
            }
            play.raw_modified[sds->roomBackgroundNumber] = 1;
        }

        sds->roomBackgroundNumber = -1;
    }
    if (sds->roomMaskType > kRoomAreaNone)
    {
        if (sds->roomMaskType == kRoomAreaWalkBehind)
        {
            walkbehinds_recalc();
        }
        sds->roomMaskType = kRoomAreaNone;
    }
    if (sds->dynamicSpriteNumber >= 0)
    {
        if (sds->modified)
        {
            game_sprite_updated(sds->dynamicSpriteNumber);
        }

        sds->dynamicSpriteNumber = -1;
    }
    if (sds->dynamicSurfaceNumber >= 0)
    {
        dynamicallyCreatedSurfaces[sds->dynamicSurfaceNumber] = nullptr;
        sds->dynamicSurfaceNumber = -1;
    }
    sds->modified = 0;
}

ScriptDrawingSurface* DrawingSurface_CreateCopy(ScriptDrawingSurface *sds)
{
    Bitmap *sourceBitmap = sds->GetBitmapSurface();

    for (int i = 0; i < MAX_DYNAMIC_SURFACES; i++)
    {
        if (dynamicallyCreatedSurfaces[i] == nullptr)
        {
            dynamicallyCreatedSurfaces[i].reset(BitmapHelper::CreateBitmapCopy(sourceBitmap));
            ScriptDrawingSurface *newSurface = new ScriptDrawingSurface();
            newSurface->dynamicSurfaceNumber = i;
            ccRegisterManagedObject(newSurface, newSurface);
            return newSurface;
        }
    }

    quit("!DrawingSurface.CreateCopy: too many copied surfaces created");
    return nullptr;
}

static void DrawingSurface_DrawImageImpl(ScriptDrawingSurface* sds, Bitmap* src,
    int dst_x, int dst_y, int trans, BlendMode mode, int dst_width, int dst_height,
    int src_x, int src_y, int src_width, int src_height, int sprite_id)
{
    Bitmap *ds = sds->GetBitmapSurface();
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

    // Setup uninitialized arguments
    if (dst_width == SCR_NO_VALUE) { dst_width = src->GetWidth(); }
    if (dst_height == SCR_NO_VALUE) { dst_height = src->GetHeight(); }

    if (src_x == SCR_NO_VALUE) { src_x = 0; }
    if (src_y == SCR_NO_VALUE) { src_y = 0; }
    if (src_width == SCR_NO_VALUE) { src_width = src->GetWidth(); }
    if (src_height == SCR_NO_VALUE) { src_height = src->GetHeight(); }

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

    draw_sprite_support_alpha(ds, dst_x, dst_y, src, mode, GfxDef::Trans100ToAlpha255(trans));

    sds->FinishedDrawing();
}

void DrawingSurface_DrawImage(ScriptDrawingSurface* sds,
    int dst_x, int dst_y, int slot, int trans,
    int dst_width, int dst_height,
    int src_x, int src_y, int src_width, int src_height)
{
    if ((slot < 0) || (!spriteset.DoesSpriteExist(slot)))
        quit("!DrawingSurface.DrawImage: invalid sprite slot number specified");
    DrawingSurface_DrawImageImpl(sds, spriteset[slot], dst_x, dst_y, trans, kBlend_Normal, dst_width, dst_height,
        src_x, src_y, src_width, src_height, slot);
}

void DrawingSurface_BlendImage(ScriptDrawingSurface* sds,
    int dst_x, int dst_y, int slot, BlendMode mode, int trans,
    int dst_width, int dst_height,
    int src_x, int src_y, int src_width, int src_height)
{
    if ((slot < 0) || (spriteset[slot] == nullptr))
        quit("!DrawingSurface.DrawImage: invalid sprite slot number specified");
    DrawingSurface_DrawImageImpl(sds, spriteset[slot], dst_x, dst_y, trans, mode, dst_width, dst_height,
        src_x, src_y, src_width, src_height, slot);
}

void DrawingSurface_DrawSurface(ScriptDrawingSurface* target, ScriptDrawingSurface* source, int trans,
    int dst_x, int dst_y, int dst_width, int dst_height,
    int src_x, int src_y, int src_width, int src_height)
{
    DrawingSurface_DrawImageImpl(target, source->GetBitmapSurface(), dst_x, dst_y, trans, kBlend_Normal, dst_width, dst_height,
        src_x, src_y, src_width, src_height, -1);
}

void DrawingSurface_BlendSurface(ScriptDrawingSurface* target, ScriptDrawingSurface* source, BlendMode mode, int trans,
    int dst_x, int dst_y, int dst_width, int dst_height,
    int src_x, int src_y, int src_width, int src_height)
{
    DrawingSurface_DrawImageImpl(target, source->GetBitmapSurface(), dst_x, dst_y, trans, mode, dst_width, dst_height,
        src_x, src_y, src_width, src_height, -1);
}

void DrawingSurface_SetDrawingColor(ScriptDrawingSurface *sds, int newColour) 
{
    sds->currentColourScript = newColour;
    // StartDrawing to set up ds to set the colour at the appropriate
    // depth for the background
    Bitmap *ds = sds->StartDrawing();
    if (newColour == SCR_COLOR_TRANSPARENT)
    {
        sds->currentColour = ds->GetMaskColor();
    }
    else
    {
        sds->currentColour = ds->GetCompatibleColor(newColour);
    }
    sds->FinishedDrawingReadOnly();
}

int DrawingSurface_GetDrawingColor(ScriptDrawingSurface *sds)
{
    return sds->currentColourScript;
}

int DrawingSurface_GetHeight(ScriptDrawingSurface *sds) 
{
    Bitmap *ds = sds->GetBitmapSurface();
    return ds ? ds->GetHeight() : 0;
}

int DrawingSurface_GetWidth(ScriptDrawingSurface *sds) 
{
    Bitmap *ds = sds->GetBitmapSurface();
    return ds ? ds->GetWidth() : 0;
}

int DrawingSurface_GetColorDepth(ScriptDrawingSurface *sds) 
{
    Bitmap *ds = sds->GetBitmapSurface();
    return ds ? ds->GetColorDepth() : 0;
}

void DrawingSurface_Clear(ScriptDrawingSurface *sds, int colour)
{
    Bitmap *ds = sds->StartDrawing();
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
    Bitmap *ds = sds->StartDrawing();
    ds->FillCircle(Circle(x, y, radius), sds->currentColour);
    sds->FinishedDrawing();
}

void DrawingSurface_DrawRectangle(ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2)
{
    Bitmap *ds = sds->StartDrawing();
    ds->FillRect(Rect(x1,y1,x2,y2), sds->currentColour);
    sds->FinishedDrawing();
}

void DrawingSurface_DrawTriangle(ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2, int x3, int y3)
{
    Bitmap *ds = sds->StartDrawing();
    ds->DrawTriangle(Triangle(x1,y1,x2,y2,x3,y3), sds->currentColour);
    sds->FinishedDrawing();
}

void DrawingSurface_DrawString(ScriptDrawingSurface *sds, int xx, int yy, int font, const char* text)
{
    Bitmap *ds = sds->StartDrawing();
    color_t text_color = sds->currentColour;
    if ((ds->GetColorDepth() <= 8) && (text_color > 255)) {
        text_color = GUI::GetStandardColorForBitmap(1);
        debug_script_warn ("DrawingSurface.DrawString: Attempted to use color %d on 256-col surface", text_color);
    }
    String res_str = GUI::ApplyTextDirection(text);
    wouttext_outline(ds, xx, yy, font, text_color, res_str.GetCStr());
    sds->FinishedDrawing();
}

void DrawingSurface_DrawStringWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int alignment, const char *msg) {
    int linespacing = get_font_linespacing(font);

    const char *draw_text = skip_voiceover_token(msg);
    if (break_up_text_into_lines(draw_text, Lines, wid, font) == 0)
        return;

    Bitmap *ds = sds->StartDrawing();
    color_t text_color = sds->currentColour;

    for (size_t i = 0; i < Lines.Count(); i++)
    {
        GUI::DrawTextAlignedHor(ds, Lines[i], font, text_color,
            xx, xx + wid - 1, yy + linespacing*i, (FrameAlignment)alignment);
    }

    sds->FinishedDrawing();
}

void DrawingSurface_DrawLine(ScriptDrawingSurface *sds, int fromx, int fromy, int tox, int toy, int thickness) {
    int ii,jj,xx,yy;
    Bitmap *ds = sds->StartDrawing();
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
    int thickness = 1;
    int ii,jj;
    Bitmap *ds = sds->StartDrawing();
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
    Bitmap *ds = sds->StartDrawing();
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

    sds->FinishedDrawingReadOnly();
    return rawPixel;
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

RuntimeScriptValue Sc_DrawingSurface_DrawImage(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_OBJ_PARAM_COUNT(METHOD, 10);
    DrawingSurface_DrawImage((ScriptDrawingSurface*)self, params[0].IValue, params[1].IValue, params[2].IValue,
        params[3].IValue, params[4].IValue, params[5].IValue,
        params[6].IValue, params[7].IValue, params[8].IValue, params[9].IValue);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_DrawingSurface_BlendImage(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_OBJ_PARAM_COUNT(METHOD, 11);
    DrawingSurface_BlendImage((ScriptDrawingSurface*)self, params[0].IValue, params[1].IValue, params[2].IValue,
        (BlendMode)params[3].IValue, params[4].IValue,
        params[5].IValue, params[6].IValue,
        params[7].IValue, params[8].IValue, params[9].IValue, params[10].IValue);
    return RuntimeScriptValue((int32_t)0);
}

// void (ScriptDrawingSurface *sds, int fromx, int fromy, int tox, int toy, int thickness)
RuntimeScriptValue Sc_DrawingSurface_DrawLine(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptDrawingSurface, DrawingSurface_DrawLine);
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

RuntimeScriptValue Sc_DrawingSurface_DrawStringWrapped(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(DrawingSurface_DrawString, 6);
    DrawingSurface_DrawStringWrapped((ScriptDrawingSurface*)self, params[0].IValue, params[1].IValue, params[2].IValue,
        params[3].IValue, params[4].IValue, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_DrawingSurface_DrawSurface(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_OBJ_PARAM_COUNT(METHOD, 10);
    DrawingSurface_DrawSurface((ScriptDrawingSurface*)self, (ScriptDrawingSurface*)params[0].Ptr,
        params[1].IValue, params[2].IValue, params[3].IValue, params[4].IValue, params[5].IValue,
        params[6].IValue, params[7].IValue, params[8].IValue, params[9].IValue);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_DrawingSurface_BlendSurface(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_OBJ_PARAM_COUNT(METHOD, 10);
    DrawingSurface_BlendSurface((ScriptDrawingSurface*)self, (ScriptDrawingSurface*)params[0].Ptr,
        (BlendMode)params[1].IValue, params[2].IValue, params[3].IValue, params[4].IValue, params[5].IValue,
        params[6].IValue, params[7].IValue, params[8].IValue, params[9].IValue, params[10].IValue);
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

void RegisterDrawingSurfaceAPI(ScriptAPIVersion /*base_api*/, ScriptAPIVersion /*compat_api*/)
{
    ScFnRegister drawsurf_api[] = {
        { "DrawingSurface::Clear^1",              API_FN_PAIR(DrawingSurface_Clear) },
        { "DrawingSurface::CreateCopy^0",         API_FN_PAIR(DrawingSurface_CreateCopy) },
        { "DrawingSurface::DrawCircle^3",         API_FN_PAIR(DrawingSurface_DrawCircle) },
        { "DrawingSurface::DrawImage^10",         API_FN_PAIR(DrawingSurface_DrawImage) },
        { "DrawingSurface::DrawLine^5",           API_FN_PAIR(DrawingSurface_DrawLine) },
        { "DrawingSurface::DrawStringWrapped^6",  API_FN_PAIR(DrawingSurface_DrawStringWrapped) },
        { "DrawingSurface::DrawPixel^2",          API_FN_PAIR(DrawingSurface_DrawPixel) },
        { "DrawingSurface::DrawRectangle^4",      API_FN_PAIR(DrawingSurface_DrawRectangle) },
        { "DrawingSurface::DrawString^104",       Sc_DrawingSurface_DrawString, ScPl_DrawingSurface_DrawString },
        { "DrawingSurface::DrawSurface^10",       API_FN_PAIR(DrawingSurface_DrawSurface) },
        { "DrawingSurface::DrawTriangle^6",       API_FN_PAIR(DrawingSurface_DrawTriangle) },
        { "DrawingSurface::GetPixel^2",           API_FN_PAIR(DrawingSurface_GetPixel) },
        { "DrawingSurface::Release^0",            API_FN_PAIR(DrawingSurface_Release) },
        { "DrawingSurface::get_DrawingColor",     API_FN_PAIR(DrawingSurface_GetDrawingColor) },
        { "DrawingSurface::set_DrawingColor",     API_FN_PAIR(DrawingSurface_SetDrawingColor) },
        { "DrawingSurface::get_ColorDepth",       API_FN_PAIR(DrawingSurface_GetColorDepth) },
        { "DrawingSurface::get_Height",           API_FN_PAIR(DrawingSurface_GetHeight) },
        { "DrawingSurface::get_Width",            API_FN_PAIR(DrawingSurface_GetWidth) },

        { "DrawingSurface::BlendImage^11",        API_FN_PAIR(DrawingSurface_BlendImage) },
        { "DrawingSurface::BlendSurface^11",      API_FN_PAIR(DrawingSurface_BlendSurface) },
    };

    ccAddExternalFunctions(drawsurf_api);
    {
    }
    { // old non-variadic and new variadic variants
        ccAddExternalObjectFunction("DrawingSurface::DrawStringWrapped^106", Sc_DrawingSurface_DrawStringWrapped, (void*)ScPl_DrawingSurface_DrawStringWrapped);
    }
}
