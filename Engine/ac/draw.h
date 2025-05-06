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
#ifndef __AGS_EE_AC__DRAW_H
#define __AGS_EE_AC__DRAW_H

#include <memory>
#include "core/types.h"
#include "ac/common_defines.h"
#include "ac/runtime_defines.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"
#include "gfx/gfx_def.h"
#include "game/roomstruct.h"

namespace AGS
{
    namespace Common
    {
        typedef std::shared_ptr<Common::Bitmap> PBitmap;
    }
    namespace Engine
    {
        class IDriverDependantBitmap;
        class IGraphicShader;
        class IShaderInstance;
    }
}
using namespace AGS; // FIXME later

// Render stage flags, for filtering out certain elements
// during room transitions, capturing screenshots, etc.
// NOTE: these values are matched by ones in a script API,
// if you require a change, make sure the script parameters are converted on function call.
#define RENDER_BATCH_ENGINE_OVERLAY  0x00000001
#define RENDER_BATCH_MOUSE_CURSOR    0x00000002
#define RENDER_BATCH_UI_LAYER        0x00000004
#define RENDER_BATCH_ROOM_LAYER      0x00000008
#define RENDER_BATCH_ALL             0xFFFFFFFF
#define RENDER_SHOT_SKIP_ON_FADE     (RENDER_BATCH_ENGINE_OVERLAY | RENDER_BATCH_MOUSE_CURSOR)

// Converts AGS color index to the actual bitmap color using game's color depth
int MakeColor(int color_index);

class Viewport;
class Camera;

// Initializes drawing methods and optimisation
void init_draw_method();
// Initializes global game drawing resources
void init_game_drawdata();
// Initializes drawing resources upon entering new room
void init_room_drawdata();
// Disposes resources related to the current drawing methods
void dispose_draw_method();
// Disposes global game drawing resources
void dispose_game_drawdata();
// Disposes any temporary resources on leaving current room
void dispose_room_drawdata();
// Releases all the cached textures of game objects
void clear_drawobj_cache();
// Releases all the textures used as render targets, if necessary;
// (this is primarily for resetting display mode of certain renderers).
void release_drawobj_rendertargets();
// Updates drawing settings depending on main viewport's size and position on screen
void on_mainviewport_changed();
// Notifies that a new room viewport was created
void on_roomviewport_created(int index);
// Notifies that a new room viewport was deleted
void on_roomviewport_deleted(int index);
// Updates drawing settings if room viewport's position or size has changed
void on_roomviewport_changed(Viewport *view);
// Detects overlapping viewports, starting from the given index in z-sorted array
void detect_roomviewport_overlaps(size_t z_index);
// Updates drawing settings if room camera's size has changed
void on_roomcamera_changed(Camera *cam);
// Marks particular object as need to update the texture
void mark_object_changed(int objid);
// TODO: write a generic drawable/objcache system where each object
// allocates a drawable for itself, and disposes one if being removed.
// Resets drawing index for dynamic objects
void reset_drawobj_dynamic_index();
// Resets drawable object for this overlay
void reset_drawobj_for_overlay(int objnum);
// Marks all game objects which reference this sprite for redraw
void notify_sprite_changed(int sprnum, bool deleted);

// Get current texture cache's stats: max size, current normal items size,
// size of locked items (included into cur_size),
// size of external items (excluded from cur_size)
void texturecache_get_state(size_t &max_size, size_t &cur_size, size_t &locked_size, size_t &ext_size);
// Returns current cache size
size_t texturecache_get_size();
// Completely resets texture cache
void texturecache_clear();
// Update shared and cached texture from the sprite's pixels
void update_shared_texture(uint32_t sprite_id);
// Remove a texture from cache
void clear_shared_texture(uint32_t sprite_id);
// Prepares a texture for the given sprite and stores in the cache
void texturecache_precache(uint32_t sprite_id);

// Adds a custom shader to the collection at the desired index;
// if there's any shader on that index then it will get disposed of
void add_custom_shader(Engine::IGraphicShader *shader, uint32_t at_index);
// Gets a custom shader by its index
Engine::IGraphicShader *get_custom_shader(uint32_t sh_index);
// Deletes a custom shader; this also deletes their instances
void delete_custom_shader(uint32_t shader_id);
// Adds a custom shader instance to the collection at the desired index;
// if there's any shader instance on that index then it will get disposed of
void add_shader_instance(Engine::IShaderInstance *shader_instance, uint32_t at_index);
// Gets a shader instance by its index
Engine::IShaderInstance *get_shader_instance(uint32_t shinst_index);
// Deletes a custom shader instance
void delete_shader_instance(uint32_t shader_inst_id);
// Deletes all custom shaders and their isntances
void dispose_all_custom_shaders();

// Initializes a loaded sprite for use in the game, adjusts the sprite flags.
// Returns a resulting bitmap, which may be a new or old bitmap; or null on failure.
// Original bitmap **gets deleted** if a new bitmap had to be created,
// or if failed to properly initialize one.
Common::Bitmap *initialize_sprite(Common::sprkey_t index, Common::Bitmap *image, uint32_t &sprite_flags);
// Run after a sprite was loaded and adjusted for the game
void post_init_sprite(Common::sprkey_t index);

// whether there are currently remnants of a DisplaySpeech
void mark_screen_dirty();
bool is_screen_dirty();

// marks whole screen as needing a redraw
void invalidate_screen();
// marks all the camera frame as needing a redraw
void invalidate_camera_frame(int index);
// marks certain rectangle on screen as needing a redraw
// in_room flag tells how to interpret the coordinates: as in-room coords or screen viewport coordinates.
void invalidate_rect(int x1, int y1, int x2, int y2, bool in_room);

void mark_current_background_dirty();

// Avoid freeing and reallocating the memory if possible
Common::Bitmap *recycle_bitmap(Common::Bitmap *bimp, int coldep, int wid, int hit, bool make_transparent = false);
void recycle_bitmap(std::unique_ptr<Common::Bitmap> &bimp, int coldep, int wid, int hit, bool make_transparent = false);
Engine::IDriverDependantBitmap* recycle_ddb_bitmap(Engine::IDriverDependantBitmap *ddb,
    Common::Bitmap *source, bool opaque = false);
Engine::IDriverDependantBitmap* recycle_ddb_sprite(Engine::IDriverDependantBitmap *ddb,
    uint32_t sprite_id, Common::Bitmap *source, bool opaque = false);
Engine::IDriverDependantBitmap* recycle_render_target(Engine::IDriverDependantBitmap *ddb,
    int width, int height, int col_depth, bool opaque = false);

// Draw everything 
void render_graphics(Engine::IDriverDependantBitmap *extraBitmap = nullptr, int extraX = 0, int extraY = 0);
// Construct game scene, scheduling drawing list for the renderer
void construct_game_scene(bool full_redraw = false);
// Construct final game screen elements; updates and draws mouse cursor
void construct_game_screen_overlay(bool draw_mouse = true);
// Construct engine overlay with debugging tools (fps, console)
void construct_engine_overlay();

void update_shakescreen();

void debug_draw_room_mask(RoomAreaMask mask);
void debug_draw_movelist(int charnum);
void update_room_debug();
void tint_image (Common::Bitmap *g, Common::Bitmap *source, int red, int grn, int blu, int light_level, int luminance=255);
void draw_sprite_support_alpha(Common::Bitmap *ds, int xpos, int ypos, Common::Bitmap *image,
                               Common::BlendMode blend_mode = Common::kBlend_Normal, int alpha = 0xFF);
void draw_sprite_slot_support_alpha(Common::Bitmap *ds, int xpos, int ypos, int src_slot,
                                    Common::BlendMode blend_mode = Common::kBlend_Normal, int alpha = 0xFF);
// CLNUP I'd like to put the default parameters to draw_gui_sprite, but the extern from guiman.h prevents it
void draw_gui_sprite(Common::Bitmap *ds, int pic, int x, int y, Common::BlendMode blend_mode);
void draw_gui_sprite(Common::Bitmap *ds, int xpos, int ypos,
    Common::Bitmap *image, Common::BlendMode blend_mode = Common::kBlend_Normal, int alpha = 0xFF);
void draw_gui_sprite_flipped(Common::Bitmap *ds, int pic, int x, int y, Common::BlendMode blend_mode, AGS::Common::GraphicFlip flip);
void draw_gui_sprite_flipped(Common::Bitmap *ds, int xpos, int ypos,
    Common::Bitmap *image, Common::BlendMode blend_mode = Common::kBlend_Normal, int alpha = 0xFF, AGS::Common::GraphicFlip flip = AGS::Common::kFlip_None);

// Render game on screen
void render_to_screen();
void GfxDriverOnInitCallback(void *data);
bool GfxDriverSpriteEvtCallback(int evt, intptr_t data);

// Create the actsps[objid] image with the object drawn correctly.
// Returns true if nothing at all has changed and actsps is still
// intact from last time; false otherwise.
// Hardware-accelerated do not require altering the raw bitmap itself,
// so they only detect whether the sprite ID itself has changed.
// Software renderers modify the cached bitmap whenever any visual
// effect changes (scaling, tint, etc).
// * force_software option forces HW renderers to construct the image
// in software mode as well.
bool construct_object_gfx(int objid, bool force_software);
bool construct_char_gfx(int charid, bool force_software);
// Returns a cached character image prepared for the render
Common::Bitmap *get_cached_character_image(int charid);
// Returns a cached object image prepared for the render
Common::Bitmap *get_cached_object_image(int objid);
// Adds a walk-behind sprite to the list for the given slot
// (reuses existing texture if possible)
void add_walkbehind_image(size_t index, Common::Bitmap *bmp, int x, int y);
// Clears black game borders in legacy letterbox mode (CLNUP?)
void clear_letterbox_borders();

void draw_and_invalidate_text(Common::Bitmap *ds, int x1, int y1, int font, color_t text_color, const char *text);

void setpal();

// Creates bitmap of a format compatible with the gfxdriver;
// if col_depth is 0, uses game's native color depth.
Common::Bitmap *CreateCompatBitmap(int width, int height, int col_depth = 0);
// Peforms any kind of conversions over bitmap if they are necessary for it
// to be be used in AGS sprite operations. Returns either old or new bitmap.
// Original bitmap **gets deleted** if a new bitmap had to be created.
// * conv_to_gamedepth - tells whether the sprite has to be matching game's
//   default color depth; otherwise its color depth is to be kept (if possible).
// * make_opaque - for sprites with alpha channel (ARGB) tells to make their
//   alpha fully opaque, if that's necessary for the sprite's use.
// * keep_mask - tells whether to keep mask pixels when converting from another
//   color depth. May be useful to disable mask when the source is a 8-bit
//   palette-based image and the opaque sprite is intended.
// TODO: think if it's logical to merge 'make_opaque' and 'keep_mask'.
Common::Bitmap *PrepareSpriteForUse(Common::Bitmap *bitmap, bool conv_to_gamedepth, bool make_opaque = false, bool keep_mask = true);
// Same as above, but compatible with std::shared_ptr.
Common::PBitmap PrepareSpriteForUse(Common::PBitmap bitmap, bool conv_to_gamedepth, bool make_opaque = false, bool keep_mask = true);
// Makes a screenshot corresponding to the last screen render and returns it as a bitmap
// of the requested width and height and game's native color depth.
Common::Bitmap *CopyScreenIntoBitmap(int width, int height, const Rect *src_rect = nullptr,
    bool at_native_res = false, uint32_t batch_skip_filter = 0u);


// TODO: hide these behind some kind of an interface
extern std::unique_ptr<Common::Bitmap> dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];

#endif // __AGS_EE_AC__DRAW_H
