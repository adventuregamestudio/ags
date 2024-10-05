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
#include <algorithm>
#include <cstdio>
#include <vector>
#include <alfont.h>
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

void adjust_y_coordinate_for_text(int* ypos, size_t fontnum)
{
    if (fontnum >= fonts.size() || !fonts[fontnum].Renderer)
        return;
    fonts[fontnum].Renderer->AdjustYCoordinateForFont(ypos, fontnum);
}

bool font_first_renderer_loaded()
{
    return fonts.size() > 0 && fonts[0].Renderer != nullptr;
}

bool is_font_loaded(size_t font_number)
{
    return font_number < fonts.size() && fonts[font_number].Renderer != nullptr;
}

// Finish font's initialization
static void font_post_init(size_t font_number)
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

static void font_replace_renderer(size_t font_number,
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

IAGSFontRenderer* font_replace_renderer(size_t font_number, IAGSFontRenderer* renderer)
{
    if (font_number >= fonts.size())
        return nullptr;
    IAGSFontRenderer* old_render = fonts[font_number].Renderer;
    font_replace_renderer(font_number, renderer, nullptr);
    return old_render;
}

IAGSFontRenderer* font_replace_renderer(size_t font_number, IAGSFontRenderer2* renderer)
{
    if (font_number >= fonts.size())
        return nullptr;
    IAGSFontRenderer* old_render = fonts[font_number].Renderer;
    font_replace_renderer(font_number, renderer, renderer);
    return old_render;
}

void font_recalc_metrics(size_t font_number)
{
    if (font_number >= fonts.size())
        return;
    fonts[font_number].Metrics = FontMetrics();
    font_post_init(font_number);
}

bool is_bitmap_font(size_t font_number)
{
    if (font_number >= fonts.size() || !fonts[font_number].RendererInt)
        return false;
    return fonts[font_number].RendererInt->IsBitmapFont();
}

bool font_supports_extended_characters(size_t font_number)
{
    if (font_number >= fonts.size() || !fonts[font_number].Renderer)
        return false;
    return fonts[font_number].Renderer->SupportsExtendedCharacters(font_number);
}

const char *get_font_name(size_t font_number)
{
    if (font_number >= fonts.size() || !fonts[font_number].Renderer2)
        return "";
    const char *name = fonts[font_number].Renderer2->GetFontName(font_number);
    return name ? name : "";
}

int get_font_flags(size_t font_number)
{
    if (font_number >= fonts.size())
        return 0;
    return fonts[font_number].Info.Flags;
}

void ensure_text_valid_for_font(char *text, size_t fontnum)
{
    if (fontnum >= fonts.size() || !fonts[fontnum].Renderer)
        return;
    fonts[fontnum].Renderer->EnsureTextValidForFont(text, fontnum);
}

int get_font_scaling_mul(size_t font_number)
{
    if (font_number >= fonts.size() || !fonts[font_number].Renderer)
        return 0;
    return fonts[font_number].Info.SizeMultiplier;
}

int get_text_width(const char *texx, size_t font_number)
{
    if (font_number >= fonts.size() || !fonts[font_number].Renderer)
        return 0;
    return fonts[font_number].Renderer->GetTextWidth(texx, font_number);
}

int get_text_width_outlined(const char *text, size_t font_number)
{
    if (font_number >= fonts.size() || !fonts[font_number].Renderer)
        return 0;
    if(text == nullptr || text[0] == 0) // we ignore outline width since the text is empty
        return 0;

    int self_width = fonts[font_number].Renderer->GetTextWidth(text, font_number);
    int outline = fonts[font_number].Info.Outline;
    if (outline < 0 || static_cast<size_t>(outline) > fonts.size())
    { // FONT_OUTLINE_AUTO or FONT_OUTLINE_NONE
        return self_width + 2 * fonts[font_number].Info.AutoOutlineThickness;
    }
    int outline_width = fonts[outline].Renderer->GetTextWidth(text, outline);
    return std::max(self_width, outline_width);
}

int get_font_outline(size_t font_number)
{
    if (font_number >= fonts.size())
        return FONT_OUTLINE_NONE;
    return fonts[font_number].Info.Outline;
}

int get_font_outline_thickness(size_t font_number)
{
    if (font_number >= fonts.size())
        return 0;
    return fonts[font_number].Info.AutoOutlineThickness;
}

void set_font_outline(size_t font_number, int outline_type,
    enum FontInfo::AutoOutlineStyle style, int thickness)
{
    if (font_number >= fonts.size())
        return;
    fonts[font_number].Info.Outline = outline_type;
    fonts[font_number].Info.AutoOutlineStyle = style;
    fonts[font_number].Info.AutoOutlineThickness = thickness;
}

bool is_font_antialiased(size_t font_number)
{
    if (font_number >= fonts.size())
        return false;
    return ShouldAntiAliasText() && !is_bitmap_font(font_number);
}

int get_font_height(size_t font_number)
{
    if (font_number >= fonts.size() || !fonts[font_number].Renderer)
        return 0;
    return fonts[font_number].Metrics.CompatHeight;
}

int get_font_height_outlined(size_t font_number)
{
    if (font_number >= fonts.size() || !fonts[font_number].Renderer)
        return 0;
    int self_height = fonts[font_number].Metrics.CompatHeight;
    int outline = fonts[font_number].Info.Outline;
    if (outline < 0 || static_cast<size_t>(outline) > fonts.size())
    { // FONT_OUTLINE_AUTO or FONT_OUTLINE_NONE
        return self_height + 2 * fonts[font_number].Info.AutoOutlineThickness;
    }
    int outline_height = fonts[outline].Metrics.CompatHeight;
    return std::max(self_height, outline_height);
}

int get_font_surface_height(size_t font_number)
{
    if (font_number >= fonts.size() || !fonts[font_number].Renderer)
        return 0;
    return fonts[font_number].Metrics.ExtentHeight();
}

std::pair<int, int> get_font_surface_extent(size_t font_number)
{
    if (font_number >= fonts.size() || !fonts[font_number].Renderer)
        return std::make_pair(0, 0);
    return fonts[font_number].Metrics.VExtent;
}

int get_font_linespacing(size_t font_number)
{
    if (font_number >= fonts.size())
        return 0;
    return fonts[font_number].LineSpacingCalc;
}

void set_font_linespacing(size_t font_number, int spacing)
{
    if (font_number < fonts.size())
    {
        fonts[font_number].Info.Flags &= ~FFLG_DEFLINESPACING;
        fonts[font_number].Info.LineSpacing = spacing;
        fonts[font_number].LineSpacingCalc = spacing;
    }
}

int get_text_lines_height(size_t font_number, size_t numlines)
{
    if (font_number >= fonts.size() || numlines == 0)
        return 0;
    return fonts[font_number].LineSpacingCalc * (numlines - 1) +
        (fonts[font_number].Metrics.CompatHeight +
         2 * fonts[font_number].Info.AutoOutlineThickness);
}

int get_text_lines_surf_height(size_t font_number, size_t numlines)
{
    if (font_number >= fonts.size() || numlines == 0)
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

void wouttextxy(Bitmap *ds, int xxx, int yyy, size_t font_number, color_t text_color, const char *texx)
{
    if (font_number >= fonts.size())
        return;
    yyy += fonts[font_number].Info.YOffset;
    if (yyy > ds->GetClip().Bottom)
        return;                   // each char is clipped but this speeds it up

    if (fonts[font_number].Renderer != nullptr)
    {
        fonts[font_number].Renderer->RenderText(texx, font_number, (BITMAP*)ds->GetAllegroBitmap(), xxx, yyy, text_color);
    }
}

void set_fontinfo(size_t font_number, const FontInfo &finfo)
{
    if (font_number < fonts.size() && fonts[font_number].Renderer)
    {
        fonts[font_number].Info = finfo;
        font_post_init(font_number);
    }
}

FontInfo get_fontinfo(size_t font_number)
{
    if (font_number < fonts.size())
        return fonts[font_number].Info;
    return FontInfo();
}

// Loads a font from disk
bool load_font_size(size_t font_number, const FontInfo &font_info)
{
    if (fonts.size() <= font_number)
        fonts.resize(font_number + 1);
    else
        freefont(font_number);

    FontRenderParams params;
    params.SizeMultiplier = font_info.SizeMultiplier;
    params.LoadMode = (font_info.Flags & FFLG_LOADMODEMASK);
    FontMetrics metrics;

    Font &font = fonts[font_number];
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
        return false;

    font.Info = font_info;
    font.Metrics = metrics;
    font_post_init(font_number);

    Debug::Printf("Loaded font %d: %s, req size: %d; nominal h: %d, real h: %d, extent: %d,%d",
        font_number, font_info.Filename.GetCStr(), font_info.Size, font.Metrics.NominalHeight, font.Metrics.RealHeight,
    font.Metrics.VExtent.first, font.Metrics.VExtent.second);
    return true;
}

bool load_font_metrics(const AGS::Common::String &filename, int pixel_size, FontMetrics &metrics)
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

void wgtprintf(Bitmap *ds, int xxx, int yyy, size_t font_number, color_t text_color, char *fmt, ...)
{
    if (font_number >= fonts.size())
        return;

    char tbuffer[2000];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tbuffer, sizeof(tbuffer), fmt, ap);
    va_end(ap);
    wouttextxy(ds, xxx, yyy, font_number, text_color, tbuffer);
}

void alloc_font_outline_buffers(size_t font_number,
    Bitmap **text_stencil, Bitmap **outline_stencil,
    int text_width, int text_height, int color_depth)
{
    if (font_number >= fonts.size())
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
            fonts[i].RendererInt->AdjustFontForAntiAlias(i, aa_mode);
    }
}

void freefont(size_t font_number)
{
    if (font_number >= fonts.size())
        return;

    if (fonts[font_number].Renderer)
        fonts[font_number].Renderer->FreeMemory(font_number);
    fonts[font_number] = Font();
}

void movefont(size_t old_number, size_t new_number)
{
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
            fonts[i].Renderer->FreeMemory(i);
    }
    fonts.clear();
}
