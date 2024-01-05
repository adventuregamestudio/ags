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
#include "font/fonts.h"
#include "font/ttffontrenderer.h"
#include "font/wfnfontrenderer.h"
#include "gfx/bitmap.h"
#include "gui/guidefines.h" // MAXLINE
#include "util/string_utils.h"
#include "util/utf8.h"


extern int get_fixed_pixel_size(int pixels);

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
};

} // Common
} // AGS

static std::vector<Font> fonts;
static TTFFontRenderer ttfRenderer;
static WFNFontRenderer wfnRenderer;


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


void init_font_renderer()
{
  alfont_init();
  alfont_text_mode(-1);
}

void shutdown_font_renderer()
{
  set_our_eip(9919);
  alfont_exit();
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

bool is_font_loaded(size_t fontNumber)
{
    return fontNumber < fonts.size() && fonts[fontNumber].Renderer != nullptr;
}

// Finish font's initialization
static void font_post_init(size_t fontNumber)
{
    Font &font = fonts[fontNumber];
    // If no font height property was provided, then try several methods,
    // depending on which interface is available
    if ((font.Metrics.Height == 0) && font.Renderer)
    {
        int height = 0;
        if (font.Renderer2)
            height = font.Renderer2->GetFontHeight(fontNumber);
        if (height <= 0)
        {
            // With the old renderer we have to rely on GetTextHeight;
            // the implementations of GetTextHeight are allowed to return varied
            // results depending on the text parameter.
            // We use special line of text to get more or less reliable font height.
            const char *height_test_string = "ZHwypgfjqhkilIK";
            height = font.Renderer->GetTextHeight(height_test_string, fontNumber);
        }
        
        font.Metrics.Height = std::max(0, height);
        font.Metrics.RealHeight = font.Metrics.Height;
    }
    // Use either nominal or real pixel height to define font's logical height
    // and default linespacing; logical height = nominal height is compatible with the old games
    font.Metrics.CompatHeight = (font.Info.Flags & FFLG_REPORTNOMINALHEIGHT) != 0 ?
        font.Metrics.Height : font.Metrics.RealHeight;

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
            linespacing = font.Renderer2->GetLineSpacing(fontNumber);
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

static void font_replace_renderer(size_t fontNumber,
    IAGSFontRenderer* renderer, IAGSFontRenderer2* renderer2)
{
    fonts[fontNumber].Renderer = renderer;
    fonts[fontNumber].Renderer2 = renderer2;
    // If this is one of our built-in font renderers, then correctly
    // reinitialize interfaces and font metrics
    if ((renderer == &ttfRenderer) || (renderer == &wfnRenderer))
    {
        fonts[fontNumber].RendererInt = static_cast<IAGSFontRendererInternal*>(renderer);
        fonts[fontNumber].RendererInt->GetFontMetrics(fontNumber, &fonts[fontNumber].Metrics);
    }
    // Otherwise, this is probably coming from plugin
    else
    {
        fonts[fontNumber].RendererInt = nullptr;
        fonts[fontNumber].Metrics = FontMetrics(); // reset to defaults
    }
    font_post_init(fontNumber);
}

IAGSFontRenderer* font_replace_renderer(size_t fontNumber, IAGSFontRenderer* renderer)
{
    if (fontNumber >= fonts.size())
        return nullptr;
    IAGSFontRenderer* old_render = fonts[fontNumber].Renderer;
    font_replace_renderer(fontNumber, renderer, nullptr);
    return old_render;
}

IAGSFontRenderer* font_replace_renderer(size_t fontNumber, IAGSFontRenderer2* renderer)
{
    if (fontNumber >= fonts.size())
        return nullptr;
    IAGSFontRenderer* old_render = fonts[fontNumber].Renderer;
    font_replace_renderer(fontNumber, renderer, renderer);
    return old_render;
}

void font_recalc_metrics(size_t fontNumber)
{
    if (fontNumber >= fonts.size())
        return;
    fonts[fontNumber].Metrics = FontMetrics();
    font_post_init(fontNumber);
}

bool is_bitmap_font(size_t fontNumber)
{
    if (fontNumber >= fonts.size() || !fonts[fontNumber].RendererInt)
        return false;
    return fonts[fontNumber].RendererInt->IsBitmapFont();
}

bool font_supports_extended_characters(size_t fontNumber)
{
  if (fontNumber >= fonts.size() || !fonts[fontNumber].Renderer)
    return false;
  return fonts[fontNumber].Renderer->SupportsExtendedCharacters(fontNumber);
}

const char *get_font_name(size_t fontNumber)
{
  if (fontNumber >= fonts.size() || !fonts[fontNumber].Renderer2)
    return "";
  const char *name = fonts[fontNumber].Renderer2->GetFontName(fontNumber);
  return name ? name : "";
}

int get_font_flags(size_t fontNumber)
{
    if (fontNumber >= fonts.size())
        return 0;
    return fonts[fontNumber].Info.Flags;
}

void ensure_text_valid_for_font(char *text, size_t fontnum)
{
  if (fontnum >= fonts.size() || !fonts[fontnum].Renderer)
    return;
  fonts[fontnum].Renderer->EnsureTextValidForFont(text, fontnum);
}

int get_font_scaling_mul(size_t fontNumber)
{
    if (fontNumber >= fonts.size() || !fonts[fontNumber].Renderer)
        return 0;
    return fonts[fontNumber].Info.SizeMultiplier;
}

int get_text_width(const char *texx, size_t fontNumber)
{
  if (fontNumber >= fonts.size() || !fonts[fontNumber].Renderer)
    return 0;
  return fonts[fontNumber].Renderer->GetTextWidth(texx, fontNumber);
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

int get_font_height(size_t fontNumber)
{
    if (fontNumber >= fonts.size() || !fonts[fontNumber].Renderer)
        return 0;
    return fonts[fontNumber].Metrics.CompatHeight;
}

int get_font_height_outlined(size_t fontNumber)
{
    if (fontNumber >= fonts.size() || !fonts[fontNumber].Renderer)
        return 0;
    int self_height = fonts[fontNumber].Metrics.CompatHeight;
    int outline = fonts[fontNumber].Info.Outline;
    if (outline < 0 || static_cast<size_t>(outline) > fonts.size())
    { // FONT_OUTLINE_AUTO or FONT_OUTLINE_NONE
        return self_height + 2 * fonts[fontNumber].Info.AutoOutlineThickness;
    }
    int outline_height = fonts[outline].Metrics.CompatHeight;
    return std::max(self_height, outline_height);
}

int get_font_surface_height(size_t fontNumber)
{
    if (fontNumber >= fonts.size() || !fonts[fontNumber].Renderer)
        return 0;
    return fonts[fontNumber].Metrics.RealHeight;
}

int get_font_linespacing(size_t fontNumber)
{
    if (fontNumber >= fonts.size())
        return 0;
    return fonts[fontNumber].LineSpacingCalc;
}

void set_font_linespacing(size_t fontNumber, int spacing)
{
    if (fontNumber < fonts.size())
    {
        fonts[fontNumber].Info.Flags &= ~FFLG_DEFLINESPACING;
        fonts[fontNumber].Info.LineSpacing = spacing;
        fonts[fontNumber].LineSpacingCalc = spacing;
    }
}

int get_text_lines_height(size_t fontNumber, size_t numlines)
{
    if (fontNumber >= fonts.size() || numlines == 0)
        return 0;
    return fonts[fontNumber].LineSpacingCalc * (numlines - 1) +
        (fonts[fontNumber].Metrics.CompatHeight +
         2 * fonts[fontNumber].Info.AutoOutlineThickness);
}

int get_text_lines_surf_height(size_t fontNumber, size_t numlines)
{
    if (fontNumber >= fonts.size() || numlines == 0)
        return 0;
    return fonts[fontNumber].LineSpacingCalc * (numlines - 1) +
        (fonts[fontNumber].Metrics.RealHeight +
         2 * fonts[fontNumber].Info.AutoOutlineThickness);
}

namespace AGS { namespace Common { SplitLines Lines; } }

// Replaces AGS-specific linebreak tags with common '\n'
static void unescape_script_string(const char *cstr, std::string &out)
{
    out.clear();
    // Handle the special case of the first char
    if (cstr[0] == '[')
    {
        out.push_back('\n');
        cstr++;
    }
    // Replace all other occurrences as they're found
    // NOTE: we do not need to decode utf8 here, because
    // we are only searching for low-code ascii chars.
    const char *off;
    for (off = cstr; *off; ++off)
    {
        if (*off != '[') continue;
        if (*(off - 1) == '\\')
        {
            // convert \[ into [
            out.insert(out.end(), cstr, off - 1);
            out.push_back('[');
        }
        else
        {
            // convert [ into \n
            out.insert(out.end(), cstr, off);
            out.push_back('\n');
        }
        cstr = off + 1;
    }
    out.insert(out.end(), cstr, off + 1);
}

// Break up the text into lines
size_t split_lines(const char *todis, SplitLines &lines, int wii, int fonnt, size_t max_lines) {
    // NOTE: following hack accomodates for the legacy math mistake in split_lines.
    // It's hard to tell how cruicial it is for the game looks, so research may be needed.
    // TODO: IMHO this should rely not on game format, but script API level, because it
    // defines necessary adjustments to game scripts. If you want to fix this, find a way to
    // pass this flag here all the way from game.options[OPT_BASESCRIPTAPI] (or game format).
    //
    // if (game.options[OPT_BASESCRIPTAPI] < $Your current version$)
    wii -= 1;

    lines.Reset();

    std::string &line_buf = lines.LineBuf[0];
    std::string &test_buf = lines.LineBuf[1];

    // Do all necessary preliminary conversions: unescape, etc
    unescape_script_string(todis, line_buf);

    // TODO: we NEED a proper utf8 string class, and refact all this mess!!
    // in this case we perhaps could use custom ITERATOR types that read
    // and write utf8 chars in std::strings or similar containers.
    test_buf.clear();
    const char *end_ptr = &line_buf.back(); // buffer end ptr
    const char *theline = &line_buf.front(); // sub-line ptr
    const char *scan_ptr = theline; // a moving scan pos
    const char *prev_ptr = scan_ptr; // previous scan pos
    const char *last_whitespace = nullptr; // last found whitespace

    while (true) {
        if (scan_ptr == end_ptr) {
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

void wouttextxy(Common::Bitmap *ds, int xxx, int yyy, size_t fontNumber, color_t text_color, const char *texx)
{
  if (fontNumber >= fonts.size())
    return;
  yyy += fonts[fontNumber].Info.YOffset;
  if (yyy > ds->GetClip().Bottom)
    return;                   // each char is clipped but this speeds it up

  if (fonts[fontNumber].Renderer != nullptr)
  {
    fonts[fontNumber].Renderer->RenderText(texx, fontNumber, (BITMAP*)ds->GetAllegroBitmap(), xxx, yyy, text_color);
  }
}

void set_fontinfo(size_t fontNumber, const FontInfo &finfo)
{
    if (fontNumber < fonts.size() && fonts[fontNumber].Renderer)
    {
        fonts[fontNumber].Info = finfo;
        font_post_init(fontNumber);
    }
}

FontInfo get_fontinfo(size_t font_number)
{
    if (font_number < fonts.size())
        return fonts[font_number].Info;
    return FontInfo();
}

// Loads a font from disk
bool load_font_size(size_t fontNumber, const FontInfo &font_info)
{
  if (fonts.size() <= fontNumber)
    fonts.resize(fontNumber + 1);
  else
    wfreefont(fontNumber);
  FontRenderParams params;
  params.SizeMultiplier = font_info.SizeMultiplier;
  params.LoadMode = (font_info.Flags & FFLG_LOADMODEMASK);
  FontMetrics metrics;

  if (ttfRenderer.LoadFromDiskEx(fontNumber, font_info.Size, &params, &metrics))
  {
    fonts[fontNumber].Renderer    = &ttfRenderer;
    fonts[fontNumber].Renderer2   = &ttfRenderer;
    fonts[fontNumber].RendererInt = &ttfRenderer;
  }
  else if (wfnRenderer.LoadFromDiskEx(fontNumber, font_info.Size, &params, &metrics))
  {
    fonts[fontNumber].Renderer    = &wfnRenderer;
    fonts[fontNumber].Renderer2   = &wfnRenderer;
    fonts[fontNumber].RendererInt = &wfnRenderer;
  }

  if (!fonts[fontNumber].Renderer)
      return false;

  fonts[fontNumber].Info = font_info;
  fonts[fontNumber].Metrics = metrics;
  font_post_init(fontNumber);
  return true;
}

void wgtprintf(Common::Bitmap *ds, int xxx, int yyy, size_t fontNumber, color_t text_color, char *fmt, ...)
{
  if (fontNumber >= fonts.size())
    return;

  char tbuffer[2000];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(tbuffer, sizeof(tbuffer), fmt, ap);
  va_end(ap);
  wouttextxy(ds, xxx, yyy, fontNumber, text_color, tbuffer);
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

void wfreefont(size_t fontNumber)
{
  if (fontNumber >= fonts.size())
    return;

  fonts[fontNumber].TextStencilSub.Destroy();
  fonts[fontNumber].OutlineStencilSub.Destroy();
  fonts[fontNumber].TextStencil.Destroy();
  fonts[fontNumber].OutlineStencil.Destroy();
  if (fonts[fontNumber].Renderer != nullptr)
    fonts[fontNumber].Renderer->FreeMemory(fontNumber);

  fonts[fontNumber].Renderer = nullptr;
}

void free_all_fonts()
{
    for (size_t i = 0; i < fonts.size(); ++i)
    {
        if (fonts[i].Renderer != nullptr)
            fonts[i].Renderer->FreeMemory(i);
    }
    fonts.clear();
}
