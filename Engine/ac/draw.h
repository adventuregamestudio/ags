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

#include "util/stdtr1compat.h"
#include TR1INCLUDE(memory)
#include "core/types.h"
#include "ac/common_defines.h"
#include "gfx/gfx_def.h"
#include "util/wgt2allg.h"

namespace AGS
{
    namespace Common
    {
        class Bitmap;
        typedef stdtr1compat::shared_ptr<Common::Bitmap> PBitmap;
    }
    namespace Engine { class IDriverDependantBitmap; }
}
using namespace AGS; // FIXME later

#define IS_ANTIALIAS_SPRITES usetup.enable_antialiasing && (play.disable_antialiasing == 0)

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

// Converts AGS color index to the actual bitmap color using game's color depth
int MakeColor(int color_index);

// Initializes drawing methods and optimisation
void init_draw_method();
// Disposes resources related to the current drawing methods
void dispose_draw_method();
// Disposes any temporary resources on leaving current room
void dispose_room_drawdata();
// Updates drawing settings depending on main viewport's size and position on screen
void on_mainviewport_changed();
// Updates drawing settings if room viewport's position or size has changed
void on_roomviewport_changed();
// Updates drawing settings if room camera's size has changed
void on_camera_size_changed();

// whether there are currently remnants of a DisplaySpeech
void mark_screen_dirty();
bool is_screen_dirty();

// marks whole screen as needing a redraw
void invalidate_screen();
// marks certain rectangle on screen as needing a redraw
// in_room flag tells how to interpret the coordinates: as in-room coords or screen viewport coordinates.
void invalidate_rect(int x1, int y1, int x2, int y2, bool in_room);

void mark_current_background_dirty();
void invalidate_cached_walkbehinds();
// Avoid freeing and reallocating the memory if possible
Common::Bitmap *recycle_bitmap(Common::Bitmap *bimp, int coldep, int wid, int hit, bool make_transparent = false);
Engine::IDriverDependantBitmap* recycle_ddb_bitmap(Engine::IDriverDependantBitmap *bimp, Common::Bitmap *source, bool hasAlpha = false, bool opaque = false);
void update_screen();
// Draw everything 
void render_graphics(Engine::IDriverDependantBitmap *extraBitmap = NULL, int extraX = 0, int extraY = 0);
void construct_virtual_screen(bool fullRedraw) ;
void add_to_sprite_list(Engine::IDriverDependantBitmap* spp, int xx, int yy, int baseline, int trans, int sprNum, bool isWalkBehind = false);
void tint_image (Common::Bitmap *g, Common::Bitmap *source, int red, int grn, int blu, int light_level, int luminance=255);
void draw_sprite_support_alpha(Common::Bitmap *ds, bool ds_has_alpha, int xpos, int ypos, Common::Bitmap *image, bool src_has_alpha,
                               Common::BlendMode blend_mode = Common::kBlendMode_Alpha, int alpha = 0xFF);
void draw_sprite_slot_support_alpha(Common::Bitmap *ds, bool ds_has_alpha, int xpos, int ypos, int src_slot,
                                    Common::BlendMode blend_mode = Common::kBlendMode_Alpha, int alpha = 0xFF);
// CLNUP I'd like to put the default parameters to draw_gui_sprite, but the extern from guiman.h prevents it
void draw_gui_sprite(Common::Bitmap *ds, int pic, int x, int y, bool use_alpha, Common::BlendMode blend_mode);
//void draw_gui_sprite_v330(Common::Bitmap *ds, int pic, int x, int y, bool use_alpha = true, Common::BlendMode blend_mode = Common::kBlendMode_Alpha);
// Render game on screen with the given custom offset
void render_to_screen(int atx = 0, int aty = 0);
void draw_screen_callback();
void write_screen();
void GfxDriverOnInitCallback(void *data);
bool GfxDriverNullSpriteCallback(int x, int y);
void destroy_invalid_regions();
void putpixel_compensate (Common::Bitmap *g, int xx,int yy, int col);
// create the actsps[aa] image with the object drawn correctly
// returns 1 if nothing at all has changed and actsps is still
// intact from last time; 0 otherwise
int construct_object_gfx(int aa, int *drawnWidth, int *drawnHeight, bool alwaysUseSoftware);
void clear_letterbox_borders();

void draw_and_invalidate_text(Common::Bitmap *ds, int x1, int y1, int font, color_t text_color, const char *text);

void setpal();

// Checks if the bitmap needs to be converted and **deletes original** if a new bitmap
// had to be created (by default).
// TODO: this helper function was meant to remove bitmap deletion from the GraphicsDriver's
// implementations while keeping code changes to minimum. The proper solution would probably
// be to use shared pointers when storing Bitmaps, or make Bitmap reference-counted object.
Common::Bitmap *ReplaceBitmapWithSupportedFormat(Common::Bitmap *bitmap);
// Checks if the bitmap needs any kind of adjustments before it may be used
// in AGS sprite operations. Also handles number of certain special cases
// (old systems or uncommon gfx modes, and similar stuff).
// Original bitmap **gets deleted** if a new bitmap had to be created.
Common::Bitmap *PrepareSpriteForUse(Common::Bitmap *bitmap, bool has_alpha);
// Same as above, but compatible for std::shared_ptr.
Common::PBitmap PrepareSpriteForUse(Common::PBitmap bitmap, bool has_alpha);
// Makes a screenshot corresponding to the last screen render and returns it as a bitmap
// of the requested width and height and game's native color depth.
Common::Bitmap *CopyScreenIntoBitmap(int width, int height, bool at_native_res = false);


// Subsection of a virtual screen, used by the Software renderer
// when the room size is smaller than the game's size.
extern Common::Bitmap *sub_vscreen;

#endif // __AGS_EE_AC__DRAW_H
