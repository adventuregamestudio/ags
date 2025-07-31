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
#include "font/ttffontrenderer.h"
#include <SDL_ttf.h>
#include "ac/game_version.h"
#include "core/platform.h"
#include "core/assetmanager.h"
#include "font/fonts.h"
#include "gfx/blender.h"
#include "util/stream.h"
#include "util/sdl2_util.h"

using namespace AGS::Common;

TTFFontRenderer::TTFFontRenderer(AssetManager *amgr)
    : _amgr(amgr)
{
    TTF_Init();
}

TTFFontRenderer::~TTFFontRenderer()
{
    TTF_Quit();
}

void TTFFontRenderer::AdjustYCoordinateForFont(int *ycoord, int /*fontNumber*/)
{
  // TTF fonts already have space at the top, so try to remove the gap
  // TODO: adding -1 was here before (check the comment above),
  // but how universal is this "space at the top"?
  // Also, why is this function used only in one case of text rendering?
  // Review this after we upgrade the font library.
  ycoord[0]--;
}

void TTFFontRenderer::EnsureTextValidForFont(char * /*text*/, int /*fontNumber*/)
{
  // do nothing, TTF can handle all characters
}

int TTFFontRenderer::GetTextWidth(const char *text, int fontNumber)
{
    int w = 0;
    TTF_SizeUTF8(_fontData[fontNumber].Font, text, &w, nullptr);
    return w;
}

int TTFFontRenderer::GetTextHeight(const char * /*text*/, int fontNumber)
{
    return TTF_FontHeight(_fontData[fontNumber].Font);
    /* FIXME: what to do with this?
  // Compatibility mode: if we required to return "nominal font height",
  // then ask alfont to return one the font was loaded with
  if ((_fontData[fontNumber].Params.LoadMode & FFLG_REPORTNOMINALHEIGHT) != 0)
    return alfont_get_font_height(_fontData[fontNumber].AlFont);
  else
    return alfont_get_font_real_height(_fontData[fontNumber].AlFont);
    */
}

void TTFFontRenderer::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int color)
{
    if (y > destination->cb)  // optimisation
        return;

    // Check if this is a alpha-blending case, and init corresponding draw mode
    const bool is_indexed_dst = (bitmap_color_depth(destination) == 8);
    const bool alpha_blend = (bitmap_color_depth(destination) == 32) &&
        ((geta32(color) != 0xFF) || (_blendMode != kBlend_Normal));
    const bool text_smooth = (ShouldAntiAliasText()) && (!is_indexed_dst);

    SDL_Color fg;
    if (is_indexed_dst)
        fg = { (Uint8)getr8(color), (Uint8)getg8(color), (Uint8)getb8(color), (Uint8)255 };
    else
        fg = { (Uint8)getr32(color), (Uint8)getg32(color), (Uint8)getb32(color), (Uint8)geta32(color) };
    
    SDL_Surface *surf = nullptr;
    if (text_smooth)
        surf = TTF_RenderUTF8_Blended(_fontData[fontNumber].Font, text, fg);
    else
        surf = TTF_RenderUTF8_Solid(_fontData[fontNumber].Font, text, fg);

    if (!surf)
        return;

    Bitmap helper(surf, false);
    Bitmap dest(destination, true);

    // For solid render: temporarily replace palette color at slot 1 with the text color
    RGB old_pal_color;
    if (!text_smooth)
    {
        if (is_indexed_dst)
        {
            BitmapHelper::ReplaceColor(&helper, 1, color);
        }
        else
        {
            get_color(1, &old_pal_color);
            set_color(1, (RGB *)&fg); // NOTE: SDL_Color and Allegro RGB match fields
        }
    }

    if (alpha_blend)
    {
        SetBlender(_blendMode, 255);
        dest.TransBlendBlt(&helper, x, y);
    }
    else
    {
        dest.MaskedBlit(&helper, x, y);
    }

    if (!text_smooth && !is_indexed_dst)
    {
        set_color(1, &old_pal_color);
    }
}

bool TTFFontRenderer::LoadFromDisk(int fontNumber, int fontSize)
{
  return LoadFromDiskEx(fontNumber, fontSize, String::FromFormat("agsfnt%d.ttf", fontNumber), nullptr, nullptr);
}

bool TTFFontRenderer::IsBitmapFont()
{
    return false;
}

// Loads a TTF font of a certain size
static TTF_Font *LoadTTF(std::unique_ptr<Stream> &&in, int font_size)
{
    SDL_RWops *rw = SDL2Util::OpenRWops(std::move(in));
    TTF_Font *font = TTF_OpenFontRW(rw, 1, font_size);
    return font;
}

// FIXME: would be best to use SDL_ttf API for retrieving FT_Face's BBOX
#include <freetype/freetype.h>

// Fill the FontMetrics struct from the given ALFONT
static void FillMetrics(TTF_Font *font, int nominal_size, FontMetrics *metrics)
{
    metrics->NominalHeight = nominal_size;
    metrics->RealHeight = TTF_FontHeight(font);
    metrics->CompatHeight = metrics->NominalHeight; // just set to default here
    std::pair<int, int> vextent;
    {
        int miny, maxy;
        TTF_GetFontBBox(font, nullptr, nullptr, &miny, &maxy);
        int real_face_extent_asc = (int)maxy;
        int real_face_extent_desc = -(int)miny;
        int face_ascender = TTF_FontAscent(font);
        int face_descender = TTF_FontDescent(font);
        int top = face_ascender - real_face_extent_asc; // may be negative
        int bottom = face_ascender + real_face_extent_desc;
        vextent = std::make_pair(top, bottom);
    }

    metrics->VExtent = vextent;
    // fixup vextent to be *not less* than realheight
    metrics->VExtent.first = std::min(0, metrics->VExtent.first);
    metrics->VExtent.second = std::max(metrics->RealHeight, metrics->VExtent.second);
}

TTF_Font *TTFFontRenderer::LoadTTF(const AGS::Common::String &filename, int font_size)
{
    auto in = _amgr->OpenAsset(filename);
    if (!in)
        return nullptr;

    return ::LoadTTF(std::move(in), font_size);
}

bool TTFFontRenderer::LoadFromDiskEx(int fontNumber, int fontSize, const String &filename,
    const FontRenderParams *params, FontMetrics *metrics)
{
    if (fontSize <= 0)
        fontSize = 8; // compatibility fix
    assert(params);
    FontRenderParams f_params = params ? *params : FontRenderParams();
    if (f_params.SizeMultiplier > 1)
        fontSize *= f_params.SizeMultiplier;

    TTF_Font *font = LoadTTF(filename, fontSize);
    if (!font)
        return false;

    _fontData[fontNumber].Font = font;
    _fontData[fontNumber].SizePt = fontSize;
    _fontData[fontNumber].Params = f_params;
    if (metrics)
        FillMetrics(font, fontSize, metrics);
    return true;
}

const char *TTFFontRenderer::GetFontName(int fontNumber)
{
    return TTF_FontFaceFamilyName(_fontData[fontNumber].Font);
}

int TTFFontRenderer::GetFontHeight(int fontNumber)
{
    return TTF_FontHeight(_fontData[fontNumber].Font);
}

void TTFFontRenderer::GetFontMetrics(int fontNumber, FontMetrics *metrics)
{
    FillMetrics(_fontData[fontNumber].Font, _fontData[fontNumber].SizePt, metrics);
}

void TTFFontRenderer::AdjustFontForAntiAlias(int /*fontNumber*/, bool /*aa_mode*/)
{
}

void TTFFontRenderer::SetBlendMode(BlendMode blend_mode)
{
    _blendMode = blend_mode;
}

void TTFFontRenderer::FreeMemory(int fontNumber)
{
    TTF_CloseFont(_fontData[fontNumber].Font);
    _fontData.erase(fontNumber);
}

bool TTFFontRenderer::MeasureFontOfPointSize(const String &filename, int size_pt, FontMetrics *metrics)
{
    TTF_Font *font = LoadTTF(filename, size_pt);
    if (!font)
        return false;
    FillMetrics(font, size_pt, metrics);
    TTF_CloseFont(font);
    return true;
}

bool TTFFontRenderer::MeasureFontOfPixelHeight(const String &filename, int pixel_height, FontMetrics *metrics)
{
    // FIXME: pixel_height to size_pt
    int size_pt = pixel_height;
    TTF_Font *font = LoadTTF(filename, size_pt);
    if (!font)
        return false;
    FillMetrics(font, size_pt, metrics);
    TTF_CloseFont(font);
    return true;
}
