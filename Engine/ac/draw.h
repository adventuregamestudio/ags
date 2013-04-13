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
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__DRAW_H
#define __AGS_EE_AC__DRAW_H

#include "core/types.h"
#include "ac/common_defines.h"
#include "util/wgt2allg.h"

namespace AGS { namespace Common { class Bitmap; } }
namespace AGS { namespace Engine { class IDriverDependantBitmap; } }
using namespace AGS; // FIXME later

// [IKM] personally I do not see much sense in this,
// memcpyfast alias is used only once in the Engine
#define memcpyfast memcpy
#define IS_ANTIALIAS_SPRITES usetup.enable_antialiasing && (play.disable_antialiasing == 0)

// Allegro 4 has switched 15-bit colour to BGR instead of RGB, so
// in this case we need to convert the graphics on load
#if ALLEGRO_DATE > 19991010
#define USE_15BIT_FIX
#endif

// [IKM] WARNING: these definitions has to be made AFTER Allegro headers
// were included, because they override few Allegro function names;
// otherwise Allegro headers should not be included at all to the same
// code unit which uses these defines.
#define getr32(xx) ((xx >> _rgb_r_shift_32) & 0xFF)
#define getg32(xx) ((xx >> _rgb_g_shift_32) & 0xFF)
#define getb32(xx) ((xx >> _rgb_b_shift_32) & 0xFF)
#define geta32(xx) ((xx >> _rgb_a_shift_32) & 0xFF)
#define makeacol32(r,g,b,a) ((r << _rgb_r_shift_32) | (g << _rgb_g_shift_32) | (b << _rgb_b_shift_32) | (a << _rgb_a_shift_32))


struct CachedActSpsData {
    int xWas, yWas;
    int baselineWas;
    int isWalkBehindHere;
    int valid;
};

void invalidate_screen();
void mark_current_background_dirty();
void invalidate_cached_walkbehinds();
void put_sprite_256(Common::Graphics *g, int xxx,int yyy,Common::Bitmap *piccy);
Common::Bitmap *recycle_bitmap(Common::Bitmap *bimp, int coldep, int wid, int hit, bool make_transparent = false);
void push_screen (Common::Graphics *g);
Common::Graphics *pop_screen();
void update_screen();
void invalidate_rect(int x1, int y1, int x2, int y2);
// Draw everything 
void render_graphics(Engine::IDriverDependantBitmap *extraBitmap = NULL, int extraX = 0, int extraY = 0);
void construct_virtual_screen(bool fullRedraw) ;
void add_to_sprite_list(Engine::IDriverDependantBitmap* spp, int xx, int yy, int baseline, int trans, int sprNum, bool isWalkBehind = false);
void tint_image (Common::Graphics *g, Common::Bitmap *source, int red, int grn, int blu, int light_level, int luminance=255);
void draw_sprite_support_alpha(Common::Graphics *g, int xpos, int ypos, Common::Bitmap *image, int slot);
void render_to_screen(Common::Bitmap *toRender, int atx, int aty);
void draw_screen_callback();
void write_screen();
void GfxDriverOnInitCallback(void *data);
bool GfxDriverNullSpriteCallback(int x, int y);
void init_invalid_regions(int scrnHit);
int get_screen_x_adjustment(Common::Bitmap *checkFor);
int get_screen_y_adjustment(Common::Bitmap *checkFor);
void putpixel_compensate (Common::Graphics *g, int xx,int yy, int col);
// create the actsps[aa] image with the object drawn correctly
// returns 1 if nothing at all has changed and actsps is still
// intact from last time; 0 otherwise
int construct_object_gfx(int aa, int *drawnWidth, int *drawnHeight, bool alwaysUseSoftware);
void clear_letterbox_borders();

void draw_and_invalidate_text(Common::Graphics *g, int x1, int y1, int font, const char *text);
void wouttext_reverseifnecessary(Common::Graphics *g, int x, int y, int font, char *text);

void setpal();

extern AGS_INLINE int get_fixed_pixel_size(int pixels);
extern AGS_INLINE int convert_to_low_res(int coord);
extern AGS_INLINE int convert_back_to_high_res(int coord);
extern AGS_INLINE int multiply_up_coordinate(int coord);
extern AGS_INLINE void multiply_up_coordinates(int *x, int *y);
extern AGS_INLINE void multiply_up_coordinates_round_up(int *x, int *y);
extern AGS_INLINE int divide_down_coordinate(int coord);
extern AGS_INLINE int divide_down_coordinate_round_up(int coord);

Common::Bitmap *convert_16_to_15(Common::Bitmap *iii);
Common::Bitmap *convert_16_to_16bgr(Common::Bitmap *tempbl);
Common::Bitmap *convert_32_to_32bgr(Common::Bitmap *tempbl);

#endif // __AGS_EE_AC__DRAW_H
