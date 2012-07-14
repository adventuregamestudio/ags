
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__DRAW_H
#define __AGS_EE_AC__DRAW_H

#include "ac/common_defines.h"
#include "ali3d.h"

// [IKM] personally I do not see much sense in this,
// memcpyfast alias is used only once in the Engine
#define memcpyfast memcpy
#define IS_ANTIALIAS_SPRITES usetup.enable_antialiasing && (play.disable_antialiasing == 0)

// Allegro 4 has switched 15-bit colour to BGR instead of RGB, so
// in this case we need to convert the graphics on load
#if ALLEGRO_DATE > 19991010
#define USE_15BIT_FIX
#endif

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
void put_sprite_256(int xxx,int yyy,block piccy);
block recycle_bitmap(block bimp, int coldep, int wid, int hit);
void push_screen ();
void pop_screen();
void update_screen();
void invalidate_rect(int x1, int y1, int x2, int y2);
// Draw everything 
void render_graphics(IDriverDependantBitmap *extraBitmap = NULL, int extraX = 0, int extraY = 0);
void construct_virtual_screen(bool fullRedraw) ;
void add_to_sprite_list(IDriverDependantBitmap* spp, int xx, int yy, int baseline, int trans, int sprNum, bool isWalkBehind = false);
void tint_image (block source, block dest, int red, int grn, int blu, int light_level, int luminance=255);
void draw_sprite_support_alpha(int xpos, int ypos, block image, int slot);
void render_to_screen(BITMAP *toRender, int atx, int aty);
void draw_screen_callback();
void write_screen();
void GfxDriverOnInitCallback(void *data);
bool GfxDriverNullSpriteCallback(int x, int y);
void init_invalid_regions(int scrnHit);
int get_screen_x_adjustment(BITMAP *checkFor);
int get_screen_y_adjustment(BITMAP *checkFor);
void putpixel_compensate (block onto, int xx,int yy, int col);
// create the actsps[aa] image with the object drawn correctly
// returns 1 if nothing at all has changed and actsps is still
// intact from last time; 0 otherwise
int construct_object_gfx(int aa, int *drawnWidth, int *drawnHeight, bool alwaysUseSoftware);
void clear_letterbox_borders();

void draw_and_invalidate_text(int x1, int y1, int font, const char *text);

void setpal();

extern AGS_INLINE int get_fixed_pixel_size(int pixels);
extern AGS_INLINE int convert_to_low_res(int coord);
extern AGS_INLINE int convert_back_to_high_res(int coord);
extern AGS_INLINE int multiply_up_coordinate(int coord);
extern AGS_INLINE void multiply_up_coordinates(int *x, int *y);
extern AGS_INLINE void multiply_up_coordinates_round_up(int *x, int *y);
extern AGS_INLINE int divide_down_coordinate(int coord);
extern AGS_INLINE int divide_down_coordinate_round_up(int coord);

block convert_16_to_15(block iii);
block convert_16_to_16bgr(block tempbl);

#endif // __AGS_EE_AC__DRAW_H
