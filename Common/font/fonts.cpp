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
#include <cstdio>
#include <vector>
#include "ac/common.h" // set_our_eip
#include "ac/gamestructdefines.h"
#include "debug/out.h"
#include "font/fonts.h"
#include "font/ttffontrenderer.h"
#include "font/wfnfontrenderer.h"
#include "gfx/bitmap.h"
#include "gui/guidefines.h" // MAXLINE
#include "util/path.h"
#include "util/string_utils.h"
#include "util/utf8.h"


using namespace AGS::Common;

namespace AGS
{
namespace Common
{

struct Font
{
    // Classic font renderer interface
    IAGSFontRenderer   *Renderer = nullptr;
    // Extended font renderer interface (optional)
    IAGSFontRenderer2  *Renderer2 = nullptr;
    // Internal interface (only for built-in renderers)
    IAGSFontRendererInternal *RendererInt = nullptr;
    // A file this font's data was loaded from
    String              Filename;
    // Font's general properties
    FontInfo            Info;
    // Values received from the renderer and saved for the reference
    FontMetrics         Metrics;
    // Precalculated linespacing, based on font properties and compat settings
    int                 LineSpacingCalc = 0;

    // Outline buffers
    Bitmap TextStencil, TextStencilSub;
    Bitmap OutlineStencil, OutlineStencilSub;

    Font() = default;
    Font(const Font &font) = default;
    Font(Font &&font) = default;
    Font &operator =(const Font &font) = default;
    Font &operator =(Font &&font) = default;
};

} // Common
} // AGS

static std::vector<Font> fonts;
static std::unique_ptr<TTFFontRenderer> ttfRenderer;
static std::unique_ptr<WFNFontRenderer> wfnRenderer;


FontInfo::FontInfo()
    : Flags(0)
    , Size(0)
    , SizeMultiplier(1)
    , Outline(FONT_OUTLINE_NONE)
    , YOffset(0)
    , LineSpacing(0)
    , AutoOutlineStyle(kSquared)
    , AutoOutlineThickness(0)
{}


void init_font_renderer(AssetManager *amgr)
{
    ttfRenderer.reset(new TTFFontRenderer(amgr));
    wfnRenderer.reset(new WFNFontRenderer(amgr));
}

void shutdown_font_renderer()
{
    set_our_eip(9919);
    free_all_fonts();
    ttfRenderer.reset();
    wfnRenderer.reset();
}

inline bool assert_font_number(int font_number)
{
    return (font_number >= 0) && (static_cast<uint32_t>(font_number) < fonts.size());
}

inline bool assert_font_renderer(int font_number)
{
    return (font_number >= 0) && (static_cast<uint32_t>(font_number) < fonts.size())
        && (fonts[font_number].Renderer != nullptr);
}

void adjust_y_coordinate_for_text(int* ypos, int font_number)
{
    if (!assert_font_renderer(font_number))
        return;
    fonts[font_number].Renderer->AdjustYCoordinateForFont(ypos, font_number);
}

bool is_any_font_loaded()
{
    for (const auto &f : fonts)
    {
        if (f.Renderer)
            return true;
    }
    return false;
}

bool is_font_loaded(int font_number)
{
    return assert_font_renderer(font_number);
}

// Finish font's initialization
static void font_post_init(int font_number)
{
    Font &font = fonts[font_number];
    // If no font height property was provided, then try several methods,
    // depending on which interface is available
    if ((font.Metrics.NominalHeight == 0) && font.Renderer)
    {
        int height = 0;
        if (font.Renderer2)
            height = font.Renderer2->GetFontHeight(font_number);
        if (height <= 0)
        {
            // With the old renderer we have to rely on GetTextHeight;
            // the implementations of GetTextHeight are allowed to return varied
            // results depending on the text parameter.
            // We use special line of text to get more or less reliable font height.
            const char *height_test_string = "ZHwypgfjqhkilIK";
            height = font.Renderer->GetTextHeight(height_test_string, font_number);
        }

        font.Metrics.NominalHeight = std::max(0, height);
        font.Metrics.RealHeight = font.Metrics.NominalHeight;
        font.Metrics.VExtent = std::make_pair(0, font.Metrics.RealHeight);
    }
    // Use either nominal or real pixel height to define font's logical height
    // and default linespacing; logical height = nominal height is compatible with the old games
    font.Metrics.CompatHeight = (font.Info.Flags & FFLG_REPORTNOMINALHEIGHT) != 0 ?
        font.Metrics.NominalHeight : font.Metrics.RealHeight;

    if (font.Info.Outline != FONT_OUTLINE_AUTO)
    {
        font.Info.AutoOutlineThickness = 0;
    }

    // If no linespacing property was provided, then try several methods,
    // depending on which interface is available
    font.LineSpacingCalc = font.Info.LineSpacing;
    if (font.Info.LineSpacing == 0)
    {
        int linespacing = 0;
        if (font.Renderer2)
            linespacing = font.Renderer2->GetLineSpacing(font_number);
        if (linespacing > 0)
        {
            font.LineSpacingCalc = linespacing;
        }
        else
        {
            // Calculate default linespacing from the font height + outline thickness.
            font.Info.Flags |= FFLG_DEFLINESPACING;
            font.LineSpacingCalc = font.Metrics.CompatHeight + 2 * font.Info.AutoOutlineThickness;
        }
    }
}

static void font_replace_renderer(int font_number,
    IAGSFontRenderer* renderer, IAGSFontRenderer2* renderer2)
{
    fonts[font_number].Renderer = renderer;
    fonts[font_number].Renderer2 = renderer2;
    // If this is one of our built-in font renderers, then correctly
    // reinitialize interfaces and font metrics
    if ((renderer == ttfRenderer.get()) || (renderer == wfnRenderer.get()))
    {
        fonts[font_number].RendererInt = static_cast<IAGSFontRendererInternal*>(renderer);
        fonts[font_number].RendererInt->GetFontMetrics(font_number, &fonts[font_number].Metrics);
    }
    // Otherwise, this is probably coming from plugin
    else
    {
        fonts[font_number].RendererInt = nullptr;
        fonts[font_number].Metrics = FontMetrics(); // reset to defaults
    }
    font_post_init(font_number);
}

IAGSFontRenderer* font_replace_renderer(int font_number, IAGSFontRenderer* renderer)
{
    if (!assert_font_number(font_number))
        return nullptr;
    IAGSFontRenderer* old_render = fonts[font_number].Renderer;
    font_replace_renderer(font_number, renderer, nullptr);
    return old_render;
}

IAGSFontRenderer* font_replace_renderer(int font_number, IAGSFontRenderer2* renderer)
{
    if (!assert_font_number(font_number))
        return nullptr;
    IAGSFontRenderer* old_render = fonts[font_number].Renderer;
    font_replace_renderer(font_number, renderer, renderer);
    return old_render;
}

void font_recalc_metrics(int font_number)
{
    if (!assert_font_number(font_number))
        return;
    fonts[font_number].Metrics = FontMetrics();
    font_post_init(font_number);
}

bool is_bitmap_font(int font_number)
{
    if (!assert_font_number(font_number) || !fonts[font_number].RendererInt)
        return false;
    return fonts[font_number].RendererInt->IsBitmapFont();
}

bool font_supports_extended_characters(int font_number)
{
    if (!assert_font_renderer(font_number))
        return false;
    return fonts[font_number].Renderer->SupportsExtendedCharacters(font_number);
}

const char *get_font_name(int font_number)
{
    if (!assert_font_number(font_number) || !fonts[font_number].Renderer2)
        return "";
    const char *name = fonts[font_number].Renderer2->GetFontName(font_number);
    return name ? name : "";
}

int get_font_flags(int font_number)
{
    if (!assert_font_number(font_number))
        return 0;
    return fonts[font_number].Info.Flags;
}

int get_font_scaling_mul(int font_number)
{
    if (!assert_font_number(font_number))
        return 0;
    return fonts[font_number].Info.SizeMultiplier;
}

int get_text_width(const char *texx, int font_number)
{
    if (!assert_font_renderer(font_number))
        return 0;
    return fonts[font_number].Renderer->GetTextWidth(texx, font_number);
}

int get_text_width_outlined(const char *text, int font_number)
{
    if (!assert_font_renderer(font_number))
        return 0;
    if(text == nullptr || text[0] == 0) // we ignore outline width since the text is empty
        return 0;

    int self_width = fonts[font_number].Renderer->GetTextWidth(text, font_number);
    int outline = fonts[font_number].Info.Outline;
    if (outline < 0 || static_cast<uint32_t>(outline) > fonts.size())
    { // FONT_OUTLINE_AUTO or FONT_OUTLINE_NONE
        return self_width + 2 * fonts[font_number].Info.AutoOutlineThickness;
    }
    int outline_width = fonts[outline].Renderer->GetTextWidth(text, outline);
    return std::max(self_width, outline_width);
}

int get_text_height(const char *text, int font_number)
{
    if (!assert_font_number(font_number))
        return 0;
    return fonts[font_number].Renderer->GetTextHeight(text, font_number);
}

int get_font_outline(int font_number)
{
    if (!assert_font_number(font_number))
        return FONT_OUTLINE_NONE;
    return fonts[font_number].Info.Outline;
}

int get_font_outline_thickness(int font_number)
{
    if (!assert_font_number(font_number))
        return 0;
    return fonts[font_number].Info.AutoOutlineThickness;
}

void set_font_outline(int font_number, int outline_type,
    enum FontInfo::AutoOutlineStyle style, int thickness)
{
    if (!assert_font_number(font_number))
        return;
    fonts[font_number].Info.Outline = outline_type;
    fonts[font_number].Info.AutoOutlineStyle = style;
    fonts[font_number].Info.AutoOutlineThickness = thickness;
}

bool is_font_antialiased(int font_number)
{
    if (!assert_font_number(font_number))
        return false;
    return ShouldAntiAliasText() && !is_bitmap_font(font_number);
}

int get_font_height(int font_number)
{
    if (!assert_font_number(font_number))
        return 0;
    return fonts[font_number].Metrics.CompatHeight;
}

int get_font_height_outlined(int font_number)
{
    if (!assert_font_number(font_number))
        return 0;
    int self_height = fonts[font_number].Metrics.CompatHeight;
    int outline = fonts[font_number].Info.Outline;
    if (outline < 0 || static_cast<uint32_t>(outline) > fonts.size())
    { // FONT_OUTLINE_AUTO or FONT_OUTLINE_NONE
        return self_height + 2 * fonts[font_number].Info.AutoOutlineThickness;
    }
    int outline_height = fonts[outline].Metrics.CompatHeight;
    return std::max(self_height, outline_height);
}

int get_font_surface_height(int font_number)
{
    if (!assert_font_number(font_number))
        return 0;
    return fonts[font_number].Metrics.ExtentHeight();
}

std::pair<int, int> get_font_surface_extent(int font_number)
{
    if (!assert_font_number(font_number))
        return std::make_pair(0, 0);
    return fonts[font_number].Metrics.VExtent;
}

int get_font_linespacing(int font_number)
{
    if (!assert_font_number(font_number))
        return 0;
    return fonts[font_number].LineSpacingCalc;
}

void set_font_linespacing(int font_number, int spacing)
{
    if (!assert_font_number(font_number))
        return;

    fonts[font_number].Info.Flags &= ~FFLG_DEFLINESPACING;
    fonts[font_number].Info.LineSpacing = spacing;
    fonts[font_number].LineSpacingCalc = spacing;
}

int get_text_lines_height(int font_number, size_t numlines)
{
    if (!assert_font_number(font_number) || numlines == 0)
        return 0;
    return fonts[font_number].LineSpacingCalc * (numlines - 1) +
        (fonts[font_number].Metrics.CompatHeight +
         2 * fonts[font_number].Info.AutoOutlineThickness);
}

int get_text_lines_surf_height(int font_number, size_t numlines)
{
    if (!assert_font_number(font_number) || numlines == 0)
        return 0;
    return fonts[font_number].LineSpacingCalc * (numlines - 1) +
        (fonts[font_number].Metrics.RealHeight +
         2 * fonts[font_number].Info.AutoOutlineThickness);
}

namespace AGS { namespace Common { SplitLines Lines; } }

// Break up the text into lines
size_t split_lines(const char *todis, SplitLines &lines, int wii, int fonnt, size_t max_lines)
{
    lines.Reset();

    std::string &test_buf = lines.LineBuf[0];

    // TODO: we NEED a proper utf8 string class, and refact all this mess!!
    // in this case we perhaps could use custom ITERATOR types that read
    // and write utf8 chars in std::strings or similar containers.
    test_buf.clear();
    const char *theline = todis; // sub-line ptr
    const char *scan_ptr = theline; // a moving scan pos
    const char *prev_ptr = scan_ptr; // previous scan pos
    const char *last_whitespace = nullptr; // last found whitespace

    while (true) {
        if (*scan_ptr == 0) {
            // end of the text, add the last line if necessary
            if (scan_ptr > theline) {
                lines.Add(test_buf.c_str());
            }
            break;
        }

        prev_ptr = scan_ptr; // save last scan pos

        if (*scan_ptr == ' ')
            last_whitespace = scan_ptr;

        const char *split_at = nullptr;
        // force end of line with the \n character
        if (*scan_ptr == '\n') {
            split_at = scan_ptr;
            ugetxc(&scan_ptr); // advance by char
        // otherwise, see if we are too wide
        } else {
            // copy next character to the test buffer and calculate its width
            char uch[Utf8::UtfSz + 1]{};
            usetc(uch, ugetxc(&scan_ptr)); // this advances scan_ptr
            test_buf.append(uch);
            if (get_text_width_outlined(test_buf.c_str(), fonnt) > wii) {
                // line is too wide, order the split
                if (last_whitespace)
                    // revert to the last whitespace
                    split_at = last_whitespace;
                else
                    // single very wide word, display as much as possible
                    split_at = prev_ptr;
            }
        }

        if (split_at != nullptr) {
            // check if even one char cannot fit...
            if (split_at == theline && !((*theline == ' ') || (*theline == '\n'))) {
                // cannot split with current width restriction
                lines.Reset();
                break;
            }
            // add this line, saved into the test buffer
            test_buf.resize(split_at - theline); // cut the buffer at the split index
            lines.Add(test_buf.c_str());
            test_buf.clear();
            // check if too many lines
            if (lines.Count() >= max_lines) {
                lines[lines.Count() - 1].Append("...");
                break;
            }
            // the next line starts from the split point
            theline = split_at;
            // skip the space or new line that caused the line break
            if ((*theline == ' ') || (*theline == '\n'))
                ugetxc(&theline); // advance by char
            scan_ptr = theline;
            prev_ptr = theline;
            last_whitespace = nullptr;
        }
    }
    return lines.Count();
}

void set_fontinfo(int font_number, const FontInfo &finfo)
{
    if (!assert_font_number(font_number))
        return;

    fonts[font_number].Info = finfo;
    font_post_init(font_number);
}

FontInfo get_fontinfo(int font_number)
{
    if (!assert_font_number(font_number))
        return FontInfo();

    return fonts[font_number].Info;
}

String get_font_file(int font_number)
{
    if (!assert_font_number(font_number))
        return String();

    return fonts[font_number].Filename;
}

bool load_font_size(int font_number, const FontInfo &font_info)
{
    return load_font_size(font_number, String(), font_info);
}

bool load_font_size(int font_number, const String &filename, const FontInfo &font_info)
{
    if (font_number < 0)
        return false;

    freefont(font_number);
    if (font_info.Filename.IsEmpty())
    {
        Debug::Printf(kDbgMsg_Error, "Font %d does not have any source filename assigned, can't be loaded.", font_number);
        return false;
    }

    if (fonts.size() <= static_cast<uint32_t>(font_number))
        fonts.resize(font_number + 1);
    else
        freefont(font_number);

    FontRenderParams params;
    params.SizeMultiplier = font_info.SizeMultiplier;
    params.LoadMode = (font_info.Flags & FFLG_LOADMODEMASK);
    FontMetrics metrics;

    Font &font = fonts[font_number];
    // TODO: make a font renderer factory which can iterate over a list of font renderers,
    // trying them in a sequnce, prioritizing one that matches file extension.
    if (ttfRenderer->LoadFromDiskEx(font_number, font_info.Size, font_info.Filename, &params, &metrics))
    {
        font.Renderer    = ttfRenderer.get();
        font.Renderer2   = ttfRenderer.get();
        font.RendererInt = ttfRenderer.get();
    }
    else if (wfnRenderer->LoadFromDiskEx(font_number, font_info.Size, font_info.Filename, &params, &metrics))
    {
        font.Renderer    = wfnRenderer.get();
        font.Renderer2   = wfnRenderer.get();
        font.RendererInt = wfnRenderer.get();
    }

    if (!font.Renderer)
    {
        Debug::Printf(kDbgMsg_Error, "Failed to load font %d with source filename %s: no font renderer was able to load or recognize font data.",
            font_number, font_info.Filename.GetCStr());
        return false;
    }

    font.Filename = filename;
    font.Info = font_info;
    font.Metrics = metrics;
    font_post_init(font_number);

    Debug::Printf("Loaded font %d: %s, req size: %d; nominal h: %d, real h: %d, extent: %d,%d",
        font_number, font_info.Filename.GetCStr(), font_info.Size, font.Metrics.NominalHeight, font.Metrics.RealHeight,
        font.Metrics.VExtent.first, font.Metrics.VExtent.second);
    return true;
}

bool load_font_metrics(const String &filename, int pixel_size, FontMetrics &metrics)
{
    const String ext = Path::GetFileExtension(filename);
    if (ext.CompareNoCase("ttf") == 0)
    {
        return ttfRenderer->MeasureFontOfPixelHeight(filename, pixel_size, &metrics);
    }
    else if (ext.CompareNoCase("wfn") == 0)
    {
        metrics = FontMetrics(); // FIXME: not supported?
        return false;
    }
    return false;
}

void alloc_font_outline_buffers(int font_number,
    Bitmap **text_stencil, Bitmap **outline_stencil,
    int text_width, int text_height, int color_depth)
{
    if (!assert_font_number(font_number))
        return;
    Font &f = fonts[font_number];
    const int thick = 2 * f.Info.AutoOutlineThickness;
    if (f.TextStencil.IsNull() || (f.TextStencil.GetColorDepth() != color_depth) ||
        (f.TextStencil.GetWidth() < text_width) || (f.TextStencil.GetHeight() < text_height))
    {
        int sw = f.TextStencil.IsNull() ? 0 : f.TextStencil.GetWidth();
        int sh = f.TextStencil.IsNull() ? 0 : f.TextStencil.GetHeight();
        sw = std::max(text_width, sw);
        sh = std::max(text_height, sh);
        f.TextStencil.Create(sw, sh, color_depth);
        f.OutlineStencil.Create(sw, sh + thick, color_depth);
        f.TextStencilSub.CreateSubBitmap(&f.TextStencil, RectWH(Size(text_width, text_height)));
        f.OutlineStencilSub.CreateSubBitmap(&f.OutlineStencil, RectWH(Size(text_width, text_height + thick)));
    }
    else
    {
        f.TextStencilSub.ResizeSubBitmap(text_width, text_height);
        f.OutlineStencilSub.ResizeSubBitmap(text_width, text_height + thick);
    }
    *text_stencil = &f.TextStencilSub;
    *outline_stencil = &f.OutlineStencilSub;
}

void adjust_fonts_for_render_mode(bool aa_mode)
{
    for (size_t i = 0; i < fonts.size(); ++i)
    {
        if (fonts[i].RendererInt)
            fonts[i].RendererInt->AdjustFontForAntiAlias(static_cast<int>(i), aa_mode);
    }
}

void font_set_blend_mode(size_t font_number, AGS::Common::BlendMode blend_mode)
{
    if (font_number >= fonts.size())
        return;

    if (fonts[font_number].RendererInt)
        fonts[font_number].RendererInt->SetBlendMode(blend_mode);
}

void freefont(int font_number)
{
    if (!assert_font_number(font_number))
        return;

    if (fonts[font_number].Renderer)
        fonts[font_number].Renderer->FreeMemory(font_number);
    fonts[font_number] = Font();
}

void movefont(int old_number, int new_number)
{
    if (!assert_font_number(old_number) || !assert_font_number(new_number))
        return;

    if (old_number == new_number)
        return; // same number

    fonts[new_number] = std::move(fonts[old_number]);
    fonts[old_number] = Font();
}

void free_all_fonts()
{
    for (size_t i = 0; i < fonts.size(); ++i)
    {
        if (fonts[i].Renderer)
            fonts[i].Renderer->FreeMemory(static_cast<int>(i));
    }
    fonts.clear();
}

void wouttextxy(Bitmap *ds, int x, int y, int font_number, color_t text_color, BlendMode blend_mode, const char *text)
{
    if (!assert_font_renderer(font_number))
        return;

    auto &font = fonts[font_number];
    y += font.Info.YOffset;
    if (y > ds->GetClip().Bottom)
        return; // each char is clipped but this speeds it up

    if (font.RendererInt)
        font.RendererInt->SetBlendMode(blend_mode);
    font.Renderer->RenderText(text, font_number, (BITMAP *)ds->GetAllegroBitmap(), x, y, text_color);
}

void wouttextxy(Bitmap *ds, int x, int y, int font_number, color_t text_color, const char *text)
{
    wouttextxy(ds, x, y, font_number, text_color, kBlend_Normal, text);
}

void woutprintf(Bitmap *ds, int x, int y, int font_number, color_t text_color, const char *fmt, ...)
{
    if (!assert_font_renderer(font_number))
        return;

    va_list ap;
    va_start(ap, fmt);
    String text = String::FromFormatV(fmt, ap);
    va_end(ap);
    wouttextxy(ds, x, y, font_number, text_color, text.GetCStr());
}
