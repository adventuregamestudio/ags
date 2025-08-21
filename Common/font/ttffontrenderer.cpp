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
#include <alfont.h>
#include "ac/game_version.h"
#include "core/platform.h"
#include "core/assetmanager.h"
#include "font/fonts.h"
#include "util/stream.h"

using namespace AGS::Common;

TTFFontRenderer::TTFFontRenderer(AssetManager *amgr)
    : _amgr(amgr)
{
    alfont_init();
    alfont_text_mode(-1);
}

TTFFontRenderer::~TTFFontRenderer()
{
    alfont_exit();
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
  return alfont_text_length(_fontData[fontNumber].AlFont, text);
}

int TTFFontRenderer::GetTextHeight(const char * /*text*/, int fontNumber)
{
  // Compatibility mode: if we required to return "nominal font height",
  // then ask alfont to return one the font was loaded with
  if ((_fontData[fontNumber].Params.LoadMode & FFLG_REPORTNOMINALHEIGHT) != 0)
    return alfont_get_font_height(_fontData[fontNumber].AlFont);
  else
    return alfont_get_font_real_height(_fontData[fontNumber].AlFont);
}

void TTFFontRenderer::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour)
{
  if (y > destination->cb)  // optimisation
    return;

  // Y - 1 because it seems to get drawn down a bit
  if ((ShouldAntiAliasText()) && (bitmap_color_depth(destination) > 8))
    alfont_textout_aa(destination, _fontData[fontNumber].AlFont, text, x, y - 1, colour);
  else
    alfont_textout(destination, _fontData[fontNumber].AlFont, text, x, y - 1, colour);
}

bool TTFFontRenderer::LoadFromDisk(int fontNumber, int fontSize)
{
  return LoadFromDiskEx(fontNumber, fontSize, String(), nullptr, nullptr, nullptr);
}

bool TTFFontRenderer::IsBitmapFont()
{
    return false;
}

static int GetAlfontFlags(int load_mode)
{
  int flags = ALFONT_FLG_FORCE_RESIZE | ALFONT_FLG_SELECT_NOMINAL_SZ;
  // Compatibility: font ascender is always adjusted to the formal font's height;
  // EXCEPTION: not if it's a game made before AGS 3.4.1 with TTF anti-aliasing
  // (the reason is uncertain, but this is to emulate old engine's behavior).
  if (((load_mode & FFLG_ASCENDERFIXUP) != 0) &&
      !(ShouldAntiAliasText() && (loaded_game_file_version < kGameVersion_341)))
      flags |= ALFONT_FLG_ASCENDER_EQ_HEIGHT;
  // Precalculate real glyphs extent (will make loading fonts relatively slower)
  flags |= ALFONT_FLG_PRECALC_MAX_CBOX;
  return flags;
}

// Loads a TTF font of a certain size
static ALFONT_FONT *LoadTTFFromMem(const uint8_t *data, size_t data_len, int font_size, int alfont_flags)
{
    ALFONT_FONT *alfptr = alfont_load_font_from_mem(reinterpret_cast<const char*>(data), data_len);
    if (!alfptr)
        return nullptr;
    alfont_set_font_size_ex(alfptr, font_size, alfont_flags);
    return alfptr;
}

// Fill the FontMetrics struct from the given ALFONT
static void FillMetrics(ALFONT_FONT *alfptr, FontMetrics *metrics)
{
    metrics->NominalHeight = alfont_get_font_height(alfptr);
    metrics->RealHeight = alfont_get_font_real_height(alfptr);
    metrics->CompatHeight = metrics->NominalHeight; // just set to default here
    {
        Rect ttf_bbox;
        alfont_get_font_bbox(alfptr, &ttf_bbox.Left, &ttf_bbox.Top, &ttf_bbox.Right, &ttf_bbox.Bottom);
        // Remember that FreeType has Y axis pointing bottom->up
        int real_face_extent_asc = (int)ttf_bbox.Top;
        int real_face_extent_desc = -(int)ttf_bbox.Bottom;
        int face_ascent = alfont_get_font_ascent(alfptr);
        int top    = face_ascent - real_face_extent_asc; // may be negative
        int bottom = face_ascent + real_face_extent_desc;
        // Get real vertical extent, relative to the ascent (y pen position)
        metrics->VExtent = std::make_pair(top, bottom);
        // Recalc bbox, from the top-left "pen" position, and match our Y axis pointing down
        metrics->BBox = Rect(ttf_bbox.Left, top, ttf_bbox.Right, bottom);
    }
    // Fix vertical extent to be *not less* than realheight
    metrics->VExtent.first = std::min(0, metrics->VExtent.first);
    metrics->VExtent.second = std::max(metrics->RealHeight - 1, metrics->VExtent.second);
}

ALFONT_FONT *TTFFontRenderer::LoadTTF(const AGS::Common::String &filename, int font_size, int alfont_flags)
{
    auto reader = _amgr->OpenAsset(filename);
    if (!reader)
        return nullptr;

    const size_t lenof = reader->GetLength();
    std::vector<uint8_t> buf(lenof);
    reader->Read(buf.data(), lenof);
    reader.reset();

    return LoadTTFFromMem(buf.data(), lenof, font_size, alfont_flags);
}

bool TTFFontRenderer::LoadFromDiskEx(int fontNumber, int fontSize, const String &filename,
    String *src_filename, const FontRenderParams *params, FontMetrics *metrics)
{
    String use_filename = !filename.IsEmpty() ? filename :
        String::FromFormat("agsfnt%d.ttf", fontNumber);
    if (fontSize <= 0)
        fontSize = 8; // compatibility fix
    assert(params);
    FontRenderParams f_params = params ? *params : FontRenderParams();
    if (f_params.SizeMultiplier > 1)
        fontSize *= f_params.SizeMultiplier;

    ALFONT_FONT *alfptr = LoadTTF(use_filename, fontSize,
        GetAlfontFlags(f_params.LoadMode));
    if (!alfptr)
        return false;

    _fontData[fontNumber].AlFont = alfptr;
    _fontData[fontNumber].Params = f_params;
    if (src_filename)
        *src_filename = use_filename;
    if (metrics)
        FillMetrics(alfptr, metrics);
    return true;
}

const char *TTFFontRenderer::GetFontName(int fontNumber)
{
  return alfont_get_name(_fontData[fontNumber].AlFont);
}

int TTFFontRenderer::GetFontHeight(int fontNumber)
{
  return alfont_get_font_real_height(_fontData[fontNumber].AlFont);
}

void TTFFontRenderer::GetFontMetrics(int fontNumber, FontMetrics *metrics)
{
    FillMetrics(_fontData[fontNumber].AlFont, metrics);
}

void TTFFontRenderer::AdjustFontForAntiAlias(int fontNumber, bool /*aa_mode*/)
{
  if (loaded_game_file_version < kGameVersion_341)
  {
    ALFONT_FONT *alfptr = _fontData[fontNumber].AlFont;
    const FontRenderParams &params = _fontData[fontNumber].Params;
    int old_height = alfont_get_font_height(alfptr);
    alfont_set_font_size_ex(alfptr, old_height, GetAlfontFlags(params.LoadMode));
  }
}

void TTFFontRenderer::GetCharCodeRange(int fontNumber, std::pair<int, int> *char_codes)
{
    int first_charcode = -1, last_charcode = -1;
    alfont_get_charcode_range(_fontData[fontNumber].AlFont, &first_charcode, &last_charcode, nullptr);
    *char_codes = std::make_pair(first_charcode, last_charcode);
}

void TTFFontRenderer::GetValidCharCodes(int fontNumber, std::vector<int> &char_codes)
{
    int *charcodes = nullptr;
    int count = alfont_get_valid_charcodes(_fontData[fontNumber].AlFont, &charcodes);
    if (!charcodes)
        return;

    char_codes.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        char_codes.push_back(charcodes[i]);
    }
    free(charcodes);
}

void TTFFontRenderer::SetCharacterSpacing(int fontNumber, int spacing)
{
    if (_fontData.find(fontNumber) != _fontData.end())
    {
        alfont_set_char_extra_spacing(_fontData[fontNumber].AlFont, spacing);
    }
}

void TTFFontRenderer::FreeMemory(int fontNumber)
{
  alfont_destroy_font(_fontData[fontNumber].AlFont);
  _fontData.erase(fontNumber);
}

bool TTFFontRenderer::MeasureFontOfPointSize(const String &filename, int size_pt, FontMetrics *metrics)
{
    ALFONT_FONT *alfptr = LoadTTF(filename, size_pt, ALFONT_FLG_FORCE_RESIZE | ALFONT_FLG_SELECT_NOMINAL_SZ);
    if (!alfptr)
        return false;
    FillMetrics(alfptr, metrics);
    alfont_destroy_font(alfptr);
    return true;
}

bool TTFFontRenderer::MeasureFontOfPixelHeight(const String &filename, int pixel_height, FontMetrics *metrics)
{
    ALFONT_FONT *alfptr = LoadTTF(filename, pixel_height, ALFONT_FLG_FORCE_RESIZE);
    if (!alfptr)
        return false;
    FillMetrics(alfptr, metrics);
    alfont_destroy_font(alfptr);
    return true;
}
