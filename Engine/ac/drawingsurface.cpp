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

#include "util/wgt2allg.h"
#include "ac/draw.h"
#include "ac/drawingsurface.h"
#include "ac/common.h"
#include "ac/charactercache.h"
#include "ac/display.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/objectcache.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/string.h"
#include "debug/debug_log.h"
#include "gui/guimain.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

extern GameSetupStruct game;
extern GameState play;
extern RoomStatus*croom;
extern RoomObject*objs;
extern CharacterCache *charcache;
extern ObjectCache objcache[MAX_INIT_SPR];
extern GUIMain*guis;
extern SpriteCache spriteset;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern Bitmap *dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];
extern RuntimeScriptValue GlobalReturnValue;

extern int current_screen_resolution_multiplier;
extern int trans_mode;

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
    if (sds->dynamicSpriteNumber >= 0)
    {
        if (sds->modified)
        {
            int tt;
            // force a refresh of any cached object or character images
            if (croom != NULL) 
            {
                for (tt = 0; tt < croom->numobj; tt++) 
                {
                    if (objs[tt].num == sds->dynamicSpriteNumber)
                        objcache[tt].sppic = -31999;
                }
            }
            for (tt = 0; tt < game.numcharacters; tt++) 
            {
                if (charcache[tt].sppic == sds->dynamicSpriteNumber)
                    charcache[tt].sppic = -31999;
            }
            for (tt = 0; tt < game.numgui; tt++) 
            {
                if ((guis[tt].bgpic == sds->dynamicSpriteNumber) &&
                    (guis[tt].on == 1))
                {
                    guis_need_update = 1;
                    break;
                }
            }
        }

        sds->dynamicSpriteNumber = -1;
    }
    if (sds->dynamicSurfaceNumber >= 0)
    {
        delete dynamicallyCreatedSurfaces[sds->dynamicSurfaceNumber];
        dynamicallyCreatedSurfaces[sds->dynamicSurfaceNumber] = NULL;
        sds->dynamicSurfaceNumber = -1;
    }
    sds->modified = 0;
}

void ScriptDrawingSurface::MultiplyCoordinates(int *xcoord, int *ycoord)
{
    if (this->highResCoordinates)
    {
        if (current_screen_resolution_multiplier == 1) 
        {
            // using high-res co-ordinates but game running at low-res
            xcoord[0] /= 2;
            ycoord[0] /= 2;
        }
    }
    else
    {
        if (current_screen_resolution_multiplier > 1) 
        {
            // using low-res co-ordinates but game running at high-res
            xcoord[0] *= 2;
            ycoord[0] *= 2;
        }
    }
}

void ScriptDrawingSurface::MultiplyThickness(int *valueToAdjust)
{
    if (this->highResCoordinates)
    {
        if (current_screen_resolution_multiplier == 1) 
        {
            valueToAdjust[0] /= 2;
            if (valueToAdjust[0] < 1)
                valueToAdjust[0] = 1;
        }
    }
    else
    {
        if (current_screen_resolution_multiplier > 1) 
        {
            valueToAdjust[0] *= 2;
        }
    }
}

// convert actual co-ordinate back to what the script is expecting
void ScriptDrawingSurface::UnMultiplyThickness(int *valueToAdjust)
{
    if (this->highResCoordinates)
    {
        if (current_screen_resolution_multiplier == 1) 
        {
            valueToAdjust[0] *= 2;
        }
    }
    else
    {
        if (current_screen_resolution_multiplier > 1) 
        {
            valueToAdjust[0] /= 2;
            if (valueToAdjust[0] < 1)
                valueToAdjust[0] = 1;
        }
    }
}

ScriptDrawingSurface* DrawingSurface_CreateCopy(ScriptDrawingSurface *sds)
{
    Bitmap *sourceBitmap = sds->GetBitmapSurface();

    for (int i = 0; i < MAX_DYNAMIC_SURFACES; i++)
    {
        if (dynamicallyCreatedSurfaces[i] == NULL)
        {
            dynamicallyCreatedSurfaces[i] = BitmapHelper::CreateBitmap(sourceBitmap->GetWidth(), sourceBitmap->GetHeight(), sourceBitmap->GetColorDepth());
            dynamicallyCreatedSurfaces[i]->Blit(sourceBitmap, 0, 0, 0, 0, sourceBitmap->GetWidth(), sourceBitmap->GetHeight());
            ScriptDrawingSurface *newSurface = new ScriptDrawingSurface();
            newSurface->dynamicSurfaceNumber = i;
            newSurface->hasAlphaChannel = sds->hasAlphaChannel;
            ccRegisterManagedObject(newSurface, newSurface);
            GlobalReturnValue.SetDynamicObject(newSurface, newSurface);
            return newSurface;
        }
    }

    quit("!DrawingSurface.CreateCopy: too many copied surfaces created");
    return NULL;
}

void DrawingSurface_DrawSurface(ScriptDrawingSurface* target, ScriptDrawingSurface* source, int translev) {
    if ((translev < 0) || (translev > 99))
        quit("!DrawingSurface.DrawSurface: invalid parameter (transparency must be 0-99)");

    target->StartDrawing();
    Bitmap *surfaceToDraw = source->GetBitmapSurface();

    if (surfaceToDraw == abuf)
        quit("!DrawingSurface.DrawSurface: cannot draw surface onto itself");

    if (translev == 0) {
        // just draw it over the top, no transparency
        abuf->Blit(surfaceToDraw, 0, 0, 0, 0, surfaceToDraw->GetWidth(), surfaceToDraw->GetHeight());
        target->FinishedDrawing();
        return;
    }

    if (surfaceToDraw->GetColorDepth() <= 8)
        quit("!DrawingSurface.DrawSurface: 256-colour surfaces cannot be drawn transparently");

    // Draw it transparently
    trans_mode = ((100-translev) * 25) / 10;
    put_sprite_256(0, 0, surfaceToDraw);
    target->FinishedDrawing();
}

void DrawingSurface_DrawImage(ScriptDrawingSurface* sds, int xx, int yy, int slot, int trans, int width, int height)
{
    if ((slot < 0) || (slot >= MAX_SPRITES) || (spriteset[slot] == NULL))
        quit("!DrawingSurface.DrawImage: invalid sprite slot number specified");

    if ((trans < 0) || (trans > 100))
        quit("!DrawingSurface.DrawImage: invalid transparency setting");

    // 100% transparency, don't draw anything
    if (trans == 100)
        return;

    Bitmap *sourcePic = spriteset[slot];
    bool needToFreeBitmap = false;

    if (width != SCR_NO_VALUE)
    {
        // Resize specified

        if ((width < 1) || (height < 1))
            return;

        sds->MultiplyCoordinates(&width, &height);

        // resize the sprite to the requested size
        Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, sourcePic->GetColorDepth());

        newPic->StretchBlt(sourcePic,
            RectWH(0, 0, spritewidth[slot], spriteheight[slot]),
            RectWH(0, 0, width, height));

        sourcePic = newPic;
        needToFreeBitmap = true;
        update_polled_stuff_if_runtime();
    }

    sds->StartDrawing();
    sds->MultiplyCoordinates(&xx, &yy);

    if (sourcePic->GetColorDepth() != abuf->GetColorDepth()) {
        debug_log("RawDrawImage: Sprite %d colour depth %d-bit not same as background depth %d-bit", slot, spriteset[slot]->GetColorDepth(), abuf->GetColorDepth());
    }

    if (trans > 0)
    {
        trans_mode = ((100 - trans) * 255) / 100;
    }

    draw_sprite_support_alpha(xx, yy, sourcePic, slot);

    sds->FinishedDrawing();

    if (needToFreeBitmap)
        delete sourcePic;
}


void DrawingSurface_SetDrawingColor(ScriptDrawingSurface *sds, int newColour) 
{
    sds->currentColourScript = newColour;
    // StartDrawing to set up abuf to set the colour at the appropriate
    // depth for the background
    sds->StartDrawing();
    if (newColour == SCR_COLOR_TRANSPARENT)
    {
        sds->currentColour = abuf->GetMaskColor();
    }
    else
    {
        sds->currentColour = get_col8_lookup(newColour);
    }
    sds->FinishedDrawingReadOnly();
}

int DrawingSurface_GetDrawingColor(ScriptDrawingSurface *sds)
{
    return sds->currentColourScript;
}

void DrawingSurface_SetUseHighResCoordinates(ScriptDrawingSurface *sds, int highRes) 
{
    sds->highResCoordinates = (highRes) ? 1 : 0;
}

int DrawingSurface_GetUseHighResCoordinates(ScriptDrawingSurface *sds) 
{
    return sds->highResCoordinates;
}

int DrawingSurface_GetHeight(ScriptDrawingSurface *sds) 
{
    sds->StartDrawing();
    int height = abuf->GetHeight();
    sds->FinishedDrawingReadOnly();
    sds->UnMultiplyThickness(&height);
    return height;
}

int DrawingSurface_GetWidth(ScriptDrawingSurface *sds) 
{
    sds->StartDrawing();
    int width = abuf->GetWidth();
    sds->FinishedDrawingReadOnly();
    sds->UnMultiplyThickness(&width);
    return width;
}

void DrawingSurface_Clear(ScriptDrawingSurface *sds, int colour)
{
    sds->StartDrawing();
    int allegroColor;
    if ((colour == -SCR_NO_VALUE) || (colour == SCR_COLOR_TRANSPARENT))
    {
        allegroColor = abuf->GetMaskColor();
    }
    else
    {
        allegroColor = get_col8_lookup(colour);
    }
    abuf->Clear(allegroColor);
    sds->FinishedDrawing();
}

void DrawingSurface_DrawCircle(ScriptDrawingSurface *sds, int x, int y, int radius)
{
    sds->MultiplyCoordinates(&x, &y);
    sds->MultiplyThickness(&radius);

    sds->StartDrawing();
    abuf->FillCircle(Circle(x, y, radius), sds->currentColour);
    sds->FinishedDrawing();
}

void DrawingSurface_DrawRectangle(ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2)
{
    sds->MultiplyCoordinates(&x1, &y1);
    sds->MultiplyCoordinates(&x2, &y2);

    sds->StartDrawing();
    abuf->FillRect(Rect(x1,y1,x2,y2), sds->currentColour);
    sds->FinishedDrawing();
}

void DrawingSurface_DrawTriangle(ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2, int x3, int y3)
{
    sds->MultiplyCoordinates(&x1, &y1);
    sds->MultiplyCoordinates(&x2, &y2);
    sds->MultiplyCoordinates(&x3, &y3);

    sds->StartDrawing();
    abuf->DrawTriangle(Triangle(x1,y1,x2,y2,x3,y3), sds->currentColour);
    sds->FinishedDrawing();
}

void DrawingSurface_DrawString(ScriptDrawingSurface *sds, int xx, int yy, int font, const char* texx, ...)
{
    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf,get_translation(texx),ap);
    va_end(ap);
    // don't use wtextcolor because it will do a 16->32 conversion
    textcol = sds->currentColour;

    sds->MultiplyCoordinates(&xx, &yy);
    sds->StartDrawing();
    wtexttransparent(TEXTFG);
    if ((abuf->GetColorDepth() <= 8) && (play.raw_color > 255)) {
        wtextcolor(1);
        debug_log ("RawPrint: Attempted to use hi-color on 256-col background");
    }
    wouttext_outline(xx, yy, font, displbuf);
    sds->FinishedDrawing();
}

void DrawingSurface_DrawStringWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int alignment, const char *msg) {
    int texthit = wgetfontheight(font);
    sds->MultiplyCoordinates(&xx, &yy);
    sds->MultiplyThickness(&wid);

    break_up_text_into_lines(wid, font, (char*)msg);

    textcol = sds->currentColour;
    sds->StartDrawing();

    wtexttransparent(TEXTFG);
    for (int i = 0; i < numlines; i++)
    {
        int drawAtX = xx;

        if (alignment == SCALIGN_CENTRE)
        {
            drawAtX = xx + ((wid / 2) - wgettextwidth(lines[i], font) / 2);
        }
        else if (alignment == SCALIGN_RIGHT)
        {
            drawAtX = (xx + wid) - wgettextwidth(lines[i], font);
        }

        wouttext_outline(drawAtX, yy + texthit*i, font, lines[i]);
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

    DrawingSurface_DrawStringWrapped(sds, xx, yy, wid, font, SCALIGN_LEFT, displbuf);
}

void DrawingSurface_DrawLine(ScriptDrawingSurface *sds, int fromx, int fromy, int tox, int toy, int thickness) {
    sds->MultiplyCoordinates(&fromx, &fromy);
    sds->MultiplyCoordinates(&tox, &toy);
    sds->MultiplyThickness(&thickness);
    int ii,jj,xx,yy;
    sds->StartDrawing();
    // draw several lines to simulate the thickness
    for (ii = 0; ii < thickness; ii++) 
    {
        xx = (ii - (thickness / 2));
        for (jj = 0; jj < thickness; jj++)
        {
            yy = (jj - (thickness / 2));
            abuf->DrawLine (Line(fromx + xx, fromy + yy, tox + xx, toy + yy), sds->currentColour);
        }
    }
    sds->FinishedDrawing();
}

void DrawingSurface_DrawPixel(ScriptDrawingSurface *sds, int x, int y) {
    sds->MultiplyCoordinates(&x, &y);
    int thickness = 1;
    sds->MultiplyThickness(&thickness);
    int ii,jj;
    sds->StartDrawing();
    // draw several pixels to simulate the thickness
    for (ii = 0; ii < thickness; ii++) 
    {
        for (jj = 0; jj < thickness; jj++)
        {
            abuf->PutPixel(x + ii, y + jj, sds->currentColour);
        }
    }
    sds->FinishedDrawing();
}

int DrawingSurface_GetPixel(ScriptDrawingSurface *sds, int x, int y) {
    sds->MultiplyCoordinates(&x, &y);
    sds->StartDrawing();
    unsigned int rawPixel = abuf->GetPixel(x, y);
    unsigned int maskColor = abuf->GetMaskColor();
    int colDepth = abuf->GetColorDepth();

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
RuntimeScriptValue Sc_DrawingSurface_Clear(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDrawingSurface, DrawingSurface_Clear)
}

// ScriptDrawingSurface* (ScriptDrawingSurface *sds)
RuntimeScriptValue Sc_DrawingSurface_CreateCopy(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptDrawingSurface, ScriptDrawingSurface, DrawingSurface_CreateCopy)
}

// void (ScriptDrawingSurface *sds, int x, int y, int radius)
RuntimeScriptValue Sc_DrawingSurface_DrawCircle(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptDrawingSurface, DrawingSurface_DrawCircle)
}

// void (ScriptDrawingSurface* sds, int xx, int yy, int slot, int trans, int width, int height)
RuntimeScriptValue Sc_DrawingSurface_DrawImage(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT6(ScriptDrawingSurface, DrawingSurface_DrawImage)
}

// void (ScriptDrawingSurface *sds, int fromx, int fromy, int tox, int toy, int thickness)
RuntimeScriptValue Sc_DrawingSurface_DrawLine(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptDrawingSurface, DrawingSurface_DrawLine)
}

// void (ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int msgm)
RuntimeScriptValue Sc_DrawingSurface_DrawMessageWrapped(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptDrawingSurface, DrawingSurface_DrawMessageWrapped)
}

// void (ScriptDrawingSurface *sds, int x, int y)
RuntimeScriptValue Sc_DrawingSurface_DrawPixel(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptDrawingSurface, DrawingSurface_DrawPixel)
}

// void (ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2)
RuntimeScriptValue Sc_DrawingSurface_DrawRectangle(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptDrawingSurface, DrawingSurface_DrawRectangle)
}

// void (ScriptDrawingSurface *sds, int xx, int yy, int font, const char* texx, ...)
RuntimeScriptValue Sc_DrawingSurface_DrawString(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return RuntimeScriptValue();
}

// void (ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int alignment, const char *msg)
RuntimeScriptValue Sc_DrawingSurface_DrawStringWrapped(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5_POBJ(ScriptDrawingSurface, DrawingSurface_DrawStringWrapped, const char)
}

// void (ScriptDrawingSurface* target, ScriptDrawingSurface* source, int translev)
RuntimeScriptValue Sc_DrawingSurface_DrawSurface(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT(ScriptDrawingSurface, DrawingSurface_DrawSurface, ScriptDrawingSurface)
}

// void (ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2, int x3, int y3)
RuntimeScriptValue Sc_DrawingSurface_DrawTriangle(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT6(ScriptDrawingSurface, DrawingSurface_DrawTriangle)
}

// int (ScriptDrawingSurface *sds, int x, int y)
RuntimeScriptValue Sc_DrawingSurface_GetPixel(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT2(ScriptDrawingSurface, DrawingSurface_GetPixel)
}

// void (ScriptDrawingSurface* sds)
RuntimeScriptValue Sc_DrawingSurface_Release(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptDrawingSurface, DrawingSurface_Release)
}

// int (ScriptDrawingSurface *sds)
RuntimeScriptValue Sc_DrawingSurface_GetDrawingColor(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetDrawingColor)
}

// void (ScriptDrawingSurface *sds, int newColour)
RuntimeScriptValue Sc_DrawingSurface_SetDrawingColor(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDrawingSurface, DrawingSurface_SetDrawingColor)
}

// int (ScriptDrawingSurface *sds)
RuntimeScriptValue Sc_DrawingSurface_GetHeight(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetHeight)
}

// int (ScriptDrawingSurface *sds)
RuntimeScriptValue Sc_DrawingSurface_GetUseHighResCoordinates(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetUseHighResCoordinates)
}

// void (ScriptDrawingSurface *sds, int highRes)
RuntimeScriptValue Sc_DrawingSurface_SetUseHighResCoordinates(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDrawingSurface, DrawingSurface_SetUseHighResCoordinates)
}

// int (ScriptDrawingSurface *sds)
RuntimeScriptValue Sc_DrawingSurface_GetWidth(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetWidth)
}

void RegisterDrawingSurfaceAPI()
{
    ccAddExternalObjectFunction("DrawingSurface::Clear^1",              Sc_DrawingSurface_Clear);
    ccAddExternalObjectFunction("DrawingSurface::CreateCopy^0",         Sc_DrawingSurface_CreateCopy);
    ccAddExternalObjectFunction("DrawingSurface::DrawCircle^3",         Sc_DrawingSurface_DrawCircle);
    ccAddExternalObjectFunction("DrawingSurface::DrawImage^6",          Sc_DrawingSurface_DrawImage);
    ccAddExternalObjectFunction("DrawingSurface::DrawLine^5",           Sc_DrawingSurface_DrawLine);
    ccAddExternalObjectFunction("DrawingSurface::DrawMessageWrapped^5", Sc_DrawingSurface_DrawMessageWrapped);
    ccAddExternalObjectFunction("DrawingSurface::DrawPixel^2",          Sc_DrawingSurface_DrawPixel);
    ccAddExternalObjectFunction("DrawingSurface::DrawRectangle^4",      Sc_DrawingSurface_DrawRectangle);
    ccAddExternalObjectFunction("DrawingSurface::DrawString^104",       Sc_DrawingSurface_DrawString);
    ccAddExternalObjectFunction("DrawingSurface::DrawStringWrapped^6",  Sc_DrawingSurface_DrawStringWrapped);
    ccAddExternalObjectFunction("DrawingSurface::DrawSurface^2",        Sc_DrawingSurface_DrawSurface);
    ccAddExternalObjectFunction("DrawingSurface::DrawTriangle^6",       Sc_DrawingSurface_DrawTriangle);
    ccAddExternalObjectFunction("DrawingSurface::GetPixel^2",           Sc_DrawingSurface_GetPixel);
    ccAddExternalObjectFunction("DrawingSurface::Release^0",            Sc_DrawingSurface_Release);
    ccAddExternalObjectFunction("DrawingSurface::get_DrawingColor",     Sc_DrawingSurface_GetDrawingColor);
    ccAddExternalObjectFunction("DrawingSurface::set_DrawingColor",     Sc_DrawingSurface_SetDrawingColor);
    ccAddExternalObjectFunction("DrawingSurface::get_Height",           Sc_DrawingSurface_GetHeight);
    ccAddExternalObjectFunction("DrawingSurface::get_UseHighResCoordinates", Sc_DrawingSurface_GetUseHighResCoordinates);
    ccAddExternalObjectFunction("DrawingSurface::set_UseHighResCoordinates", Sc_DrawingSurface_SetUseHighResCoordinates);
    ccAddExternalObjectFunction("DrawingSurface::get_Width",            Sc_DrawingSurface_GetWidth);
}
