#ifndef __AC_DRAW_H
#define __AC_DRAW_H

#include "ali3d.h"
#include "ac/ac_defines.h" // AGS_INLINE
#include "ac/rundefines.h"

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



void RawSaveScreen ();
// RawRestoreScreen: copy backup bitmap back to screen; we
// deliberately don't free the block cos they can multiple restore
// and it gets freed on room exit anyway
void RawRestoreScreen();
// Restores the backup bitmap, but tints it to the specified level
void RawRestoreScreenTinted(int red, int green, int blue, int opacity);
void RawDrawFrameTransparent (int frame, int translev);
void RawClear (int clr);
void RawSetColor (int clr);
void RawSetColorRGB(int red, int grn, int blu);
void RawPrint (int xx, int yy, char*texx, ...);
void RawPrintMessageWrapped (int xx, int yy, int wid, int font, int msgm);
void RawDrawImageCore(int xx, int yy, int slot);
void RawDrawImage(int xx, int yy, int slot);
void RawDrawImageOffset(int xx, int yy, int slot);
void RawDrawImageTransparent(int xx, int yy, int slot, int trans);
void RawDrawImageResized(int xx, int yy, int gotSlot, int width, int height);
void RawDrawLine (int fromx, int fromy, int tox, int toy);
void RawDrawCircle (int xx, int yy, int rad);
void RawDrawRectangle(int x1, int y1, int x2, int y2);
void RawDrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3);
void CyclePalette(int strt,int eend);
void SetPalRGB(int inndx,int rr,int gg,int bb);
void UpdatePalette();
void TintScreen(int red, int grn, int blu);


extern color palette[256];
extern COLOR_MAP maincoltable;

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

extern IGraphicsDriver *gfxDriver;
extern IDriverDependantBitmap *mouseCursor;
extern IDriverDependantBitmap *blankImage;
extern IDriverDependantBitmap *blankSidebarImage;
extern IDriverDependantBitmap *debugConsole;

extern block *actsps;
extern block virtual_screen; 

extern block dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];
extern int trans_mode;

// actsps is used for temporary storage of the bitamp image
// of the latest version of the sprite
extern int actSpsCount;
extern block *actsps;
extern IDriverDependantBitmap* *actspsbmp;
// temporary cache of walk-behind for this actsps image
extern block *actspswb;
extern IDriverDependantBitmap* *actspswbbmp;
extern CachedActSpsData* actspswbcache;

extern block *guibg;
extern IDriverDependantBitmap **guibgbmp;

extern int offsetx, offsety;

extern block raw_saved_screen;
extern block dotted_mouse_cursor;
extern block blank_mouse_cursor;

extern block _old_screen;
extern block _sub_screen;

extern int _places_r, _places_g, _places_b;

// whether there are currently remnants of a DisplaySpeech
extern int screen_is_dirty;

extern IDriverDependantBitmap* roomBackgroundBmp;


#endif // __AC_DRAW_H
