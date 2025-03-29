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
#ifndef __AGS_CN_FONT__FONTS_H
#define __AGS_CN_FONT__FONTS_H

#include <string>
#include <utility>
#include <vector>
#include "ac/gamestructdefines.h"
#include "core/assetmanager.h"
#include "gfx/bitmap.h"
#include "util/string.h"

// TODO: we need to make some kind of TextManager class of this module

class IAGSFontRenderer;
class IAGSFontRenderer2;
class IAGSFontRendererInternal;
struct FontInfo;
struct FontRenderParams;
struct FontMetrics;

void init_font_renderer(AGS::Common::AssetManager *amgr);
void shutdown_font_renderer();
void adjust_y_coordinate_for_text(int* ypos, int font_number);
IAGSFontRenderer* font_replace_renderer(int font_number, IAGSFontRenderer* renderer);
IAGSFontRenderer* font_replace_renderer(int font_number, IAGSFontRenderer2* renderer);
void font_recalc_metrics(int font_number);
bool font_first_renderer_loaded();
bool is_font_loaded(int font_number);
bool is_bitmap_font(int font_number);
bool font_supports_extended_characters(int font_number);
// Get font's name, if it's available, otherwise returns empty string
const char *get_font_name(int font_number);
// Get a collection of FFLG_* flags corresponding to this font
int get_font_flags(int font_number);
// TODO: with changes to WFN font renderer that implemented safe rendering of
// strings containing invalid chars (since 3.3.1) this function is not
// important, except for (maybe) few particular cases.
// Furthermore, its use complicated things, because AGS could modify some texts
// at random times (usually - drawing routines).
// Need to check whether it is safe to completely remove it.
void ensure_text_valid_for_font(char *text, int font_number);
// Get font's scaling multiplier
int get_font_scaling_mul(int font_number);
// Calculate actual width of a line of text
int get_text_width(const char *texx, int font_number);
// Get the maximal width of the given font, with corresponding outlining
int get_text_width_outlined(const char *text, int font_number);
// Get font's height; this value is used for logical arrangement of UI elements;
// note that this is a "formal" font height, that may have different value
// depending on compatibility mode (used when running old games);
int get_font_height(int font_number);
// Get the maximal height of the given font, with corresponding outlining
int get_font_height_outlined(int font_number);
// Get font's surface height: this always returns the height enough to accomodate
// font letters on a bitmap or a texture; the distinction is needed for compatibility reasons
int get_font_surface_height(int font_number);
// Get font's maximal graphical extent: this means the farthest vertical positions of glyphs,
// relative to the "pen" position. Besides letting to calculate the surface height,
// this information also lets to detect if some of the glyphs may appear above y0.
std::pair<int, int> get_font_surface_extent(int font_number);
// Get font's line spacing
int get_font_linespacing(int font_number);
// Set font's line spacing
void set_font_linespacing(int font_number, int spacing);
// Get font's outline type
int  get_font_outline(int font_number);
// Get font's automatic outline thickness (if set)
int  get_font_outline_thickness(int font_number);
// Gets the total maximal height of the given number of lines printed with the given font;
// note that this uses formal font height, for compatibility purposes
int get_text_lines_height(int font_number, size_t numlines);
// Gets the height of a graphic surface enough to accomodate this number of text lines;
// note this accounts for the real pixel font height
int get_text_lines_surf_height(int font_number, size_t numlines);
// Set font's outline type
void set_font_outline(int font_number, int outline_type,
    enum FontInfo::AutoOutlineStyle style = FontInfo::kSquared, int thickness = 1);
bool is_font_antialiased(int font_number);
// Outputs a single line of text on the defined position on bitmap, using defined font, color and parameters
void wouttextxy(AGS::Common::Bitmap *ds, int xxx, int yyy, int font_number, color_t text_color, const char *texx);
// Assigns FontInfo to the font
void set_fontinfo(int font_number, const FontInfo &finfo);
// Gets full information about the font
FontInfo get_fontinfo(int font_number);
// Loads a font from disk
bool load_font_size(int font_number, const FontInfo &font_info);
// Loads a font from disk, reads metrics, and disposes a font
bool load_font_metrics(const AGS::Common::String &filename, int pixel_size, FontMetrics &metrics);
// FIXME: review this function, used only in AGS.Native (editor)
void wgtprintf(AGS::Common::Bitmap *ds, int xxx, int yyy, int font_number, color_t text_color, char *fmt, ...);
// Allocates two outline stencil buffers, or returns previously creates ones;
// these buffers are owned by the font, they should not be deleted by the caller.
void alloc_font_outline_buffers(int font_number,
    AGS::Common::Bitmap **text_stencil, AGS::Common::Bitmap **outline_stencil,
    int text_width, int text_height, int color_depth);
// Perform necessary adjustments on all fonts in case the text render mode changed (anti-aliasing etc)
void adjust_fonts_for_render_mode(bool aa_mode);
// Free particular font's data
void freefont(int font_number);
// Moves font data from one index to another; previous index becomes empty
void movefont(size_t old_font_num, size_t new_font_num);
// Free all fonts data
void free_all_fonts();

// Tells if the text should be antialiased when possible
bool ShouldAntiAliasText();

// SplitLines class represents a list of lines and is meant to reduce
// subsequent memory (de)allocations if used often during game loops
// and drawing. For that reason it is not equivalent to std::vector,
// but keeps constructed String buffers intact for most time.
// TODO: implement proper strings pool.
class SplitLines
{
public:
    inline size_t Count() const { return _count; }
    inline const AGS::Common::String &operator[](size_t i) const { return _pool[i]; }
    inline AGS::Common::String &operator[](size_t i) { return _pool[i]; }
    inline void Clear() { _pool.clear(); _count = 0; }
    inline void Reset() { _count = 0; }
    inline void Add(const char *cstr)
    {
        if (_pool.size() == _count) _pool.resize(_count + 1);
        _pool[_count++].SetString(cstr);
    }
    inline const std::vector<AGS::Common::String> &GetVector() const { return _pool; }

    // Auxiliary line processing buffers
    std::string LineBuf[2];

private:
    std::vector<AGS::Common::String> _pool;
    size_t _count; // actual number of lines in use
};

// Break up the text into lines restricted by the given width;
// returns number of lines, or 0 if text cannot be split well to fit in this width
size_t split_lines(const char *texx, SplitLines &lines, int width, int font_number, size_t max_lines = -1);

namespace AGS { namespace Common { extern SplitLines Lines; } }

#endif // __AGS_CN_FONT__FONTS_H
