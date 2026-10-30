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
#include "font/wfnfontrenderer.h"
#include <algorithm>
#include <cstring>
#include <vector>
#include "ac/common.h" // our_eip
#include "data/assetmanager.h"
#include "debug/out.h"
#include "font/wfnfont.h"
#include "gfx/bitmap.h"
#include "util/stream.h"
#include <cmath>

using namespace AGS::Common;

// Compare a palette color's hue with the base color.
// RGB components use Allegro's 0-63 range.
static bool IsCompatibleWFNHue(const RGB& base, const RGB& candidate)
{
    const int base_min = std::min(base.r, std::min(base.g, base.b));
    const int base_max = std::max(base.r, std::max(base.g, base.b));
    const int base_chroma = base_max - base_min;

    const int candidate_min = std::min(candidate.r,
        std::min(candidate.g, candidate.b));
    const int candidate_max = std::max(candidate.r,
        std::max(candidate.g, candidate.b));
    const int candidate_chroma = candidate_max - candidate_min;

    // Neutral base colors should only match other neutral colors.
    if (base_chroma <= 4)
        return candidate_chroma <= 4;

    // Avoid neutral candidates for colored text.
    if (candidate_chroma <= 4)
        return false;

    // Compare hue angles, allowing gradual hue shifts in lighter shades.
    const auto hue_angle = [](const RGB& c) -> float
        {
            const float x = 2.0f * c.r - c.g - c.b;
            const float y = 1.7320508f * (c.g - c.b);
            return std::atan2(y, x);
        };

    float difference = std::abs(hue_angle(base) - hue_angle(candidate));

    const float pi = 3.14159265f;
    if (difference > pi)
        difference = 2.0f * pi - difference;

    // Allow up to 45 degrees of hue variation.
    return difference <= pi / 4.0f;
}

void WFNFontRenderer::AdjustYCoordinateForFont(int * /*ycoord*/, int /*fontNumber*/)
{
    // Do nothing
}

void WFNFontRenderer::EnsureTextValidForFont(char *text, int fontNumber)
{
    // Do nothing
}

int WFNFontRenderer::GetTextWidth(const char *text, int fontNumber)
{
    const WFNFont* font = _fontData[fontNumber].Font;
    const FontRenderParams &params = _fontData[fontNumber].Params;
    const int char_spacing = _fontData[fontNumber].CharacterSpacing;
    int text_width = 0;
    int char_count = 0;

    for (int code = ugetxc(&text); code; code = ugetxc(&text))
    {
        text_width += font->GetChar(code).Width;
        char_count++;
    }
    text_width *= params.SizeMultiplier;
    // Add extra character spacing between characters (except the last character)
    // NOTE: we do NOT scale global character spacing with SizeMultiplier
    text_width += char_spacing * (char_count - 1);
    return text_width;
}

int WFNFontRenderer::GetTextHeight(const char *text, int fontNumber)
{
    const WFNFont* font = _fontData[fontNumber].Font;
    const FontRenderParams &params = _fontData[fontNumber].Params;
    int max_height = 0;

    for (int code = ugetxc(&text); code; code = ugetxc(&text))
    {
        const uint16_t height = font->GetChar(code).Height;
        max_height = std::max(max_height, static_cast<int>(height));
    }
    return max_height * params.SizeMultiplier;
}

static int RenderChar(Bitmap* ds, const int at_x, const int at_y, Rect clip,
    const WFNChar& wfn_char, const int scale, const color_t text_color,
    const color_t* shade_colors, const uint8_t base_shade,
    bool silhouette);

void WFNFontRenderer::RenderText(const char* text, int fontNumber, BITMAP* destination, int x, int y, int colour)
{
    RenderTextInternal(text, fontNumber, destination, x, y, colour, false);
}

void WFNFontRenderer::RenderTextSilhouette(const char* text, int fontNumber, BITMAP* destination, int x, int y, int colour)
{
    RenderTextInternal(text, fontNumber, destination, x, y, colour, true);
}

void WFNFontRenderer::RenderTextInternal(const char* text, int fontNumber, BITMAP* destination, int x, int y, int colour, bool silhouette)
{
    int oldeip = get_our_eip();
    set_our_eip(415);

    const WFNFont* font = _fontData[fontNumber].Font;
    const FontRenderParams& params = _fontData[fontNumber].Params;
    // NOTE: we do NOT scale global character spacing with SizeMultiplier
    const int char_spacing = _fontData[fontNumber].CharacterSpacing;
    Bitmap ds(destination, true);

    color_t shade_colors[16] = {};
    const color_t* shade_lookup = nullptr;

    if (!silhouette && font->GetBitsPerPixel() == 4 && ds.GetColorDepth() == 8)
    {
        PALETTE current_palette;
        get_palette(current_palette);
        const int base_palette_index = colour & 0xFF;

        const RGB& base = current_palette[base_palette_index];
        const int base_shade = font->GetBaseShadeIndex();

        shade_colors[0] = 0;
        shade_colors[base_shade] = colour;

        // Collect palette colors that match the base hue.
        struct PaletteShade
        {
            int Index;
            int Brightness;
            int Distance;
        };

        std::vector<PaletteShade> darker_colors;
        std::vector<PaletteShade> lighter_colors;

        const int base_brightness =
            299 * base.r + 587 * base.g + 114 * base.b;

        for (int i = 1; i < 256; ++i)
        {
            if (i == base_palette_index)
                continue;

            if (_hasPaletteEligibility && !_paletteEligible[i])
                continue;

            const RGB& candidate = current_palette[i];

            if (!IsCompatibleWFNHue(base, candidate))
                continue;

            const int brightness =
                299 * candidate.r + 587 * candidate.g + 114 * candidate.b;

            const int distance = std::abs(i - base_palette_index);

            if (brightness < base_brightness)
                darker_colors.push_back({ i, brightness, distance });
            else if (brightness > base_brightness)
                lighter_colors.push_back({ i, brightness, distance });
        }

        // Prefer nearby palette entries while preserving brightness order.
        const auto by_brightness = [](const PaletteShade& a, const PaletteShade& b)
        {
            if (a.Brightness != b.Brightness)
                return a.Brightness < b.Brightness;

            return a.Distance < b.Distance;
        };

        std::sort(darker_colors.begin(), darker_colors.end(), by_brightness);
        std::sort(lighter_colors.begin(), lighter_colors.end(), by_brightness);

        // Distribute compatible palette colors across the WFN shade range,
        // preserving brightness order and the exact base color.
        const auto select_shades = [&](bool darker)
            {
                const auto& colors = darker ? darker_colors : lighter_colors;

                if (colors.empty())
                {
                    for (int shade = darker ? base_shade - 1 : base_shade + 1;
                        darker ? shade >= 1 : shade <= 15;
                        shade += darker ? -1 : 1)
                    {
                        shade_colors[shade] = colour;
                    }
                    return;
                }

                // Prefer hue-compatible palette entries near the base index.
                std::vector<PaletteShade> preferred_colors;

                for (const auto& candidate : colors)
                {
                    if (candidate.Distance <= 4)
                        preferred_colors.push_back(candidate);
                }

                // Prioritize two compatible palette colors immediately after the base index.
                std::vector<PaletteShade> adjacent_colors;

                for (const auto& candidate : colors)
                {
                    if (candidate.Index == base_palette_index + 1 ||
                        candidate.Index == base_palette_index + 2)
                    {
                        adjacent_colors.push_back(candidate);
                    }
                }

                if (adjacent_colors.size() >= 2)
                    preferred_colors = adjacent_colors;

                if (preferred_colors.empty())
                    preferred_colors = colors;

                const int range = darker ? base_shade - 1 : 15 - base_shade;
                const int count = static_cast<int>(preferred_colors.size());

                for (int distance = 1; distance <= range; ++distance)
                {
                    const int shade = darker
                        ? base_shade - distance
                        : base_shade + distance;

                    // Spread available colors across the shade range,
                    // allowing intermediate shades to share palette entries.
                    const int step = 1 + (distance - 1) * count / range;

                    const int index = darker
                        ? std::max(0, count - step)
                        : std::min(count - 1, step - 1);

                    shade_colors[shade] = preferred_colors[index].Index;
                }
            };

        select_shades(true);
        select_shades(false);

        shade_lookup = shade_colors;
    }

    // NOTE: allegro's putpixel ignores clipping (optimization),
    // so we'll have to accomodate for that ourselves
    Rect clip = ds.GetClip();
    for (int code = ugetxc(&text); code; code = ugetxc(&text))
    {
        x += char_spacing + RenderChar(&ds, x, y, clip, font->GetChar(code),
            params.SizeMultiplier, colour, shade_lookup,
            font->GetBaseShadeIndex(), silhouette);
    }

    set_our_eip(oldeip);
}

static int RenderChar(Bitmap* ds, const int at_x, const int at_y, Rect clip,
    const WFNChar& wfn_char, const int scale, const color_t text_color,
    const color_t* shade_colors, const uint8_t base_shade,
    bool silhouette)
{
    const int width = wfn_char.Width;
    const int height = wfn_char.Height;
    const unsigned char* actdata = wfn_char.Data;
    const int bytewid = wfn_char.GetRowByteCount();

    int sx = std::max(at_x, clip.Left), ex = clip.Right + 1;
    int sy = std::max(at_y, clip.Top), ey = clip.Bottom + 1;
    int sw = std::max(0, clip.Left - at_x);
    int sh = std::max(0, clip.Top - at_y);

    for (int h = sh, y = sy; h < height && y < ey; ++h, y += scale)
    {
        for (int w = sw, x = sx; w < width && x < ex; ++w, x += scale)
        {
            color_t pixel_color = text_color;
            bool draw_pixel = false;

            if (wfn_char.BitsPerPixel == 4)
            {
                const uint8_t packed = actdata[h * bytewid + (w / 2)];
                const uint8_t shade = (w & 1) ? (packed & 0x0F) : (packed >> 4);

                if (shade != 0)
                {
                    draw_pixel = true;

                    const int depth = ds->GetColorDepth();
                    if (!silhouette && depth > 8 && shade != base_shade)
                    {
                        const int base_red = getr_depth(depth, text_color);
                        const int base_green = getg_depth(depth, text_color);
                        const int base_blue = getb_depth(depth, text_color);

                        int red, green, blue;

                        // Apply fixed RGB increments of 17 per shade relative to the base color.
                        // Shade values below the base darken; values above it brighten.
                        // Clamp each component to the valid 0-255 range.
                        const int offset = (static_cast<int>(shade) - base_shade) * 17;

                        red = std::max(0, std::min(255, base_red + offset));
                        green = std::max(0, std::min(255, base_green + offset));
                        blue = std::max(0, std::min(255, base_blue + offset));

                        pixel_color = (depth == 32)
                            ? makeacol32(red, green, blue, geta32(text_color))
                            : makecol_depth(depth, red, green, blue);
                    }
                    else if (!silhouette && shade_colors != nullptr)
                    {
                        pixel_color = shade_colors[shade];
                    }
                }
            }
            else
            {
                draw_pixel =
                    (actdata[h * bytewid + (w / 8)] & (0x80 >> (w % 8))) != 0;
            }

            if (draw_pixel)
            {
                if (scale > 1)
                {
                    ds->FillRect(RectWH(x, y, scale, scale), pixel_color);
                }
                else
                {
                    ds->PutPixel(x, y, pixel_color);
                }
            }
        }
    }

    return width * scale;
}

bool WFNFontRenderer::LoadFromDisk(int fontNumber, int fontSize)
{
  return LoadFromDiskEx(fontNumber, fontSize, String(), nullptr, nullptr, nullptr);
}

bool WFNFontRenderer::IsBitmapFont()
{
    return true;
}

int WFNFontRenderer::GetFontHeight(int fontNumber)
{
    return _fontData[fontNumber].Font->GetHeight(); // return real height
}

bool WFNFontRenderer::LoadFromDiskEx(int fontNumber, int /*fontSize*/, const String &filename,
    String *src_filename, const FontRenderParams *params, FontMetrics *metrics)
{
  String use_filename = !filename.IsEmpty() ? filename :
    String::FromFormat("agsfnt%d.wfn", fontNumber);
  auto ffi = _amgr->OpenAsset(use_filename);
  if (!ffi)
    return false;

  WFNFont *font = new WFNFont();
  WFNError err = font->ReadFromFile(ffi.get());
  if (err == kWFNErr_HasBadCharacters)
  {
    Debug::Printf(kDbgMsg_Warn, "WARNING: font '%s' has mistakes in data format, some characters may be displayed incorrectly", use_filename.GetCStr());
  }
  else if (err != kWFNErr_NoError)
  {
    delete font;
    return false;
  }
  _fontData[fontNumber].Font = font;
  _fontData[fontNumber].Params = params ? *params : FontRenderParams();
  _fontData[fontNumber].CharacterSpacing = 0;
  if (src_filename)
    *src_filename = use_filename;
  if (metrics)
    GetFontMetrics(fontNumber, metrics);
  return true;
}

void WFNFontRenderer::GetFontMetrics(int fontNumber, FontMetrics *metrics)
{
    const WFNFont *font = _fontData[fontNumber].Font;
    const int size_mult = _fontData[fontNumber].Params.SizeMultiplier;
    // Use this magic string to calculate a "nominal height" for WFN.
    // This is very silly, but has to be done for compatibility reasons.
    // WFN's "real height" may be bit higher because it may contain occasional larger glyphs.
    // TODO: this has to be changed in a future version (AGS 4?) because there's
    // no real guarantee that the font even has these letters in it.
    const char *height_test_string = "ZHwypgfjqhkilIK";
    metrics->NominalHeight = GetTextHeight(height_test_string, fontNumber);
    metrics->RealHeight = font->GetHeight() * size_mult;
    metrics->LogicalHeight = metrics->NominalHeight; // just set to default here
    metrics->BBox = font->GetBBox() * size_mult;
    metrics->VExtent = std::make_pair(metrics->BBox.Top, metrics->BBox.Bottom);
    // fix it up to be *not less* than realheight
    metrics->VExtent.first = std::min(0, metrics->VExtent.first);
    metrics->VExtent.second = std::max(metrics->RealHeight - 1, metrics->VExtent.second);
}

void WFNFontRenderer::GetCharCodeRange(int fontNumber, std::pair<int, int> *char_codes)
{
    if (_fontData[fontNumber].Font->GetCharCount() == 0)
        *char_codes = std::make_pair(0, 0);
    else
        *char_codes = std::make_pair(0,
            static_cast<int>(_fontData[fontNumber].Font->GetCharCount() - 1));
}

void WFNFontRenderer::GetValidCharCodes(int fontNumber, std::vector<int> &char_codes)
{
    const WFNFont *font = _fontData[fontNumber].Font;
    for (int i = 0; i < font->GetCharCount(); ++i)
    {
        if (font->GetChar(i).IsValid())
            char_codes.push_back(i);
    }
}

void WFNFontRenderer::FreeMemory(int fontNumber)
{
  delete _fontData[fontNumber].Font;
  _fontData.erase(fontNumber);
}

bool WFNFontRenderer::SupportsExtendedCharacters(int fontNumber)
{
  return _fontData[fontNumber].Font->GetCharCount() > 128;
}

void WFNFontRenderer::SetCharacterSpacing(int fontNumber, int spacing)
{
    if (_fontData.find(fontNumber) != _fontData.end())
    {
        _fontData[fontNumber].CharacterSpacing = spacing;
    }
}

void WFNFontRenderer::SetPaletteEligibility(const uint8_t* palette_eligible)
{
    if (palette_eligible)
    {
        std::memcpy(_paletteEligible, palette_eligible, sizeof(_paletteEligible));
        _hasPaletteEligibility = true;
    }
    else
    {
        _hasPaletteEligibility = false;
    }
}

int WFNFontRenderer::GetFontBitsPerPixel(int fontNumber) const
{
    auto it = _fontData.find(fontNumber);

    if (it == _fontData.end() || it->second.Font == nullptr)
        return 0;

    return it->second.Font->GetBitsPerPixel();
}
