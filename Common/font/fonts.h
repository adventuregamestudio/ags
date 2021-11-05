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
#ifndef __AC_FONT_H
#define __AC_FONT_H

#include <vector>
#include "ac/gamestructdefines.h"
#include "util/string.h"

// TODO: we need to make some kind of TextManager class of this module

namespace AGS { namespace Common { class Bitmap; } }
using namespace AGS;

// Font load flags, primarily for backward compatibility
// REPORTNOMINALHEIGHT: get_font_height should return nominal font's height,
// eq to "font size" parameter, otherwise returns real pixel height.
#define FONT_LOAD_REPORTNOMINALHEIGHT 0x01
// ASCENDFIXUP: do the TTF ascender fixup, where font's ascender is resized
// to the nominal font's height.
#define FONT_LOAD_ASCENDERFIXUP       0x02
// Collection of flags defining fully backward compatible TTF behavior
#define FONT_LOAD_FULLBACKCOMPAT      (FONT_LOAD_REPORTNOMINALHEIGHT | FONT_LOAD_ASCENDERFIXUP)

class IAGSFontRenderer;
class IAGSFontRenderer2;
struct FontInfo;
struct FontRenderParams;

void init_font_renderer();
void shutdown_font_renderer();
void adjust_y_coordinate_for_text(int* ypos, size_t fontnum);
IAGSFontRenderer* font_replace_renderer(size_t fontNumber, IAGSFontRenderer* renderer, int load_mode);
bool font_first_renderer_loaded();
bool is_font_loaded(size_t fontNumber);
bool is_bitmap_font(size_t fontNumber);
bool font_supports_extended_characters(size_t fontNumber);
// Get font's name, if it's available, otherwise returns empty string
const char *get_font_name(size_t fontNumber);
// Get a collection of FFLG_* flags corresponding to this font
int get_font_flags(size_t fontNumber);
// TODO: with changes to WFN font renderer that implemented safe rendering of
// strings containing invalid chars (since 3.3.1) this function is not
// important, except for (maybe) few particular cases.
// Furthermore, its use complicated things, because AGS could modify some texts
// at random times (usually - drawing routines).
// Need to check whether it is safe to completely remove it.
void ensure_text_valid_for_font(char *text, size_t fontnum);
// Get font's scaling multiplier
int get_font_scaling_mul(size_t fontNumber);
// Calculate actual width of a line of text
int get_text_width(const char *texx, size_t fontNumber);
// Get font's height; this value is used for logical arrangement of UI elements;
// note that this is a "formal" font height, that may have different value
// depending on compatibility mode (used when running old games);
int get_font_height(size_t fontNumber);
// Get the maximal height of the given font, with corresponding outlining
int get_font_height_outlined(size_t fontNumber);
// Get font's surface height: this always returns the height enough to accomodate
// font letters on a bitmap or a texture; the distinction is needed for compatibility reasons
int get_font_surface_height(size_t fontNumber);
// Get font's line spacing
int get_font_linespacing(size_t fontNumber);
// Set font's line spacing
void set_font_linespacing(size_t fontNumber, int spacing);
// Get font's outline type
int  get_font_outline(size_t font_number);
// Get font's automatic outline thickness (if set)
int  get_font_outline_thickness(size_t font_number);
// Gets the total maximal height of the given number of lines printed with the given font;
// note that this uses formal font height, for compatibility purposes
int get_text_lines_height(size_t fontNumber, size_t numlines);
// Gets the height of a graphic surface enough to accomodate this number of text lines;
// note this accounts for the real pixel font height
int get_text_lines_surf_height(size_t fontNumber, size_t numlines);
// Set font's outline type
void set_font_outline(size_t font_number, int outline_type,
    enum FontInfo::AutoOutlineStyle style = FontInfo::kSquared, int thickness = 1);
// Outputs a single line of text on the defined position on bitmap, using defined font, color and parameters
void wouttextxy(Common::Bitmap *ds, int xxx, int yyy, size_t fontNumber, color_t text_color, const char *texx);
// Assigns FontInfo to the font
void set_fontinfo(size_t fontNumber, const FontInfo &finfo, int load_mode);
// Gets full information about the font
FontInfo get_fontinfo(size_t font_number);
// Loads a font from disk
bool load_font_size(size_t fontNumber, const FontInfo &font_info, int load_mode);
void wgtprintf(Common::Bitmap *ds, int xxx, int yyy, size_t fontNumber, color_t text_color, char *fmt, ...);
// Allocates two outline stencil buffers, or returns previously creates ones;
// these buffers are owned by the font, they should not be deleted by the caller.
void alloc_font_outline_buffers(size_t font_number,
    Common::Bitmap **text_stencil, Common::Bitmap **outline_stencil,
    int text_width, int text_height, int color_depth);
// Perform necessary adjustments on all fonts in case the text render mode changed (anti-aliasing etc)
void adjust_fonts_for_render_mode(bool aa_mode);
// Free particular font's data
void wfreefont(size_t fontNumber);
// Free all fonts data
void free_all_fonts();

// SplitLines class represents a list of lines and is meant to reduce
// subsequent memory (de)allocations if used often during game loops
// and drawing. For that reason it is not equivalent to std::vector,
// but keeps constructed String buffers intact for most time.
// TODO: implement proper strings pool.
class SplitLines
{
public:
    inline size_t Count() const { return _count; }
    inline const Common::String &operator[](size_t i) const { return _pool[i]; }
    inline Common::String &operator[](size_t i) { return _pool[i]; }
    inline void Clear() { _pool.clear(); _count = 0; }
    inline void Reset() { _count = 0; }
    inline void Add(const char *cstr)
    {
        if (_pool.size() == _count) _pool.resize(_count + 1);
        _pool[_count++].SetString(cstr);
    }

    // An auxiliary line processing buffer
    std::vector<char> LineBuf;

private:
    std::vector<Common::String> _pool;
    size_t _count; // actual number of lines in use
};

// Break up the text into lines restricted by the given width;
// returns number of lines, or 0 if text cannot be split well to fit in this width
size_t split_lines(const char *texx, SplitLines &lines, int width, int fontNumber, size_t max_lines = -1);

namespace AGS { namespace Common { extern SplitLines Lines; } }

#endif // __AC_FONT_H
