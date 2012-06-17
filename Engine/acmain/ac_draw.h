#ifndef __AC_DRAW_H
#define __AC_DRAW_H

#include "ali3d.h"

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
int Game_GetColorFromRGB(int red, int grn, int blu);
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

extern color palette[256];
extern COLOR_MAP maincoltable;

void setpal();

AGS_INLINE int convert_to_low_res(int coord);
AGS_INLINE int convert_back_to_high_res(int coord);
AGS_INLINE void multiply_up_coordinates_round_up(int *x, int *y);

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


#endif // __AC_DRAW_H