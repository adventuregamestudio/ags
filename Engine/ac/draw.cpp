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
#include <stdio.h>
#include <algorithm>
#include <cmath>
#include "aastr.h"
#include "core/platform.h"
#include "ac/common.h"
#include "util/compress.h"
#include "ac/view.h"
#include "ac/characterextras.h"
#include "ac/characterinfo.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/draw_software.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_region.h"
#include "ac/gui.h"
#include "ac/mouse.h"
#include "ac/movelist.h"
#include "ac/overlay.h"
#include "ac/sys_events.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/runtime_defines.h"
#include "ac/screenoverlay.h"
#include "ac/sprite.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/viewframe.h"
#include "ac/walkablearea.h"
#include "ac/walkbehind.h"
#include "ac/dynobj/scriptsystem.h"
#include "debug/debugger.h"
#include "debug/debug_log.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "gui/guiobject.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/plugin_engine.h"
#include "ac/spritecache.h"
#include "gfx/gfx_util.h"
#include "gfx/graphicsdriver.h"
#include "gfx/ali3dexception.h"
#include "gfx/blender.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "util/wgt2allg.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern ScriptSystem scsystem;
extern AGSPlatformDriver *platform;
extern RoomStruct thisroom;
extern unsigned int loopcounter;
extern SpriteCache spriteset;
extern RoomStatus*croom;
extern EnterNewRoomState in_new_room;
extern bool in_room_transition;
extern RoomObject*objs;
extern std::vector<ViewStruct> views;
extern int displayed_room;
extern CharacterInfo*playerchar;
extern int eip_guinum;
extern int cur_mode,cur_cursor;
extern int mouse_hotx, mouse_hoty;
extern int bg_just_changed;


// TODO: refactor the draw unit into a virtual interface with
// two implementations: for software and video-texture render,
// instead of checking whether the current method is "software".
struct DrawState
{
    // Whether we should use software rendering methods
    // (aka raw draw), as opposed to video texture transform & fx
    bool SoftwareRender = false;
    // Whether we should redraw whole game screen each frame
    bool FullFrameRedraw = false;
    // Walk-behinds representation
    WalkBehindMethodEnum WalkBehindMethod = DrawAsSeparateSprite;

    // The base of DrawIndex range that may be allocated to dynamically
    // created objects; set after initing static game objects.
    uint32_t FixedDrawIndexBase = 0u;
    // Next allocated DrawIndex index
    uint32_t NextDrawIndex = 0u;

    // A map of shared "control blocks" per each sprite used
    // when preparing object textures. "Control block" is currently just
    // an integer which lets to check whether the object texture is in sync
    // with the sprite. When the dynamic sprite is updated or deleted,
    // the control block is marked as invalid and removed from the map;
    // but certain objects may keep the shared ptr to the old block with
    // "invalid" mark, thus they know that they must reset their texture.
    //
    // TODO: investigate an alternative of having a equivalent of
    // "shared texture" with sprite ID ref in Software renderer too,
    // which would allow to use same method of testing DDB ID for both
    // kinds of renderers, thus saving on 1 extra notification mechanism.
    std::unordered_map<sprkey_t, std::shared_ptr<uint32_t>>
        SpriteNotifyMap;
};

DrawState drawstate;
RGB palette[256];
COLOR_MAP maincoltable;

IGraphicsDriver *gfxDriver = nullptr;
IDriverDependantBitmap *blankImage = nullptr;
IDriverDependantBitmap *blankSidebarImage = nullptr;

// ObjTexture is a helper struct that pairs a raw bitmap with
// a renderer's texture and an optional position
struct ObjTexture
{
    // Sprite ID
    uint32_t SpriteID = UINT32_MAX;
    // DrawIndex is a persistent arbitrary number allocated for the drawable object,
    // that is used to resolve z-sorting in case of equal z;
    // this is meant to keep object's sorting order fixed, and avoid random sorting
    // swaps when the list of objects changes.
    uint32_t DrawIndex = 0u;
    // Raw bitmap; used for software render mode,
    // or when particular object types require generated image.
    std::unique_ptr<Bitmap> Bmp;
    // Corresponding texture, created by renderer
    IDriverDependantBitmap *Ddb = nullptr;
    // Sprite notification block: becomes invalid to notify an updated
    // or deleted sprite
    std::shared_ptr<uint32_t> SpriteNotify;
    // Sprite's position
    Point Pos;
    // Texture's offset, *relative* to the logical sprite's position;
    // may be used in case the texture's size is different for any reason
    Point Off;

    ObjTexture() = default;
    ObjTexture(uint32_t sprite_id, Bitmap *bmp, IDriverDependantBitmap *ddb, int x, int y, int xoff = 0, int yoff = 0)
        : SpriteID(sprite_id), Bmp(bmp), Ddb(ddb), Pos(x, y), Off(xoff, yoff) {}
    ObjTexture(ObjTexture &&o) { *this = std::move(o); }
    ~ObjTexture()
    {
        Bmp.reset();
        if (Ddb)
        {
            assert(gfxDriver);
            gfxDriver->DestroyDDB(Ddb);
        }
    }

    // Tests if the sprite change was notified
    inline bool IsChangeNotified() const
    {
        return SpriteNotify && (*SpriteNotify != SpriteID);
    }

    ObjTexture &operator =(ObjTexture &&o)
    {
        SpriteID = o.SpriteID;
        DrawIndex = o.DrawIndex;
        if (Ddb)
        {
            assert(gfxDriver);
            gfxDriver->DestroyDDB(Ddb);
        }
        Bmp = std::move(o.Bmp);
        Ddb = o.Ddb;
        o.Ddb = nullptr;
        Pos = o.Pos;
        Off = o.Off;
        return *this;
    }
};

// ObjectCache stores cached object data, used to determine
// if active sprite / texture should be reconstructed
struct ObjectCache
{
#if (AGS_PLATFORM_DEBUG)
    String tag;
#endif

    std::unique_ptr<Bitmap> image;
    bool  in_use = false; // CHECKME: possibly may be removed
    int   sppic = 0;
    // TODO: pickout tint settings, maybe even share with Char/Obj structs,
    // if plugin API permits!
    short tintr = 0, tintg = 0, tintb = 0, tintamnt = 0, tintlight = 0;
    short lightlev = 0, zoom = 0;
    bool  mirrored = 0;
    int   x = 0, y = 0;

    ObjectCache() = default;
    ObjectCache(int pic_, int tintr_, int tintg_, int tintb_, int tint_amnt_, int tint_light_,
                int light_, int zoom_, bool mirror_, int posx_, int posy_)
        : sppic(pic_), tintr(tintr_), tintg(tintg_), tintb(tintb_)
        , tintamnt(tint_amnt_), tintlight(tint_light_), lightlev(light_)
        , zoom(zoom_), mirrored(mirror_), x(posx_), y(posy_) { }
};

//
// TextureCache class stores textures created by the GraphicsDriver from plain bitmaps.
// Consists of two parts:
// * A long-term MRU cache, which keeps texture data even when it's not in immediate use,
//   and disposes less used textures to free space when reaching the configured mem limit.
// * A short-term cache of texture references, which keeps only weak refs to the textures
//   that are currently in use. This short-term cache lets to keep reusing same texture
//   so long as there's at least one object on screen that uses it.
// NOTE: because of this two-component structure, TextureCache has to override
// number of ResourceCache's parent methods. This design may probably be improved.
class TextureCache :
    public ResourceCache<uint32_t, std::shared_ptr<Texture>>
{
public:
    TextureCache(SpriteCache &spriteset)
        : _spriteset(spriteset) {}

    // Gets existing texture from either MRU cache, or short-term cache
    const std::shared_ptr<Texture> Get(const uint32_t &sprite_id)
    {
        assert(sprite_id != UINT32_MAX); // only valid sprite IDs may be stored
        if (sprite_id == UINT32_MAX)
            return nullptr;

        // Begin getting texture data, first check the MRU cache
        auto txdata = ResourceCache::Get(sprite_id);
        if (txdata)
            return txdata;

        // If not found in MRU cache, try the short-term cache, which
        // may still hold it so long as there are active textures on screen
        const auto found = _txRefs.find(sprite_id);
        if (found != _txRefs.end())
        {
            txdata = found->second.lock();
            // If found, then cache the texture again, and return
            if (txdata)
            {
                Put(sprite_id, txdata);
                return txdata;
            }
        }
        return nullptr;
    }

    // Gets existing texture, or load a sprite and create texture from it;
    // optionally, if "source" bitmap is provided, then use it
    std::shared_ptr<Texture> GetOrLoad(uint32_t sprite_id, Bitmap *source, bool has_alpha, bool opaque)
    {
        assert(sprite_id != UINT32_MAX); // only valid sprite IDs may be stored
        if (sprite_id == UINT32_MAX)
            return nullptr;

        // Try getting existing texture first
        auto txdata = Get(sprite_id);
        if (txdata)
            return txdata;

        // If not in any cache, then try loading the sprite's bitmap,
        // and create a texture data from it
        Bitmap *bitmap = source;
        std::unique_ptr<Bitmap> tmp_source;
        if (!source)
        {
            // Following is a logic of not keeping raw sprite in the cache,
            // and thus potentially saving much RAM.
            // - if texture cache's capacity is > 3/4 of raw sprite cache,
            //   then there's little to no practical reason to keep a raw image.
            // This may be adjusted, or added more rules, as seems necessary.
            bool skip_rawcache =
                GetMaxCacheSize() > (3 * (_spriteset.GetMaxCacheSize() / 4));

            if (_spriteset.IsSpriteLoaded(sprite_id) || !skip_rawcache)
            { // if it's already there, or we are not allowed to skip, then cache normally
                bitmap = _spriteset[sprite_id];
            }
            else
            { // if skipping, ask it to only load, but not keep in raw cache
                tmp_source = _spriteset.LoadSpriteNoCache(sprite_id);
                bitmap = tmp_source.get();
            }
            if (!bitmap)
                return nullptr;
        }

        txdata.reset(gfxDriver->CreateTexture(bitmap, has_alpha, opaque));
        if (!txdata)
            return nullptr;

        txdata->ID = sprite_id;
        _txRefs[sprite_id] = txdata;
        Put(sprite_id, txdata);
        return txdata;
    }

    // Deletes the cached item
    void Dispose(const uint32_t &sprite_id)
    {
        assert(sprite_id != UINT32_MAX); // only valid sprite IDs may be stored
        // Reset sprite ID for any remaining shared txdata
        DetachSharedTexture(sprite_id);
        ResourceCache::Dispose(sprite_id);
    }

    // Removes the item from the cache and returns to the caller.
    std::shared_ptr<Texture> Remove(const uint32_t &sprite_id)
    {
        assert(sprite_id != UINT32_MAX); // only valid sprite IDs may be stored
        // Reset sprite ID for any remaining shared txdata
        DetachSharedTexture(sprite_id);
        return ResourceCache::Remove(sprite_id);
    }

private:
    size_t CalcSize(const std::shared_ptr<Texture> &item) override
    {
        assert(item);
        return item ? item->GetMemSize() : 0u;
    }

    // Marks a shared texture with the invalid sprite ID,
    // this logically disconnects this texture from the cache,
    // and the game objects will be forced to recreate it on the next update
    void DetachSharedTexture(uint32_t sprite_id)
    {
        const auto found = _txRefs.find(sprite_id);
        if (found != _txRefs.end())
        {
            auto txdata = found->second.lock();
            if (txdata)
                txdata->ID = UINT32_MAX;
            _txRefs.erase(sprite_id);
        }
    }

    // A reference to the raw sprites cache, which is also used to load
    // sprites from the asset file.
    SpriteCache &_spriteset;
    // Texture short-term cache:
    // - caches textures while they are in the immediate use;
    // - this lets to share same texture data among multiple sprites on screen.
    typedef std::weak_ptr<Texture> TexDataRef;
    std::unordered_map<uint32_t, TexDataRef> _txRefs;
} texturecache(spriteset);

// actsps is used for temporary storage of the bitmap and texture
// of the latest version of the sprite (room objects and characters);
// objects sprites begin with index 0, characters are after ACTSP_OBJSOFF
std::vector<ObjTexture> actsps;
// Walk-behind textures (3D renderers only)
std::vector<ObjTexture> walkbehindobj;
// GUI surfaces
std::vector<ObjTexture> guibg;
// GUI render targets, for rendering all controls on same texture buffer
std::vector<IDriverDependantBitmap*> gui_render_tex;
// GUI control surfaces
std::vector<ObjTexture> guiobjbg;
// first control texture index of each GUI
std::vector<int> guiobjddbref;
// Overlays textures
std::vector<ObjTexture> overtxs;
// For debugging room masks
RoomAreaMask debugRoomMask = kRoomAreaNone;
ObjTexture debugRoomMaskObj;
int debugMoveListChar = -1;
ObjTexture debugMoveListObj;
// Mouse cursor texture
ObjTexture cursor_tx;

// Draw cache: keep record of all kinds of things related to the previous drawing state
//
// Cached character and object states, used to determine
// whether these require texture update
std::vector<ObjectCache> charcache;
ObjectCache objcache[MAX_ROOM_OBJECTS];
std::vector<Point> overcache;

// Room background sprite
IDriverDependantBitmap* roomBackgroundBmp = nullptr;
// Whether room bg was modified
bool current_background_is_dirty = false;


// Buffer and info flags for viewport/camera pairs rendering in software mode
struct RoomCameraDrawData
{
    // Intermediate bitmap for the software drawing method.
    // We use this bitmap in case room camera has scaling enabled, we draw dirty room rects on it,
    // and then pass to software renderer which draws sprite on top and then either blits or stretch-blits
    // to the virtual screen.
    // For more details see comment in ALSoftwareGraphicsDriver::RenderToBackBuffer().
    PBitmap Buffer;      // this is the actual bitmap
    PBitmap Frame;       // this is either same bitmap reference or sub-bitmap of virtual screen
    bool    IsOffscreen; // whether room viewport was offscreen (cannot use sub-bitmap)
    bool    IsOverlap;   // whether room viewport overlaps any others (marking dirty rects is complicated)
};
std::vector<RoomCameraDrawData> CameraDrawData;


// Describes a texture or node description, for sorting and passing into renderer
struct SpriteListEntry
{
    IDriverDependantBitmap *DDB = nullptr;
    int X = 0;
    int Y = 0;
    int Z = 0;
    // Mark for the render stage callback (if >= 0 other fields are ignored)
    int RenderStage = -1;
    // Optional sprite identifier; used as a second factor when sorting
    uint32_t DrawIndex = 0u;

    SpriteListEntry(IDriverDependantBitmap *ddb, int x, int y)
        : DDB(ddb), X(x), Y(y), Z(0), RenderStage(-1), DrawIndex(0u) { }
    SpriteListEntry(IDriverDependantBitmap *ddb, int x, int y, int z, int render_stage, uint32_t draw_index = 0u)
        : DDB(ddb), X(x), Y(y), Z(z), RenderStage(render_stage), DrawIndex(draw_index) { }
};

// Two lists of sprites to push into renderer during next render pass
// thingsToDrawList - is the main list, unsorted, drawn in the index order
std::vector<SpriteListEntry> thingsToDrawList;
// sprlist - will be sorted using baseline and appended to main list
std::vector<SpriteListEntry> sprlist;

// For raw drawing
std::unique_ptr<Bitmap> raw_saved_screen;
std::unique_ptr<Bitmap> dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];


void setpal() {
    set_palette_range(palette, 0, 255, 0);
}

Bitmap *CreateCompatBitmap(int width, int height, int col_depth)
{
    return new Bitmap(width, height,
        gfxDriver->GetCompatibleBitmapFormat(col_depth == 0 ? game.GetColorDepth() : col_depth));
}

// PrepareSpriteForUseImpl converts input bitmap to format which may be used
// in AGS sprite operations, including raw drawing operations.
// In addition, in rare cases, it may require a conversion to a format
// compatible with the graphics driver (which may be converted to a texture).
// * conv_to_gamedepth - tells whether the sprite has to be matching game's
//   default color depth; otherwise its color depth is to be kept (if possible).
// * has_alpha - for sprites with alpha channel (ARGB) tells whether their
//   alpha channel should be kept, otherwise it's filled with opaqueness.
// * keep_mask - tells whether to keep mask pixels when converting from another
//   color depth. May be useful to disable mask when the source is a 8-bit
//   palette-based image and the opaque sprite is intended.
Bitmap *PrepareSpriteForUseImpl(Bitmap* bitmap, bool has_alpha, bool keep_mask, const RGB *bm_palette)
{
    // sprite must be converted to game's color depth;
    // this behavior is hardcoded in the current engine version
    const bool conv_to_gamedepth = true;
    const int bmp_col_depth = bitmap->GetColorDepth();
    const int game_col_depth = game.GetColorDepth();

    // Palette must be selected if we convert a 8-bit bitmap for a 32-bit game
    const bool must_switch_palette = conv_to_gamedepth && (bitmap->GetColorDepth() == 8) && (game_col_depth > 8);
    if (must_switch_palette)
        select_palette(bm_palette != nullptr ? bm_palette : palette);

    Bitmap *new_bitmap = bitmap;

    // If it was requested to convert bitmap to the game's default color depth,
    // then do so if bitmap is not matching the game.
    bool was_conv_to_gamedepth = false;
    if (conv_to_gamedepth && (bmp_col_depth != game_col_depth))
    {
        // Prior to downgrading a 32-bit sprite with valid alpha channel,
        // replace its alpha channel to a regular transparency mask.
        if ((bmp_col_depth == 32) && has_alpha)
        {
            BitmapHelper::ReplaceHalfAlphaWithRGBMask(bitmap);
        }

        new_bitmap = GfxUtil::ConvertBitmap(bitmap, gfxDriver->GetCompatibleBitmapFormat(game_col_depth), keep_mask);
        was_conv_to_gamedepth = true;
    }

    // Handle alpha channel values for 32-bit bitmaps in 32-bit games.
    // * If alpha channel is requested, then mark fully transparent pixels as
    // MASK_COLOR. This adjustment is currently required for DrawingSurface
    // operations in script.
    // * Else this is either a 32-bit sprite with ignored alpha channel or
    // it was converted from another color depth, then make a fully-opaque
    // alpha channel, except for the existing MASK_COLOR pixels.
    if ((game_col_depth == 32) && (new_bitmap->GetColorDepth() == 32))
    {
        if (has_alpha)
            BitmapHelper::ReplaceZeroAlphaWithRGBMask(new_bitmap);
        else if (keep_mask)
            BitmapHelper::MakeOpaqueSkipMask(new_bitmap);
        else
            BitmapHelper::MakeOpaque(new_bitmap);
    }
    
    // Finally, if we did not create a new copy already, - ensure gfxdriver compatible format
    if (new_bitmap == bitmap)
        new_bitmap = GfxUtil::ConvertBitmap(bitmap, gfxDriver->GetCompatibleBitmapFormat(bitmap->GetColorDepth()), keep_mask);

    if (must_switch_palette)
        unselect_palette();

    return new_bitmap;
}

Bitmap *PrepareSpriteForUse(Bitmap* bitmap, bool has_alpha, bool keep_mask, const RGB *bm_palette)
{
    Bitmap *new_bitmap = PrepareSpriteForUseImpl(bitmap, has_alpha, keep_mask, bm_palette);
    if (new_bitmap != bitmap)
        delete bitmap;
    return new_bitmap;
}

PBitmap PrepareSpriteForUse(PBitmap bitmap, bool has_alpha, bool keep_mask, const RGB *bm_palette)
{
    Bitmap *new_bitmap = PrepareSpriteForUseImpl(bitmap.get(), has_alpha, keep_mask, bm_palette);
    return new_bitmap == bitmap.get() ? bitmap : PBitmap(new_bitmap); // if bitmap is same, don't create new smart ptr!
}

Bitmap *CopyScreenIntoBitmap(int width, int height, const Rect *src_rect,
    bool at_native_res, uint32_t batch_skip_filter)
{
    Bitmap *dst = new Bitmap(width, height, game.GetColorDepth());
    GraphicResolution want_fmt;
    // If the size and color depth are supported, then we may copy right into our final bitmap
    if (gfxDriver->GetCopyOfScreenIntoBitmap(dst, src_rect, at_native_res, &want_fmt, batch_skip_filter))
        return dst;

    // Otherwise we might need to copy between few bitmaps...
    // Get screenshot in the suitable format
    std::unique_ptr<Bitmap> buf_screenfmt(new Bitmap(want_fmt.Width, want_fmt.Height, want_fmt.ColorDepth));
    gfxDriver->GetCopyOfScreenIntoBitmap(buf_screenfmt.get(), src_rect, at_native_res, nullptr, batch_skip_filter);
    // If color depth does not match, and we must stretch-blit, then we need another helper bmp,
    // because Allegro does not support stretching with mismatching color depths
    std::unique_ptr<Bitmap> buf_fixdepth;
    Bitmap *blit_from = buf_screenfmt.get();
    if ((dst->GetSize() != blit_from->GetSize())
        && (want_fmt.ColorDepth != game.GetColorDepth()))
    {
        buf_fixdepth.reset(new Bitmap(want_fmt.Width, want_fmt.Height, game.GetColorDepth()));
        buf_fixdepth->Blit(buf_screenfmt.get());
        blit_from = buf_fixdepth.get();
    }

    // Now either blit or stretch-blit
    if (dst->GetSize() == blit_from->GetSize())
    {
        dst->Blit(blit_from);
    }
    else
    {
        dst->StretchBlt(blit_from, RectWH(dst->GetSize()));
    }
    return dst;
}


// Begin resolution system functions

// Multiplies up the number of pixels depending on the current 
// resolution, to give a relatively fixed size at any game res
int get_fixed_pixel_size(int pixels)
{
    return pixels * game.GetRelativeUIMult();
}

int data_to_game_coord(int coord)
{
    return coord * game.GetDataUpscaleMult();
}

void data_to_game_coords(int *x, int *y)
{
    const int mul = game.GetDataUpscaleMult();
    x[0] *= mul;
    y[0] *= mul;
}

void data_to_game_round_up(int *x, int *y)
{
    const int mul = game.GetDataUpscaleMult();
    x[0] = x[0] * mul + (mul - 1);
    y[0] = y[0] * mul + (mul - 1);
}

int game_to_data_coord(int coord)
{
    return coord / game.GetDataUpscaleMult();
}

void game_to_data_coords(int &x, int &y)
{
    const int mul = game.GetDataUpscaleMult();
    x /= mul;
    y /= mul;
}

int game_to_data_round_up(int coord)
{
    const int mul = game.GetDataUpscaleMult();
    return (coord / mul) + (mul - 1);
}

void ctx_data_to_game_coord(int &x, int &y, bool hires_ctx)
{
    if (hires_ctx && !game.IsLegacyHiRes())
    {
        x /= HIRES_COORD_MULTIPLIER;
        y /= HIRES_COORD_MULTIPLIER;
    }
    else if (!hires_ctx && game.IsLegacyHiRes())
    {
        x *= HIRES_COORD_MULTIPLIER;
        y *= HIRES_COORD_MULTIPLIER;
    }
}

void ctx_data_to_game_size(int &w, int &h, bool hires_ctx)
{
    if (hires_ctx && !game.IsLegacyHiRes())
    {
        w = std::max(1, (w / HIRES_COORD_MULTIPLIER));
        h = std::max(1, (h / HIRES_COORD_MULTIPLIER));
    }
    else if (!hires_ctx && game.IsLegacyHiRes())
    {
        w *= HIRES_COORD_MULTIPLIER;
        h *= HIRES_COORD_MULTIPLIER;
    }
}

int ctx_data_to_game_size(int size, bool hires_ctx)
{
    if (hires_ctx && !game.IsLegacyHiRes())
        return std::max(1, (size / HIRES_COORD_MULTIPLIER));
    if (!hires_ctx && game.IsLegacyHiRes())
        return size * HIRES_COORD_MULTIPLIER;
    return size;
}

int game_to_ctx_data_size(int size, bool hires_ctx)
{
    if (hires_ctx && !game.IsLegacyHiRes())
        return size * HIRES_COORD_MULTIPLIER;
    else if (!hires_ctx && game.IsLegacyHiRes())
        return std::max(1, (size / HIRES_COORD_MULTIPLIER));
    return size;
}

void defgame_to_finalgame_coords(int &x, int &y)
{ // Note we support only upscale now
    x *= game.GetScreenUpscaleMult();
    y *= game.GetScreenUpscaleMult();
}

// End resolution system functions

// Create blank (black) images used to repaint borders around game frame
void create_blank_image(int coldepth)
{
    // this is the first time that we try to use the graphics driver,
    // so it's the most likey place for a crash
    try
    {
        Bitmap *blank = CreateCompatBitmap(16, 16, coldepth);
        blank->Clear();
        blankImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
        blankSidebarImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
        delete blank;
    }
    catch (Ali3DException& gfxException)
    {
        quit(gfxException.Message.GetCStr());
    }
}

void destroy_blank_image()
{
    if (blankImage)
        gfxDriver->DestroyDDB(blankImage);
    if (blankSidebarImage)
        gfxDriver->DestroyDDB(blankSidebarImage);
    blankImage = nullptr;
    blankSidebarImage = nullptr;
}

int MakeColor(int color_index)
{
    return my_setcolor(color_index, game.GetColorDepth());
}

void init_draw_method()
{
    drawstate.SoftwareRender = !gfxDriver->HasAcceleratedTransform();
    drawstate.FullFrameRedraw = gfxDriver->RequiresFullRedrawEachFrame();

    // Must init this as early as possible, as this affects bitmap->texture conv
    gfxDriver->UseSmoothScaling(play.ShouldAASprites());

    if (drawstate.SoftwareRender)
    {
        drawstate.WalkBehindMethod = DrawOverCharSprite;
    }
    else
    {
        drawstate.WalkBehindMethod = DrawAsSeparateSprite;
        create_blank_image(game.GetColorDepth());
        size_t tx_cache_size = usetup.TextureCacheSize * 1024;
        // If graphics driver can report available texture memory,
        // then limit the setting by, let's say, 66% of it (we use it for other things)
        uint64_t avail_tx_mem = gfxDriver->GetAvailableTextureMemory();
        if (avail_tx_mem > 0)
            tx_cache_size = std::min<size_t>(SIZE_MAX, std::min<uint64_t>(tx_cache_size, avail_tx_mem * 0.66));
        texturecache.SetMaxCacheSize(tx_cache_size);
        Debug::Printf("Texture cache set: %zu KB", tx_cache_size / 1024);
    }

    on_mainviewport_changed();
    init_room_drawdata();
    if (gfxDriver->UsesMemoryBackBuffer())
        gfxDriver->GetMemoryBackBuffer()->Clear();
}

void dispose_draw_method()
{
    dispose_room_drawdata();
    dispose_invalid_regions(false);
    destroy_blank_image();
}

static void alloc_fixed_drawindexes()
{
    size_t draw_index = 1u; // DrawIndex 0 is reserved for walk-behinds
    for (auto &actsp : actsps)
        actsp.DrawIndex = draw_index++;
    for (auto &guidraw : guibg)
        guidraw.DrawIndex = draw_index++;
    drawstate.FixedDrawIndexBase = draw_index;
    drawstate.NextDrawIndex = draw_index;
}

void init_game_drawdata()
{
    // character and object caches
    charcache.resize(game.numcharacters);
    for (size_t i = 0; i < (size_t)MAX_ROOM_OBJECTS; ++i)
        objcache[i] = ObjectCache();
#if (AGS_PLATFORM_DEBUG)
    for (auto &ch : game.chars)
        charcache[ch.index_id].tag.Format("CHAR%d:%s", ch.index_id, ch.scrname);
#endif

    size_t actsps_num = game.numcharacters + MAX_ROOM_OBJECTS;
    actsps.resize(actsps_num);

    guibg.resize(game.numgui);
    gui_render_tex.resize(game.numgui);
    size_t guio_num = 0;
    // Prepare GUI cache lists and build the quick reference for controls cache
    guiobjddbref.resize(game.numgui);
    for (const auto &gui : guis)
    {
        guiobjddbref[gui.ID] = guio_num;
        guio_num += gui.GetControlCount();
    }
    guiobjbg.resize(guio_num);
    alloc_fixed_drawindexes();
}

extern void dispose_engine_overlay();

void dispose_game_drawdata()
{
    clear_drawobj_cache();

    charcache.clear();
    actsps.clear();
    walkbehindobj.clear();

    texturecache_clear();
    guibg.clear();
    gui_render_tex.clear();
    guiobjbg.clear();
    guiobjddbref.clear();

    dispose_engine_overlay();
}

static void dispose_debug_room_drawdata()
{
    debugRoomMaskObj = ObjTexture();
    debugMoveListObj = ObjTexture();
}

void dispose_room_drawdata()
{
    CameraDrawData.clear();
    dispose_invalid_regions(true);
}

void clear_drawobj_cache()
{
    // clear the character cache
    for (auto &cc : charcache)
    {
        cc = ObjectCache();
    }
    // clear the object cache
    for (int i = 0; i < MAX_ROOM_OBJECTS; ++i)
    {
        objcache[i] = ObjectCache();
    }
    // room overlays cache
    overcache.clear();

    // cleanup Character + Room object textures
    for (auto &o : actsps) o = ObjTexture();
    for (auto &o : walkbehindobj) o = ObjTexture();
    // cleanup GUI and controls textures
    for (auto &o : guibg) o = ObjTexture();
    for (auto &tex : gui_render_tex)
    {
        if (tex)
            gfxDriver->DestroyDDB(tex);
        tex = nullptr;
    }
    for (auto &o : guiobjbg) o = ObjTexture();
    overtxs.clear();
    // Mouse cursor texture
    cursor_tx = ObjTexture();

    // Clear sprite update notification blocks
    drawstate.SpriteNotifyMap.clear();

    dispose_debug_room_drawdata();
}

void release_drawobj_rendertargets()
{
    if ((gui_render_tex.size() == 0) ||
        !gfxDriver->ShouldReleaseRenderTargets())
        return;

    gfxDriver->ClearDrawLists(); // force clear to ensure nothing stays cached
    for (auto &tex : gui_render_tex)
    {
        if (tex)
            gfxDriver->DestroyDDB(tex);
        tex = nullptr;
    }
}

void on_mainviewport_changed()
{
    if (!drawstate.FullFrameRedraw)
    {
        const auto &view = play.GetMainViewport();
        set_invalidrects_globaloffs(view.Left, view.Top);
        // the black background region covers whole game screen
        init_invalid_regions(-1, game.GetGameRes(), RectWH(game.GetGameRes()));
        if (game.GetGameRes().ExceedsByAny(view.GetSize()))
            clear_letterbox_borders();
    }
}

// Allocates a bitmap for rendering camera/viewport pair (software render mode)
void prepare_roomview_frame(Viewport *view)
{
    if (!view->GetCamera()) return; // no camera link
    const int view_index = view->GetID();
    const Size view_sz = view->GetRect().GetSize();
    const Size cam_sz = view->GetCamera()->GetRect().GetSize();
    RoomCameraDrawData &draw_dat = CameraDrawData[view_index];
    // We use intermediate bitmap to render camera/viewport pair in software mode under these conditions:
    // * camera size and viewport size are different (this may be suboptimal to paint dirty rects stretched,
    //   and also Allegro backend cannot stretch background of different colour depth).
    // * viewport is located outside of the virtual screen (even if partially): subbitmaps cannot contain
    //   regions outside of master bitmap, and we must not clamp surface size to virtual screen because
    //   plugins may want to also use viewport bitmap, therefore it should retain full size.
    if (cam_sz == view_sz && !draw_dat.IsOffscreen)
    { // note we keep the buffer allocated in case it will become useful later
        draw_dat.Frame.reset();
    }
    else
    {
        PBitmap &camera_frame = draw_dat.Frame;
        PBitmap &camera_buffer = draw_dat.Buffer;
        if (!camera_buffer || camera_buffer->GetWidth() < cam_sz.Width || camera_buffer->GetHeight() < cam_sz.Height)
        {
            // Allocate new buffer bitmap with an extra size in case they will want to zoom out
            int room_width = data_to_game_coord(thisroom.Width);
            int room_height = data_to_game_coord(thisroom.Height);
            Size alloc_sz = Size::Clamp(cam_sz * 2, Size(1, 1), Size(room_width, room_height));
            camera_buffer.reset(new Bitmap(alloc_sz.Width, alloc_sz.Height, gfxDriver->GetMemoryBackBuffer()->GetColorDepth()));
        }

        if (!camera_frame || camera_frame->GetSize() != cam_sz)
        {
            camera_frame.reset(BitmapHelper::CreateSubBitmap(camera_buffer.get(), RectWH(cam_sz)));
        }
    }
}

// Syncs room viewport and camera in case either size has changed
void sync_roomview(Viewport *view)
{
    if (view->GetCamera() == nullptr)
        return;
    // Note the dirty regions' viewport is found using absolute offset on game screen
    init_invalid_regions(view->GetID(),
        view->GetCamera()->GetRect().GetSize(),
        play.GetRoomViewportAbs(view->GetID()));
    prepare_roomview_frame(view);
}

void init_room_drawdata()
{
    if (displayed_room < 0)
        return; // not loaded yet

    // Must realloc, because the ObjTextures are cleared on room change
    alloc_fixed_drawindexes();

    if (drawstate.WalkBehindMethod == DrawAsSeparateSprite)
    {
        walkbehinds_generate_sprites();
    }

    // Update debug overlays, if any were on
    debug_draw_room_mask(debugRoomMask);
    debug_draw_movelist(debugMoveListChar);

    // Following data is only updated for software renderer
    if (drawstate.FullFrameRedraw)
        return;
    // Make sure all frame buffers are created for software drawing
    int view_count = play.GetRoomViewportCount();
    CameraDrawData.resize(view_count);
    for (int i = 0; i < play.GetRoomViewportCount(); ++i)
        sync_roomview(play.GetRoomViewport(i).get());

#if (AGS_PLATFORM_DEBUG)
    for (size_t i = 0; i < (size_t)thisroom.Objects.size(); ++i)
        objcache[i].tag.Format("OBJ%d:%s", (int)i, thisroom.Objects[i].ScriptName.GetCStr());
#endif
}

void on_roomviewport_created(int index)
{
    if (drawstate.FullFrameRedraw || (displayed_room < 0))
        return;
    if ((size_t)index < CameraDrawData.size())
        return;
    CameraDrawData.resize(index + 1);
}

void on_roomviewport_deleted(int index)
{
    if (drawstate.FullFrameRedraw || (displayed_room < 0))
        return;
    CameraDrawData.erase(CameraDrawData.begin() + index);
    delete_invalid_regions(index);
}

void on_roomviewport_changed(Viewport *view)
{
    if (drawstate.FullFrameRedraw || (displayed_room < 0))
        return;
    if (!view->IsVisible() || view->GetCamera() == nullptr)
        return;
    const bool off = !IsRectInsideRect(RectWH(gfxDriver->GetMemoryBackBuffer()->GetSize()), view->GetRect());
    const bool off_changed = off != CameraDrawData[view->GetID()].IsOffscreen;
    CameraDrawData[view->GetID()].IsOffscreen = off;
    if (view->HasChangedSize())
        sync_roomview(view);
    else if (off_changed)
        prepare_roomview_frame(view);
    // TODO: don't have to do this all the time, perhaps do "dirty rect" method
    // and only clear previous viewport location?
    invalidate_screen();
    gfxDriver->GetMemoryBackBuffer()->Clear();
}

void detect_roomviewport_overlaps(size_t z_index)
{
    if (drawstate.FullFrameRedraw || (displayed_room < 0))
        return;
    // Find out if we overlap or are overlapped by anything;
    const auto &viewports = play.GetRoomViewportsZOrdered();
    for (; z_index < viewports.size(); ++z_index)
    {
        auto this_view = viewports[z_index];
        const int this_id = this_view->GetID();
        bool is_overlap = false;
        if (!this_view->IsVisible()) continue;
        for (size_t z_index2 = 0; z_index2 < z_index; ++z_index2)
        {
            if (!viewports[z_index2]->IsVisible()) continue;
            if (AreRectsIntersecting(this_view->GetRect(), viewports[z_index2]->GetRect()))
            {
                is_overlap = true;
                break;
            }
        }
        if (CameraDrawData[this_id].IsOverlap != is_overlap)
        {
            CameraDrawData[this_id].IsOverlap = is_overlap;
            prepare_roomview_frame(this_view.get());
        }
    }
}

void on_roomcamera_changed(Camera *cam)
{
    if (drawstate.FullFrameRedraw || (displayed_room < 0))
        return;
    if (cam->HasChangedSize())
    {
        auto viewrefs = cam->GetLinkedViewports();
        for (auto vr : viewrefs)
        {
            PViewport vp = vr.lock();
            if (vp)
                sync_roomview(vp.get());
        }
    }
    // TODO: only invalidate what this particular camera sees
    invalidate_screen();
}

void mark_object_changed(int objid)
{
    objcache[objid].y = -9999;
}

void reset_drawobj_dynamic_index()
{
    drawstate.NextDrawIndex = drawstate.FixedDrawIndexBase;
}

void add_drawobj_for_overlay(int objnum)
{
    if (objnum > 0)
    {
        if (static_cast<uint32_t>(objnum) >= overtxs.size())
            overtxs.resize(static_cast<uint32_t>(objnum) + 1);

        overtxs[objnum].DrawIndex = drawstate.NextDrawIndex;
        drawstate.NextDrawIndex = (drawstate.NextDrawIndex == UINT32_MAX) ? drawstate.FixedDrawIndexBase : drawstate.NextDrawIndex + 1;
    }
}

void reset_drawobj_for_overlay(int objnum)
{
    if (objnum > 0 && static_cast<size_t>(objnum) < overtxs.size())
    {
        overtxs[objnum] = ObjTexture();
        if (drawstate.SoftwareRender)
            overcache[objnum] = Point(INT32_MIN, INT32_MIN);
    }
}

void notify_sprite_changed(int sprnum, bool deleted)
{
    assert(sprnum >= 0 && static_cast<uint32_t>(sprnum) < game.SpriteInfos.size());
    // Update texture cache (regen texture or clear from cache)
    if (deleted)
        clear_shared_texture(sprnum);
    else
        update_shared_texture(sprnum);

    // For texture-based renderers updating a shared texture will already
    // update all the related drawn objects on screen; software renderer
    // will need to know to redraw active cached sprite for objects.
    // We have this notification for both kinds of renderers though,
    // because it makes the code simpler, and also it makes it simpler to
    // notify texture-based ones in a specific case when a deleted sprite
    // was replaced by another of same ID.
    {
        auto it_notify = drawstate.SpriteNotifyMap.find(sprnum);
        if (it_notify != drawstate.SpriteNotifyMap.end())
        {
            *it_notify->second = UINT32_MAX;
            drawstate.SpriteNotifyMap.erase(sprnum);
        }
    }
}

void texturecache_get_state(size_t &max_size, size_t &cur_size, size_t &locked_size, size_t &ext_size)
{
    max_size = texturecache.GetMaxCacheSize();
    cur_size = texturecache.GetCacheSize();
    locked_size = texturecache.GetLockedSize();
    ext_size = texturecache.GetExternalSize();
}

size_t texturecache_get_size()
{
    return texturecache.GetCacheSize();
}

void texturecache_clear()
{
    texturecache.Clear();
}

void update_shared_texture(uint32_t sprite_id)
{
    auto txdata = texturecache.Get(sprite_id);
    if (!txdata)
        return;

    const auto &res = txdata->Res;
    if (res.Width == game.SpriteInfos[sprite_id].Width &&
        res.Height == game.SpriteInfos[sprite_id].Height)
    {
        gfxDriver->UpdateTexture(txdata.get(), spriteset[sprite_id],
            (game.SpriteInfos[sprite_id].Flags & SPF_ALPHACHANNEL) != 0, false);
    }
    else
    {
        // Remove texture from cache, assume it will be recreated on demand
        texturecache.Dispose(sprite_id);
    }
}

void clear_shared_texture(uint32_t sprite_id)
{
    texturecache.Dispose(sprite_id);
}

void texturecache_precache(uint32_t sprite_id)
{
    bool has_alpha = (sprite_id < game.SpriteInfos.size()) ?
        ((game.SpriteInfos[sprite_id].Flags & SPF_ALPHACHANNEL) != 0) : false;
    texturecache.GetOrLoad(sprite_id, nullptr, has_alpha, false);
}

void invalidate_screen()
{
    invalidate_all_rects();
}

void invalidate_camera_frame(int index)
{
    invalidate_all_camera_rects(index);
}

void invalidate_rect(int x1, int y1, int x2, int y2, bool in_room)
{
    invalidate_rect_ds(x1, y1, x2, y2, in_room);
}

void invalidate_sprite(int x1, int y1, IDriverDependantBitmap *pic, bool in_room)
{
    invalidate_rect_ds(x1, y1, x1 + pic->GetWidth(), y1 + pic->GetHeight(), in_room);
}

void invalidate_sprite_glob(int x1, int y1, IDriverDependantBitmap *pic)
{
    invalidate_rect_global(x1, y1, x1 + pic->GetWidth(), y1 + pic->GetHeight());
}

void mark_current_background_dirty()
{
    current_background_is_dirty = true;
}


void draw_and_invalidate_text(Bitmap *ds, int x1, int y1, int font, color_t text_color, const char *text)
{
    wouttext_outline(ds, x1, y1, font, text_color, text);
    invalidate_rect(x1, y1, x1 + get_text_width_outlined(text, font),
        y1 + get_font_height_outlined(font) + get_fixed_pixel_size(1), false);
}

// Renders black borders for the legacy boxed game mode,
// where whole game screen changes size between large and small rooms
static void render_black_borders()
{
    const Rect &viewport = play.GetMainViewport();
    if (viewport.Top > 0)
    {
        // letterbox borders
        blankImage->SetStretch(game.GetGameRes().Width, viewport.Top, false);
        gfxDriver->DrawSprite(0, 0, blankImage);
        gfxDriver->DrawSprite(0, viewport.Bottom + 1, blankImage);
    }
    if (viewport.Left > 0)
    {
        // sidebar borders for widescreen
        blankSidebarImage->SetStretch(viewport.Left, viewport.GetHeight(), false);
        gfxDriver->DrawSprite(0, 0, blankSidebarImage);
        gfxDriver->DrawSprite(viewport.Right + 1, 0, blankSidebarImage);
    }
}


extern volatile bool game_update_suspend;
extern volatile bool want_exit, abort_engine;

void render_to_screen()
{
    // Stage: final plugin callback (still drawn on game screen)
    if (pl_any_want_hook(kPluginEvt_FinalScreenDraw))
    {
        gfxDriver->BeginSpriteBatch(play.GetMainViewport(),
            play.GetGlobalTransform(drawstate.FullFrameRedraw), (GraphicFlip)play.screen_flipped);
        gfxDriver->AddRenderEvent(kPluginEvt_FinalScreenDraw, 0);
        gfxDriver->EndSpriteBatch();
    }
    // Stage: engine overlay
    construct_engine_overlay();

    // Try set new vsync value, and remember the actual result
    if (isTimerFpsMaxed())
    {
        gfxDriver->SetVsync(false);
    }
    else
    {
        bool new_vsync = gfxDriver->SetVsync(scsystem.vsync > 0);
        if (new_vsync != (scsystem.vsync != 0))
            System_SetVSyncInternal(new_vsync);
    }

    bool succeeded = false;
    while (!succeeded && !want_exit && !abort_engine)
    {
        try
        {
            if (drawstate.FullFrameRedraw)
            {
                gfxDriver->Render();
            }
            else
            {
                // NOTE: the shake yoff and global flip here will only be used by a software renderer;
                // as hw renderers have these as transform parameters for the parent scene nodes.
                // This may be a matter for the future code improvement.
                //
                // For software renderer, need to blacken upper part of the game frame when shaking screen moves image down
                if (play.shake_screen_yoff > 0)
                {
                    const Rect &viewport = play.GetMainViewport();
                    gfxDriver->ClearRectangle(viewport.Left, viewport.Top, viewport.GetWidth() - 1, play.shake_screen_yoff, nullptr);
                }
                gfxDriver->Render(0, play.shake_screen_yoff, (GraphicFlip)play.screen_flipped);
            }
            succeeded = true;
        }
        catch (Ali3DFullscreenLostException& e) 
        {
            Debug::Printf("Renderer exception: %s", e.Message.GetCStr());
            do
            {
                sys_evt_process_pending();
                platform->Delay(300);
            } while (game_update_suspend && (!want_exit) && (!abort_engine));
        }
    }
}

// Blanks out borders around main viewport in case it became smaller (e.g. after loading another room)
void clear_letterbox_borders()
{
    const Rect &viewport = play.GetMainViewport();
    gfxDriver->ClearRectangle(0, 0, game.GetGameRes().Width - 1, viewport.Top - 1, nullptr);
    gfxDriver->ClearRectangle(0, viewport.Bottom + 1, game.GetGameRes().Width - 1, game.GetGameRes().Height - 1, nullptr);
}

void putpixel_scaled(Bitmap *ds, int x, int y, int col)
{
    ds->FillRect(Rect(x, y, x + get_fixed_pixel_size(1) - 1, y + get_fixed_pixel_size(1) - 1), col);
}

void draw_sprite_support_alpha(Bitmap *ds, bool ds_has_alpha, int xpos, int ypos, Bitmap *image, bool src_has_alpha,
                               BlendMode blend_mode, int alpha)
{
    if (alpha <= 0)
        return;

    if (game.options[OPT_SPRITEALPHA] == kSpriteAlphaRender_Proper)
    {
        GfxUtil::DrawSpriteBlend(ds, Point(xpos, ypos), image, blend_mode, ds_has_alpha, src_has_alpha, alpha);
    }
    // Backwards-compatible drawing
    else if (src_has_alpha && alpha == 0xFF)
    {
        set_alpha_blender();
        ds->TransBlendBlt(image, xpos, ypos);
    }
    else
    {
        GfxUtil::DrawSpriteWithTransparency(ds, image, xpos, ypos, alpha);
    }
}

void draw_sprite_slot_support_alpha(Bitmap *ds, bool ds_has_alpha, int xpos, int ypos, int src_slot,
                                    BlendMode blend_mode, int alpha)
{
    draw_sprite_support_alpha(ds, ds_has_alpha, xpos, ypos, spriteset[src_slot], (game.SpriteInfos[src_slot].Flags & SPF_ALPHACHANNEL) != 0,
        blend_mode, alpha);
}

IDriverDependantBitmap* recycle_ddb_bitmap(IDriverDependantBitmap *ddb,
    Common::Bitmap *source, bool has_alpha, bool opaque)
{
    assert(source);
    if (ddb && // already has an allocated DDB,
        (drawstate.SoftwareRender || // is software renderer, or...
        ((ddb->GetRefID() == UINT32_MAX) && // the texture is *not identified*, and...
            (ddb->MatchesFormat(source))))) // existing DDB format matches
        gfxDriver->UpdateDDBFromBitmap(ddb, source, has_alpha);
    else if (ddb) // if existing texture format does not match, then create a new texture
        ddb->AttachData(std::shared_ptr<Texture>(gfxDriver->CreateTexture(source, has_alpha, opaque)), opaque);
    else // ...else allocated new DDB
        ddb = gfxDriver->CreateDDBFromBitmap(source, has_alpha, opaque);
    return ddb;
}

IDriverDependantBitmap* recycle_ddb_sprite(IDriverDependantBitmap *ddb, uint32_t sprite_id,
    Common::Bitmap *source, bool has_alpha, bool opaque)
{
    // If sprite_id is not cachable, then fallback to a simpler variant
    if (drawstate.SoftwareRender || sprite_id == UINT32_MAX)
    {
        if (!source && (sprite_id < UINT32_MAX))
            source = spriteset[sprite_id];
        return recycle_ddb_bitmap(ddb, source, has_alpha, opaque);
    }

    if (ddb && ddb->IsValid() && ddb->GetRefID() == sprite_id)
        return ddb; // texture in sync

    auto txdata = texturecache.GetOrLoad(sprite_id, source, has_alpha, opaque);
    if (!txdata)
    {
        // On failure - invalidate ddb (we don't want to draw old pixels);
        // or create a dummy one in case it did not exist
        if (ddb)
            ddb->DetachData();
        else
            ddb = gfxDriver->CreateDDB();
        return ddb;
    }

    if (ddb)
        ddb->AttachData(txdata, opaque);
    else
        ddb = gfxDriver->CreateDDB(txdata, opaque);
    return ddb;
}

IDriverDependantBitmap* recycle_render_target(IDriverDependantBitmap *ddb, int width, int height, int col_depth, bool opaque)
{
    if (ddb && (ddb->GetWidth() == width) && (ddb->GetHeight() == height))
        return ddb;
    if (ddb)
        gfxDriver->DestroyDDB(ddb);
    return gfxDriver->CreateRenderTargetDDB(width, height, col_depth, opaque);
}

// FIXME: make has_alpha and opaque properties of ObjTexture?!
static void sync_object_texture(ObjTexture &obj, bool has_alpha = false, bool opaque = false)
{
    obj.Ddb = recycle_ddb_sprite(obj.Ddb, obj.SpriteID, obj.Bmp.get(), has_alpha, opaque);

    // Handle notification control block for the dynamic sprites
    if ((obj.SpriteID != UINT32_MAX) && game.SpriteInfos[obj.SpriteID].IsDynamicSprite())
    {
        // For dynamic sprite: check and update a notification block for this drawable
        if (!obj.SpriteNotify || (*obj.SpriteNotify != obj.SpriteID))
        {
            auto it_notify = drawstate.SpriteNotifyMap.find(obj.SpriteID);
            if (it_notify != drawstate.SpriteNotifyMap.end())
            { // assign existing
                obj.SpriteNotify = it_notify->second;
            }
            else
            { // if does not exist, then create and share one
                obj.SpriteNotify = std::make_shared<uint32_t>(obj.SpriteID);
                drawstate.SpriteNotifyMap.insert(std::make_pair(obj.SpriteID, obj.SpriteNotify));
            }
        }
    }
    else
    {
        obj.SpriteNotify = nullptr; // reset, for static sprite or without ID
    }
}

//------------------------------------------------------------------------
// Functions for filling the lists of sprites to render
static void clear_draw_list()
{
    thingsToDrawList.clear();
}

static void add_thing_to_draw(IDriverDependantBitmap* ddb, int x, int y)
{
    assert(ddb);
    thingsToDrawList.push_back(SpriteListEntry(ddb, x, y));
}

static void add_render_stage(int stage)
{
    thingsToDrawList.push_back(SpriteListEntry(nullptr, 0, 0, 0, stage));
}

static void clear_sprite_list()
{
    sprlist.clear();
}

static void add_to_sprite_list(IDriverDependantBitmap* ddb, int x, int y, int zorder, uint32_t id = 0u)
{
    assert(ddb);
    // completely invisible, so don't draw it at all
    if (ddb->GetAlpha() == 0)
        return;

    sprlist.push_back(SpriteListEntry(ddb, x, y, zorder, -1, id));
}

// Sprite drawing order sorting function,
// where equal zorder is resolved by comparing optional IDs too.
static bool spritelistentry_less(const SpriteListEntry &e1, const SpriteListEntry &e2)
{
    return (e1.Z < e2.Z) ||
        ((e1.Z == e2.Z) && (e1.DrawIndex < e2.DrawIndex));
}

// copy the sorted sprites into the Things To Draw list
static void draw_sprite_list()
{
    std::sort(sprlist.begin(), sprlist.end(), spritelistentry_less);
    thingsToDrawList.insert(thingsToDrawList.end(), sprlist.begin(), sprlist.end());
}

// Push the gathered list of sprites into the active graphic renderer
void put_sprite_list_on_screen(bool in_room);
//
//------------------------------------------------------------------------

void repair_alpha_channel(Bitmap *dest, Bitmap *bgpic)
{
    // Repair the alpha channel, because sprites may have been drawn
    // over it by the buttons, etc
    int theWid = (dest->GetWidth() < bgpic->GetWidth()) ? dest->GetWidth() : bgpic->GetWidth();
    int theHit = (dest->GetHeight() < bgpic->GetHeight()) ? dest->GetHeight() : bgpic->GetHeight();
    for (int y = 0; y < theHit; y++) 
    {
        unsigned int *destination = ((unsigned int*)dest->GetScanLineForWriting(y));
        unsigned int *source = ((unsigned int*)bgpic->GetScanLineForWriting(y));
        for (int x = 0; x < theWid; x++) 
        {
            destination[x] |= (source[x] & 0xff000000);
        }
    }
}


// used by GUI renderer to draw images
// NOTE: use_alpha arg is for backward compatibility (legacy draw modes)
void draw_gui_sprite(Bitmap *ds, int pic, int x, int y, bool use_alpha, BlendMode blend_mode)
{
    draw_gui_sprite(ds, use_alpha, x, y, spriteset[pic],
        (game.SpriteInfos[pic].Flags & SPF_ALPHACHANNEL) != 0, blend_mode);
}

void draw_gui_sprite(Bitmap *ds, bool use_alpha, int x, int y, Bitmap *sprite, bool src_has_alpha,
    BlendMode blend_mode, int alpha)
{
    if (alpha <= 0)
        return;

    const bool ds_has_alpha = (ds->GetColorDepth() == 32);
    if (use_alpha && game.options[OPT_NEWGUIALPHA] == kGuiAlphaRender_Proper)
    {
        GfxUtil::DrawSpriteBlend(ds, Point(x, y), sprite, blend_mode, ds_has_alpha, src_has_alpha, alpha);
    }
    // Backwards-compatible drawing
    else if (use_alpha && ds_has_alpha && (game.options[OPT_NEWGUIALPHA] == kGuiAlphaRender_AdditiveAlpha) && (alpha == 0xFF))
    {
        if (src_has_alpha)
            set_additive_alpha_blender();
        else
            set_opaque_alpha_blender();
        ds->TransBlendBlt(sprite, x, y);
    }
    else
    {
        GfxUtil::DrawSpriteWithTransparency(ds, sprite, x, y, alpha);
    }
}

void draw_gui_sprite_v330(Bitmap *ds, int pic, int x, int y, bool use_alpha, BlendMode blend_mode)
{
    draw_gui_sprite(ds, pic, x, y, use_alpha && (loaded_game_file_version >= kGameVersion_330), blend_mode);
}


// Avoid freeing and reallocating the memory if possible
Bitmap *recycle_bitmap(Bitmap *bimp, int coldep, int wid, int hit, bool make_transparent)
{
    assert(wid > 0 && hit > 0);

    if (bimp != nullptr)
    {
        // same colour depth, width and height -> reuse
        if ((bimp->GetColorDepth() == coldep) && (bimp->GetWidth() == wid)
                && (bimp->GetHeight() == hit))
        {
            bimp->ResetClip();
            if (make_transparent)
            {
                bimp->ClearTransparent();
            }
            return bimp;
        }

        delete bimp;
    }
    bimp = make_transparent ? BitmapHelper::CreateTransparentBitmap(wid, hit,coldep) :
        BitmapHelper::CreateBitmap(wid, hit,coldep);
    return bimp;
}

void recycle_bitmap(std::unique_ptr<Common::Bitmap> &bimp, int coldep, int wid, int hit, bool make_transparent)
{
    bimp.reset(recycle_bitmap(bimp.release(), coldep, wid, hit, make_transparent));
}

// Get the local tint at the specified X & Y co-ordinates, based on
// room regions and SetAmbientTint
// tint_amnt will be set to 0 if there is no tint enabled
// if this is the case, then light_lev holds the light level (0=none)
void get_local_tint(int xpp, int ypp, bool use_region_tint,
                    int *tint_amnt, int *tint_r, int *tint_g,
                    int *tint_b, int *tint_lit,
                    int *light_lev) {

    int tint_level = 0, light_level = 0;
    int tint_amount = 0;
    int tint_red = 0;
    int tint_green = 0;
    int tint_blue = 0;
    int tint_light = 255;

    if (use_region_tint) {
        int onRegion = 0;

        if ((play.ground_level_areas_disabled & GLED_EFFECTS) == 0) {
            // check if the player is on a region, to find its
            // light/tint level
            onRegion = GetRegionIDAtRoom(xpp, ypp);
            if (onRegion == 0) {
                // when walking, he might just be off a walkable area
                onRegion = GetRegionIDAtRoom(xpp - 3, ypp);
                if (onRegion == 0)
                    onRegion = GetRegionIDAtRoom(xpp + 3, ypp);
                if (onRegion == 0)
                    onRegion = GetRegionIDAtRoom(xpp, ypp - 3);
                if (onRegion == 0)
                    onRegion = GetRegionIDAtRoom(xpp, ypp + 3);
            }
        }

        if ((onRegion > 0) && (onRegion < MAX_ROOM_REGIONS)) {
            light_level = thisroom.Regions[onRegion].Light;
            tint_level = thisroom.Regions[onRegion].Tint;
        }
        else if (onRegion <= 0) {
            light_level = thisroom.Regions[0].Light;
            tint_level = thisroom.Regions[0].Tint;
        }

        int tint_sat = (tint_level >> 24) & 0xFF;
        if ((game.color_depth == 1) || ((tint_level & 0x00ffffff) == 0) ||
            (tint_sat == 0))
            tint_level = 0;

        if (tint_level) {
            tint_red = (unsigned char)(tint_level & 0x000ff);
            tint_green = (unsigned char)((tint_level >> 8) & 0x000ff);
            tint_blue = (unsigned char)((tint_level >> 16) & 0x000ff);
            tint_amount = tint_sat;
            tint_light = light_level;
        }

        if (play.rtint_enabled)
        {
            if (play.rtint_level > 0)
            {
                // override with room tint
                tint_red = play.rtint_red;
                tint_green = play.rtint_green;
                tint_blue = play.rtint_blue;
                tint_amount = play.rtint_level;
                tint_light = play.rtint_light;
            }
            else
            {
                // override with room light level
                tint_amount = 0;
                light_level = play.rtint_light;
            }
        }
    }

    // copy to output parameters
    *tint_amnt = tint_amount;
    *tint_r = tint_red;
    *tint_g = tint_green;
    *tint_b = tint_blue;
    *tint_lit = tint_light;
    if (light_lev)
        *light_lev = light_level;
}




// Applies the specified RGB Tint or Light Level to the ObjTexture 'actsp'.
// Used for software render mode only.
static void apply_tint_or_light(ObjTexture &actsp, int light_level,
                         int tint_amount, int tint_red, int tint_green,
                         int tint_blue, int tint_light, int coldept,
                         Bitmap *blitFrom) {

 // In a 256-colour game, we cannot do tinting or lightening
 // (but we can do darkening, if light_level < 0)
 if (game.color_depth == 1) {
     if ((light_level > 0) || (tint_amount != 0))
         return;
 }

 // we can only do tint/light if the colour depths match
 if (game.GetColorDepth() == actsp.Bmp->GetColorDepth()) {
     std::unique_ptr<Bitmap> oldwas;
     // if the caller supplied a source bitmap, ->Blit from it
     // (used as a speed optimisation where possible)
     if (blitFrom) 
         oldwas.reset(blitFrom);
     // otherwise, make a new target bmp
     else {
         oldwas = std::move(actsp.Bmp);
         actsp.Bmp.reset(BitmapHelper::CreateBitmap(oldwas->GetWidth(), oldwas->GetHeight(), coldept));
     }
     Bitmap *active_spr = actsp.Bmp.get();

     if (tint_amount) {
         // It is an RGB tint
         tint_image(active_spr, oldwas.get(), tint_red, tint_green, tint_blue, tint_amount, tint_light);
     }
     else {
         // the RGB values passed to set_trans_blender decide whether it will darken
         // or lighten sprites ( <128=darken, >128=lighten). The parameter passed
         // to LitBlendBlt defines how much it will be darkened/lightened by.
         
         int lit_amnt;
         active_spr->FillTransparent();
         // It's a light level, not a tint
         if (game.color_depth == 1) {
             // 256-col
             lit_amnt = (250 - ((-light_level) * 5)/2);
         }
         else {
             // hi-color
             if (light_level < 0)
                 set_my_trans_blender(8,8,8,0);
             else
                 set_my_trans_blender(248,248,248,0);
             lit_amnt = abs(light_level) * 2;
         }

         active_spr->LitBlendBlt(oldwas.get(), 0, 0, lit_amnt);
     }

     if (oldwas.get() == blitFrom)
         oldwas.release();

 }
 else if (blitFrom) {
     // sprite colour depth != game colour depth, so don't try and tint
     // but we do need to do something, so copy the source
     Bitmap *active_spr = actsp.Bmp.get();
     active_spr->Blit(blitFrom, 0, 0, 0, 0, active_spr->GetWidth(), active_spr->GetHeight());
 }

}

// Generates a transformed sprite, using src image and parameters;
// * if transformation is necessary - writes into dst and returns dst;
// * if no transformation is necessary - simply returns src;
// Used for software render mode only.
static Bitmap *transform_sprite(std::unique_ptr<Bitmap> &dst, Bitmap *src, bool src_has_alpha,
    const Size &dst_sz, GraphicFlip flip = Common::kFlip_None)
{
    assert(!dst_sz.IsNull());

    if ((src->GetSize() == dst_sz) && (flip == kFlip_None))
        return src; // No transform: return source image

    recycle_bitmap(dst, src->GetColorDepth(), dst_sz.Width, dst_sz.Height, true);

    // If scaled: first scale then optionally mirror
    if (src->GetSize() != dst_sz)
    {
        // 8-bit support: ensure that anti-aliasing routines have a palette
        // to use for mapping while faded out.
        // FIXME: investigate if this may be moved out and not repeated, or at least passed as a parameter!
        const bool do_select_palette = (in_new_room > 0) && play.ShouldAASprites() && (src->GetColorDepth() == 1);
        if (do_select_palette)
            select_palette(palette);

        if (flip != kFlip_None)
        {
            // TODO: "flip self" function could have allowed to optimize this
            Bitmap tempbmp;
            tempbmp.CreateTransparent(dst_sz.Width, dst_sz.Height, src->GetColorDepth());
            if (play.ShouldAASprites() && !src_has_alpha)
                tempbmp.AAStretchBlt(src, RectWH(dst_sz), kBitmap_Transparency);
            else
                tempbmp.StretchBlt(src, RectWH(dst_sz), kBitmap_Transparency);
            dst->FlipBlt(&tempbmp, 0, 0, flip);
        }
        else
        {
            if (play.ShouldAASprites() && !src_has_alpha)
                dst->AAStretchBlt(src, RectWH(dst_sz), kBitmap_Transparency);
            else
                dst->StretchBlt(src, RectWH(dst_sz), kBitmap_Transparency);
        }

        if (do_select_palette)
            unselect_palette();
    }
    else
    {
        // If not scaled, then simply blit mirrored
        dst->FlipBlt(src, 0, 0, flip);
    }
    return dst.get(); // return transformed result
}

// Draws the specified 'sppic' sprite onto ObjTexture 'actsp' at the specified
// width and height, and flips the sprite if necessary.
// If any transformation was necessary, then writes the result into ObjTexture.
// Returns if any transformation was done.
// Used for software render mode only.
static bool transform_sprite(ObjTexture &actsp, int sppic,
    const Size &dst_sz, GraphicFlip flip = Common::kFlip_None)
{
    Bitmap *src = spriteset[sppic];
    Bitmap *result = transform_sprite(actsp.Bmp, src, (game.SpriteInfos[sppic].Flags & SPF_ALPHACHANNEL) != 0,
        dst_sz, flip);
    return result != src;
}

// Prepares the ObjTexture 'actsp' for an arbitrary room entity.
// Records visual parameters in ObjectCache 'objsav'.
// Returns true if actsp's raw image was not changed and actsps is still
// intact from last time; false otherwise, which means that the raw bitmap
// was redrawn.
// Hardware-accelerated renderers always return true, because they do not
// require preparing the raw bitmap.
// Except if alwaysUseSoftware is set, in which case even HW renderers
// construct the image in software mode as well.
static bool construct_object_gfx(const ViewFrame *vf, int pic,
    const Size &scale_size,
    int tint_flags, // OBJF_* flags related to using tint and light fx
    const ObjectCache &objsrc, // source item to acquire values from
    ObjectCache &objsav, // cache item to use
    ObjTexture &actsp, // object texture to draw upon
    bool optimize_by_position, // allow to optimize walk-behind merging using object's pos
    bool force_software)
{
    const bool use_hw_transform = !force_software && !drawstate.SoftwareRender;

    int tint_red, tint_green, tint_blue;
    int tint_level, tint_light, light_level;
    tint_red = tint_green = tint_blue = tint_level = tint_light = light_level = 0;

    if (tint_flags & OBJF_HASTINT)
    {
        // object specific tint, use it
        tint_red = objsrc.tintr;
        tint_green = objsrc.tintg;
        tint_blue = objsrc.tintb;
        tint_level = objsrc.tintamnt;
        tint_light = objsrc.tintlight;
        light_level = 0;
    }
    else if (tint_flags & OBJF_HASLIGHT)
    {
        light_level = objsrc.tintlight;
    }
    else
    {
        // get the ambient or region tint
        get_local_tint(objsrc.x, objsrc.y, (tint_flags & OBJF_USEREGIONTINTS) != 0,
            &tint_level, &tint_red, &tint_green, &tint_blue,
            &tint_light, &light_level);
    }

    // check whether the image should be flipped
    bool is_mirrored = false;
    int specialpic = pic;
    if (vf && (vf->pic == pic) && ((vf->flags & VFLG_FLIPSPRITE) != 0))
    {
        is_mirrored = true;
        specialpic = -pic;
    }

    actsp.SpriteID = pic; // for texture sharing

    // Hardware accelerated mode: always use original sprite and apply texture transform
    if (use_hw_transform)
    {
        // HW acceleration
        const bool is_texture_intact =
            (objsav.sppic == specialpic) && !actsp.IsChangeNotified();
        objsav.sppic = specialpic;
        objsav.tintamnt = tint_level;
        objsav.tintr = tint_red;
        objsav.tintg = tint_green;
        objsav.tintb = tint_blue;
        objsav.tintlight = tint_light;
        objsav.lightlev = light_level;
        objsav.zoom = objsrc.zoom;
        objsav.mirrored = is_mirrored;
        return is_texture_intact;
    }

    //
    // Software mode below
    //
    // They want to draw it in software mode with the hw driver, so force a redraw (???)
    if (!drawstate.SoftwareRender)
    {
        objsav.sppic = INT32_MIN;
    }

    // If we have the image cached, use it
    if ((objsav.image != nullptr) &&
        (objsav.sppic == specialpic) &&
        // not a dynamic sprite, or not sprite modified lately
        (!actsp.IsChangeNotified()) &&
        (objsav.tintamnt == tint_level) &&
        (objsav.tintlight == tint_light) &&
        (objsav.tintr == tint_red) &&
        (objsav.tintg == tint_green) &&
        (objsav.tintb == tint_blue) &&
        (objsav.lightlev == light_level) &&
        (objsav.zoom == objsrc.zoom) &&
        (objsav.mirrored == is_mirrored))
    {
        // If the image is the same, we can use it cached
        if ((drawstate.WalkBehindMethod != DrawOverCharSprite) &&
            (actsp.Bmp != nullptr))
        {
            return true;
        }
        // Check if the X & Y co-ords are the same, too -- if so, there
        // is scope for further optimisations
        if (optimize_by_position &&
            (objsav.x == objsrc.x) &&
            (objsav.y == objsrc.y) &&
            (actsp.Bmp != nullptr) &&
            (walk_behind_baselines_changed == 0))
        {
            return true;
        }
        recycle_bitmap(actsp.Bmp, objsav.image->GetColorDepth(), objsav.image->GetWidth(), objsav.image->GetHeight());
        actsp.Bmp->Blit(objsav.image.get(), 0, 0);
        return false; // image was modified
    }

    // Not cached, so draw the image
    Bitmap *sprite = spriteset[pic];
    const int coldept = sprite->GetColorDepth();
    const int src_sprwidth = sprite->GetWidth();
    const int src_sprheight = sprite->GetHeight();
    bool actsps_used = false;
    // draw the base sprite, scaled and flipped as appropriate
    actsps_used = transform_sprite(actsp, pic, scale_size, is_mirrored ? kFlip_Horizontal : kFlip_None);
    if (!actsps_used)
    {
        // ensure actsps exists // CHECKME: why do we need this in hardware accel mode too?
        recycle_bitmap(actsp.Bmp, coldept, src_sprwidth, src_sprheight);
    }

    // apply tints or lightenings where appropriate, else just copy the source bitmap
    if ((tint_level > 0) || (light_level != 0))
    {
        // direct read from source bitmap, where possible
        Bitmap *blit_from = nullptr;
        if (!actsps_used)
            blit_from = sprite;

        apply_tint_or_light(actsp, light_level, tint_level, tint_red,
            tint_green, tint_blue, tint_light, coldept,
            blit_from);
    }
    else if (!actsps_used)
    {
        // no scaling, flipping or tinting was done, so just blit it normally
        actsp.Bmp->Blit(sprite, 0, 0);
    }

    // Create the cached image and store it
    objsav.in_use = true;
    recycle_bitmap(objsav.image, actsp.Bmp->GetColorDepth(), actsp.Bmp->GetWidth(), actsp.Bmp->GetHeight());
    objsav.image->Blit(actsp.Bmp.get(), 0, 0);
    objsav.sppic = specialpic;
    objsav.tintamnt = tint_level;
    objsav.tintr = tint_red;
    objsav.tintg = tint_green;
    objsav.tintb = tint_blue;
    objsav.tintlight = tint_light;
    objsav.lightlev = light_level;
    objsav.zoom = objsrc.zoom;
    objsav.mirrored = is_mirrored;
    objsav.x = objsrc.x;
    objsav.y = objsrc.y;
    return false; // image was modified
}

// Do last time setup to the ObjTexture 'actsp', prepare the final texture.
// Applies walk-behinds to an object's raw bitmap if necessary (software mode);
// update the object's texture from the sprite if necessary,
// apply texture parameters from ObjectCache 'objsav'.
// - atx and aty are coordinates of the top-left object's corner in the room;
// - usebasel is object's z-order, it may be modified within the function;
// TODO: possibly makes sense to split this function into parts later.
void prepare_and_add_object_gfx(
    const ObjectCache &objsav,
    ObjTexture &actsp, bool actsp_modified,
    const Size &scale_size,
    int atx, int aty, int &usebasel, bool use_walkbehinds,
    int transparency, bool hw_accel)
{
    // Handle the walk-behinds, according to the WalkBehindMethod.
    // This potentially may edit actsp's raw bitmap if actsp_modified is set.
    if (use_walkbehinds)
    {
        // Only merge sprite with the walk-behinds in software mode
        if ((drawstate.WalkBehindMethod == DrawOverCharSprite) && (actsp_modified))
        {
            walkbehinds_cropout(actsp.Bmp.get(), atx, aty, usebasel);
        }
    }
    else
    {
        // Ignore walk-behinds by shifting baseline to a larger value
        // CHECKME: may this fail if WB somehow got larger than room baseline?
        if (drawstate.WalkBehindMethod == DrawAsSeparateSprite)
        {
            usebasel += thisroom.Height;
        }
    }

    // Sync object texture with the raw sprite bitmap.
    if ((actsp.Ddb == nullptr) || (actsp_modified))
    {
        sync_object_texture(actsp, (game.SpriteInfos[actsp.SpriteID].Flags & SPF_ALPHACHANNEL) != 0);
#if (AGS_PLATFORM_DEBUG)
        actsp.Ddb->SetTag(objsav.tag);
#endif
    }

    // Now when we have a ready texture, assign texture properties
    // (transform, effects, and so forth)
    assert(actsp.Ddb);
    if (hw_accel)
    {
        actsp.Ddb->SetStretch(scale_size.Width, scale_size.Height);
        actsp.Ddb->SetFlippedLeftRight(objsav.mirrored);
        actsp.Ddb->SetTint(objsav.tintr, objsav.tintg, objsav.tintb, (objsav.tintamnt * 256) / 100);

        if (objsav.tintamnt > 0)
        {
            if (objsav.tintlight == 0)  // luminance of 0 -- pass 1 to enable
                actsp.Ddb->SetLightLevel(1);
            else if (objsav.tintlight < 250)
                actsp.Ddb->SetLightLevel(objsav.tintlight);
            else
                actsp.Ddb->SetLightLevel(0);
        }
        else if (objsav.lightlev != 0)
            actsp.Ddb->SetLightLevel((objsav.lightlev * 25) / 10 + 256);
        else
            actsp.Ddb->SetLightLevel(0);
    }

    actsp.Ddb->SetAlpha(GfxDef::LegacyTrans255ToAlpha255(transparency));
}

// Prepares a actsps element for RoomObject; updates object cache.
// Software mode draws an actual bitmap in actsp, hardware mode only
// assigns parameters, which may further be assigned to a texture.
bool construct_object_gfx(int objid, bool force_software)
{
    const RoomObject &obj = objs[objid];
    const int sprite_id = spriteset.DoesSpriteExist(obj.num) ? obj.num : 0;

    ObjectCache objsrc(sprite_id, obj.tint_r, obj.tint_g, obj.tint_b,
        obj.tint_level, obj.tint_light, 0 /* skip */, obj.zoom, false /* skip */,
        obj.x, obj.y);

    return construct_object_gfx(
        (obj.view != UINT16_MAX) ? &views[obj.view].loops[obj.loop].frames[obj.frame] : nullptr,
        sprite_id,
        Size(obj.last_width, obj.last_height),
        obj.flags & OBJF_TINTLIGHTMASK,
        objsrc,
        objcache[objid],
        actsps[objid],
        true,
        force_software);
}

void prepare_objects_for_drawing()
{
    set_our_eip(32);

    const bool hw_accel = !drawstate.SoftwareRender;

    for (uint32_t objid = 0; objid < croom->numobj; ++objid)
    {
        const RoomObject &obj = objs[objid];
        if (obj.on != 1) // WARNING: 'on' may have other values than 0 and 1 !!
            continue; // disabled
        if ((obj.x >= thisroom.Width) || (obj.y < 1))
            continue; // offscreen

        eip_guinum = objid;
        const ObjectCache &objsav = objcache[objid];
        ObjTexture &actsp = actsps[objid];

        // Calculate sprite top-left position in the room and baseline
        const int atx = data_to_game_coord(obj.x);
        const int aty = data_to_game_coord(obj.y) - obj.last_height;
        int usebasel = obj.get_baseline();

        // Generate raw bitmap in ObjTexture and store parameters in ObjectCache.
        bool actsp_modified = !construct_object_gfx(objid, false);
        // Prepare the object texture
        prepare_and_add_object_gfx(objsav, actsp, actsp_modified,
            Size(obj.last_width, obj.last_height), atx, aty, usebasel,
            (obj.flags & OBJF_NOWALKBEHINDS) == 0, obj.transparent, hw_accel);
        // Finally, add the texture to the draw list
        add_to_sprite_list(actsp.Ddb, atx, aty, usebasel, actsp.DrawIndex);
    }
}



// Draws srcimg onto destimg, tinting to the specified level
// Totally overwrites the contents of the destination image
void tint_image (Bitmap *ds, Bitmap *srcimg, int red, int grn, int blu, int light_level, int luminance) {

    if ((srcimg->GetColorDepth() != ds->GetColorDepth()) ||
        (srcimg->GetColorDepth() <= 8)) {
            debug_script_warn("Image tint failed - images must both be same color depth and not 8-bit");
            // the caller expects something to have been copied
            ds->Blit(srcimg, 0, 0, 0, 0, srcimg->GetWidth(), srcimg->GetHeight());
            return;
    }

    // For performance reasons, we have a seperate blender for
    // when light is being adjusted and when it is not.
    // If luminance >= 250, then normal brightness, otherwise darken
    if (luminance >= 250)
        set_blender_mode (_myblender_color15, _myblender_color16, _myblender_color32, red, grn, blu, 0);
    else
        set_blender_mode (_myblender_color15_light, _myblender_color16_light, _myblender_color32_light, red, grn, blu, 0);

    if (light_level >= 100) {
        // fully colourised
        ds->FillTransparent();
        ds->LitBlendBlt(srcimg, 0, 0, luminance);
    }
    else {
        // light_level is between -100 and 100 normally; 0-100 in
        // this case when it's a RGB tint
        light_level = (light_level * 25) / 10;

        // Copy the image to the new bitmap
        ds->Blit(srcimg, 0, 0, 0, 0, srcimg->GetWidth(), srcimg->GetHeight());
        // Render the colourised image to a temporary bitmap,
        // then transparently draw it over the original image
        Bitmap *finaltarget = BitmapHelper::CreateTransparentBitmap(srcimg->GetWidth(), srcimg->GetHeight(), srcimg->GetColorDepth());
        finaltarget->LitBlendBlt(srcimg, 0, 0, luminance);

        // customized trans blender to preserve alpha channel
        set_my_trans_blender (0, 0, 0, light_level);
        ds->TransBlendBlt (finaltarget, 0, 0);
        delete finaltarget;
    }
}


// Prepares a actsps element for Character; updates character cache.
// Software mode draws an actual bitmap in actsp, hardware mode only
// assigns parameters, which may further be assigned to a texture.
bool construct_char_gfx(int charid, bool force_software)
{
    const CharacterInfo &chin = game.chars[charid];
    const CharacterExtras &chex = charextra[charid];
    const ViewFrame *vf = &views[chin.view].loops[chin.loop].frames[chin.frame];
    const int pic = spriteset.DoesSpriteExist(vf->pic) ? vf->pic : 0;

    ObjectCache chsrc(pic, chex.tint_r, chex.tint_g, chex.tint_b,
        chex.tint_level, chex.tint_light, 0 /* skip */, chex.zoom, false /* skip */,
        chin.x, chin.y);

    return construct_object_gfx(
        vf,
        pic,
        Size(chex.width, chex.height),
        CharFlagsToObjFlags(chin.flags) & OBJF_TINTLIGHTMASK,
        chsrc,
        charcache[charid],
        actsps[charid + ACTSP_OBJSOFF],
        false, // characters cannot optimize by pos, probably because of z coord and view offsets (?)
        force_software);
}

void prepare_characters_for_drawing()
{
    set_our_eip(33);
    const bool hw_accel = !drawstate.SoftwareRender;

    // draw characters
    for (int charid = 0; charid < game.numcharacters; ++charid)
    {
        const CharacterInfo &chin = game.chars[charid];
        if (chin.on == 0)
            continue; // disabled
        if (chin.room != displayed_room)
            continue; // in another room

        eip_guinum = charid;
        const CharacterExtras &chex = charextra[charid];
        const ObjectCache &chsav = charcache[charid];
        ObjTexture &actsp = actsps[charid + ACTSP_OBJSOFF];

        // Calculate sprite top-left position in the room and baseline
        const int atx = chin.actx + chin.pic_xoffs * chex.zoom_offs / 100;
        const int aty = chin.acty + chin.pic_yoffs * chex.zoom_offs / 100;
        int usebasel = chin.get_baseline();

        // Generate raw bitmap in ObjTexture and store parameters in ObjectCache.
        bool actsp_modified = !construct_char_gfx(charid, false);
        // Prepare the object texture
        prepare_and_add_object_gfx(chsav, actsp, actsp_modified,
            Size(chex.width, chex.height), atx, aty, usebasel,
            (chin.flags & CHF_NOWALKBEHINDS) == 0, chin.transparency, hw_accel);
#if (AGS_PLATFORM_DEBUG)
        actsp.Ddb->SetTag(String::FromFormat("CHAR%d:%s", chin.index_id, chin.scrname));
#endif
        // Finally, add the texture to the draw list
        add_to_sprite_list(actsp.Ddb, atx, aty, usebasel, actsp.DrawIndex);
    }
}

Bitmap *get_cached_character_image(int charid)
{
    return actsps[charid + ACTSP_OBJSOFF].Bmp.get();
}

Bitmap *get_cached_object_image(int objid)
{
    return actsps[objid].Bmp.get();
}

void add_walkbehind_image(size_t index, Common::Bitmap *bmp, int x, int y)
{
    if (walkbehindobj.size() <= index)
        walkbehindobj.resize(index + 1);
    walkbehindobj[index].Bmp.reset(); // don't store bitmap if added this way
    walkbehindobj[index].Ddb = recycle_ddb_bitmap(walkbehindobj[index].Ddb, bmp);
    walkbehindobj[index].Pos = Point(x, y);
}


// Add active room overlays to the sprite list
static void add_roomovers_for_drawing()
{
    const auto &overs = get_overlays();
    for (const auto &over : overs)
    {
        if (over.type < 0) continue; // empty slot
        if (!over.IsRoomLayer()) continue; // not a room layer
        if (over.transparency == 255) continue; // skip fully transparent
        Point pos = get_overlay_position(over);
        add_to_sprite_list(overtxs[over.type].Ddb, pos.X, pos.Y, over.zorder, overtxs[over.type].DrawIndex);
    }
}

// Compiles a list of room sprites (characters, objects, background)
void prepare_room_sprites()
{
    // Background sprite is required for the non-software renderers always,
    // and for software renderer in case there are overlapping viewports.
    // Note that software DDB is just a tiny wrapper around bitmap, so overhead is negligible.
    if (current_background_is_dirty || !roomBackgroundBmp)
    {
        roomBackgroundBmp =
            recycle_ddb_bitmap(roomBackgroundBmp, thisroom.BgFrames[play.bg_frame].Graphic.get(), false, true);
    }
    if (drawstate.FullFrameRedraw)
    {
        if (current_background_is_dirty || walkBehindsCachedForBgNum != play.bg_frame)
        {
            if (drawstate.WalkBehindMethod == DrawAsSeparateSprite)
            {
                walkbehinds_generate_sprites();
            }
        }
        add_thing_to_draw(roomBackgroundBmp, 0, 0);
    }
    current_background_is_dirty = false; // Note this is only place where this flag is checked

    clear_sprite_list();

    if ((debug_flags & DBG_NOOBJECTS) == 0)
    {
        prepare_objects_for_drawing();
        prepare_characters_for_drawing();
        add_roomovers_for_drawing();

        if ((debug_flags & DBG_NODRAWSPRITES) == 0)
        {
            set_our_eip(34);

            if (drawstate.WalkBehindMethod == DrawAsSeparateSprite)
            {
                for (size_t wb = 1 /* 0 is "no area" */;
                    (wb < MAX_WALK_BEHINDS) && (wb < walkbehindobj.size()); ++wb)
                {
                    const auto &wbobj = walkbehindobj[wb];
                    if (wbobj.Ddb)
                    {
                        add_to_sprite_list(wbobj.Ddb, wbobj.Pos.X, wbobj.Pos.Y,
                            croom->walkbehind_base[wb], 0u);
                        // when baselines are equal, walk-behinds must be sorted back, so draw index = 0
                    }
                }
            }

            if (pl_any_want_hook(kPluginEvt_PreScreenDraw))
                add_render_stage(kPluginEvt_PreScreenDraw);

            draw_sprite_list();
        }
    }
    set_our_eip(36);

    // Debug room overlay
    update_room_debug();
    if ((debugRoomMask != kRoomAreaNone) && debugRoomMaskObj.Ddb)
        add_thing_to_draw(debugRoomMaskObj.Ddb, 0, 0);
    if ((debugMoveListChar >= 0) && debugMoveListObj.Ddb)
        add_thing_to_draw(debugMoveListObj.Ddb, 0, 0);

    if (pl_any_want_hook(kPluginEvt_PostRoomDraw))
        add_render_stage(kPluginEvt_PostRoomDraw);
}

// Draws the black surface behind (or rather between) the room viewports
void draw_preroom_background()
{
    if (drawstate.FullFrameRedraw)
        return;
    update_black_invreg_and_reset(gfxDriver->GetMemoryBackBuffer());
}

// Draws the room background on the given surface.
//
// NOTE that this is **strictly** for software rendering.
// ds is a full game screen surface, and roomcam_surface is a surface for drawing room camera content to.
// ds and roomcam_surface may be the same bitmap.
// no_transform flag tells to copy dirty regions on roomcam_surface without any coordinate conversion
// whatsoever.
PBitmap draw_room_background(Viewport *view)
{
    set_our_eip(31);

    // For the sake of software renderer, if there is any kind of camera transform required
    // except screen offset, we tell it to draw on separate bitmap first with zero transformation.
    // There have been few reasons for this in the past, primary being that Allegro does not support
    // StretchBlt between different colour depths, but that one may be not relevant now.
    // See Also: comment inside ALSoftwareGraphicsDriver::RenderToBackBuffer().
    const int view_index = view->GetID();
    Bitmap *ds = gfxDriver->GetMemoryBackBuffer();
    // If separate bitmap was prepared for this view/camera pair then use it, draw untransformed
    // and blit transformed whole surface later.
    const bool draw_to_camsurf = CameraDrawData[view_index].Frame != nullptr;
    Bitmap *roomcam_surface = draw_to_camsurf ? CameraDrawData[view_index].Frame.get() : ds;
    {
        // For software renderer: copy dirty rects onto the virtual screen.
        // TODO: that would be SUPER NICE to reorganize the code and move this operation into SoftwareGraphicDriver somehow.
        // Because basically we duplicate sprite batch transform here.

        auto camera = view->GetCamera();
        set_invalidrects_cameraoffs(view_index, camera->GetRect().Left, camera->GetRect().Top);

        // TODO: (by CJ)
        // the following line takes up to 50% of the game CPU time at
        // high resolutions and colour depths - if we can optimise it
        // somehow, significant performance gains to be had
        update_room_invreg_and_reset(view_index, roomcam_surface, thisroom.BgFrames[play.bg_frame].Graphic.get(), draw_to_camsurf);
    }

    return CameraDrawData[view_index].Frame;
}


struct DrawFPS
{
    IDriverDependantBitmap* ddb = nullptr;
    std::unique_ptr<Bitmap> bmp;
    int font = -1; // in case normal font changes at runtime
} gl_DrawFPS;

void dispose_engine_overlay()
{
    gl_DrawFPS.bmp.reset();
    if (gl_DrawFPS.ddb)
        gfxDriver->DestroyDDB(gl_DrawFPS.ddb);
    gl_DrawFPS.ddb = nullptr;
    gl_DrawFPS.font = -1;
}

void draw_fps(const Rect &viewport)
{
    const int font = FONT_NORMAL;
    auto &fpsDisplay = gl_DrawFPS.bmp;
    if (fpsDisplay == nullptr || gl_DrawFPS.font != font)
    {
        recycle_bitmap(fpsDisplay, game.GetColorDepth(), viewport.GetWidth(), (get_font_surface_height_outlined(font) + get_fixed_pixel_size(5)));
        gl_DrawFPS.font = font;
    }

    fpsDisplay->ClearTransparent();
    const color_t text_color = fpsDisplay->GetCompatibleColor(14);
    char base_buffer[20];
    if (!isTimerFpsMaxed()) {
        snprintf(base_buffer, sizeof(base_buffer), "%d", frames_per_second);
    } else {
        snprintf(base_buffer, sizeof(base_buffer), "unlimited");
    }

    char fps_buffer[60];
    // Don't display fps if we don't have enough information (because loop count was just reset)
    float fps = get_real_fps();
    if (!std::isnan(fps)) {
        snprintf(fps_buffer, sizeof(fps_buffer), "FPS: %2.1f / %s", fps, base_buffer);
    } else {
        snprintf(fps_buffer, sizeof(fps_buffer), "FPS: --.- / %s", base_buffer);
    }
    char loop_buffer[60];
    snprintf(loop_buffer, sizeof(loop_buffer), "Loop %u", loopcounter);

    int text_off = get_font_surface_extent(font).first; // TODO: a generic function that accounts for this?
    wouttext_outline(fpsDisplay.get(), 1, 1 - text_off, font, text_color, fps_buffer);
    wouttext_outline(fpsDisplay.get(), viewport.GetWidth() / 2, 1 - text_off, font, text_color, loop_buffer);

    gl_DrawFPS.ddb = recycle_ddb_bitmap(gl_DrawFPS.ddb, gl_DrawFPS.bmp.get());
    int yp = viewport.GetHeight() - fpsDisplay->GetHeight();
    gfxDriver->DrawSprite(1, yp, gl_DrawFPS.ddb);
    invalidate_sprite_glob(1, yp, gl_DrawFPS.ddb);
}

// Draw GUI controls as separate sprites, each on their own texture
static void construct_guictrl_tex(GUIMain &gui)
{
    if (GUI::ShouldSkipControls(&gui))
        return; // don't draw GUI controls

    int draw_index = guiobjddbref[gui.ID];
    for (int i = 0; i < gui.GetControlCount(); ++i, ++draw_index)
    {
        GUIObject *obj = gui.GetControl(i);
        if (!GUI::IsGUIVisible(obj) || // not visible, or hidden by disabled state
            (obj->GetSize().IsNull()))
            continue;
        if (!obj->HasChanged())
            continue;

        auto &objbg = guiobjbg[draw_index];
        Rect obj_surf = obj->CalcGraphicRect(GUI::Options.ClipControls);
        recycle_bitmap(objbg.Bmp, game.GetColorDepth(), obj_surf.GetWidth(), obj_surf.GetHeight(), true);
        obj->Draw(objbg.Bmp.get(), -obj_surf.Left, -obj_surf.Top);

        sync_object_texture(objbg, obj->HasAlphaChannel());
        objbg.Off = Point(obj_surf.GetLT());
        obj->ClearChanged();
    }
}

// Push gui bg & controls textures for the render to the corresponding render target
static void draw_gui_controls_batch(int gui_id)
{
    auto *gui_rtex =  gui_render_tex[gui_id];
    assert(gui_rtex);
    const auto &gui = guis[gui_id];
    auto *gui_bg = guibg[gui_id].Ddb;
    // Create a sub-batch
    gfxDriver->BeginSpriteBatch(gui_rtex, RectWH(0, 0, gui_bg->GetWidth(), gui_bg->GetHeight()),
        SpriteTransform(), kFlip_None);
    // Add GUI itself
    gfxDriver->DrawSprite(0, 0, gui_bg);

    // Don't draw child controls at all if disabled with kGuiDis_Blackout style
    if (GUI::ShouldSkipControls(&gui))
    {
        gfxDriver->EndSpriteBatch();
        return;
    }

    // Add all the GUI controls
    const int draw_index = guiobjddbref[gui_id];
    for (const auto &obj_id : gui.GetControlsDrawOrder())
    {
        GUIObject *obj = gui.GetControl(obj_id);
        if (!GUI::IsGUIVisible(obj) || // not visible, or hidden by disabled state
            (obj->GetSize().IsNull()))
            continue;

        const auto &obj_tx = guiobjbg[draw_index + obj_id];
        auto *obj_ddb = obj_tx.Ddb;
        assert(obj_ddb); // Test for missing texture, might happen if not marked for update
        if (!obj_ddb) continue;
        obj_ddb->SetAlpha(GfxDef::LegacyTrans255ToAlpha255(obj->GetTransparency()));
        gfxDriver->DrawSprite(obj->X + obj_tx.Off.X, obj->Y + obj_tx.Off.Y, obj_ddb);
    }
    gfxDriver->EndSpriteBatch();
}

// Draw GUI and overlays of all kinds, anything outside the room space
void draw_gui_and_overlays()
{
    // Draw gui controls on separate textures if:
    // - it is a 3D renderer (software one may require adjustments -- needs testing)
    // - not legacy alpha blending (may we implement specific texture blend?)
    const bool draw_controls_as_textures =
           !drawstate.SoftwareRender &&
           (game.options[OPT_NEWGUIALPHA] == kGuiAlphaRender_Proper);

    if(pl_any_want_hook(kPluginEvt_PreGUIDraw))
        gfxDriver->AddRenderEvent(kPluginEvt_PreGUIDraw, 0); // render stage

    clear_sprite_list();

    // Add active overlays to the sprite list
    const auto &overs = get_overlays();
    for (const auto &over : overs)
    {
        if (over.type < 0) continue; // empty slot
        if (over.IsRoomLayer()) continue; // not a ui layer
        if (over.transparency == 255) continue; // skip fully transparent
        Point pos = get_overlay_position(over);
        add_to_sprite_list(overtxs[over.type].Ddb, pos.X, pos.Y, over.zorder, overtxs[over.type].DrawIndex);
    }

    // Add GUIs
    set_our_eip(35);
    if (((debug_flags & DBG_NOIFACE)==0) && (displayed_room >= 0)) {
        if (playerchar->activeinv >= MAX_INV) {
            quit("!The player.activeinv variable has been corrupted, probably as a result\n"
                "of an incorrect assignment in the game script.");
        }
        if (playerchar->activeinv < 1)
            GUI::Context.InventoryPic = -1;
        else
            GUI::Context.InventoryPic = game.invinfo[playerchar->activeinv].pic;
        GUI::Options.GreyOutInvWindow = play.inventory_greys_out;

        set_our_eip(37);
        // Prepare and update GUI textures
        {
            for (int index = 0; index < game.numgui; ++index)
            {
                auto &gui = guis[index];
                if (!gui.IsDisplayed()) continue; // not on screen
                if (!gui.HasChanged() && !gui.HasControlsChanged()) continue; // no changes: no need to update image
                if (gui.Transparency == 255) continue; // 100% transparent

                eip_guinum = index;
                set_our_eip(372);
                const bool draw_with_controls = !draw_controls_as_textures;
                if (gui.HasChanged() || (draw_with_controls && gui.HasControlsChanged()))
                {
                    auto &gbg = guibg[index];
                    recycle_bitmap(gbg.Bmp, game.GetColorDepth(), gui.Width, gui.Height, true);
                    if (draw_with_controls)
                        gui.DrawWithControls(gbg.Bmp.get());
                    else
                        gui.DrawSelf(gbg.Bmp.get());

                    const bool is_alpha = gui.HasAlphaChannel();
                    if (is_alpha)
                    {
                        if ((game.options[OPT_NEWGUIALPHA] == kGuiAlphaRender_Legacy) && (gui.BgImage > 0))
                        {
                            // old-style (pre-3.0.2) GUI alpha rendering
                            repair_alpha_channel(gbg.Bmp.get(), spriteset[gui.BgImage]);
                        }
                    }
                    sync_object_texture(gbg, is_alpha);
#if (AGS_PLATFORM_DEBUG)
                    gbg.Ddb->SetTag(String::FromFormat("GUI%d:%s", gui.ID, gui.Name.GetCStr()));
#endif
                }

                set_our_eip(373);
                // Update control textures, if they have changed themselves
                if (draw_controls_as_textures && gui.HasControlsChanged())
                {
                    construct_guictrl_tex(gui);
                }

                set_our_eip(374);

                gui.ClearChanged();
            }
        }
        set_our_eip(38);
        // Draw the GUIs
        for (int index = 0; index < game.numgui; ++index)
        {
            const auto &gui = guis[index];
            // Don't draw GUI if it's not on, or it's a "GUIs Turn Off When Disabled" state
            if (!GUI::IsGUIVisible(&gui))
                continue;
            if (gui.Transparency == 255)
                continue; // 100% transparent

            auto *gui_ddb = guibg[index].Ddb;
            assert(gui_ddb); // Test for missing texture, might happen if not marked for update
            if (!gui_ddb) continue;
            if (draw_controls_as_textures)
            {
                gui_render_tex[index] = recycle_render_target(gui_render_tex[index],
                    gui_ddb->GetWidth(), gui_ddb->GetHeight(), gui_ddb->GetColorDepth(), false);
                // Render control textures onto the GUI texture
                draw_gui_controls_batch(index);
                // Replace gui bg ddb with a render target texture,
                // and push it to the sprite list instead
                gui_ddb = gui_render_tex[index];
            }
            gui_ddb->SetAlpha(GfxDef::LegacyTrans255ToAlpha255(gui.Transparency));
            add_to_sprite_list(gui_ddb, gui.X, gui.Y, gui.ZOrder, guibg[index].DrawIndex);
        }
    }

    // Move the resulting sprlist with guis and overlays to render
    draw_sprite_list();
    put_sprite_list_on_screen(false);
    set_our_eip(1099);
}

// Push the gathered list of sprites into the active graphic renderer
void put_sprite_list_on_screen(bool in_room)
{
    for (const auto &t : thingsToDrawList)
    {
        assert(t.DDB || (t.RenderStage >= 0));
        if (t.DDB)
        {
            if (!t.DDB->IsValid())
                continue; // skip empty DDBs
            if (t.DDB->GetAlpha() == 0)
                continue; // skip completely invisible things
            // mark the image's region as dirty
            invalidate_sprite(t.X, t.Y, t.DDB, in_room);
            // push to the graphics driver
            gfxDriver->DrawSprite(t.X, t.Y, t.DDB);
        }
        else if (t.RenderStage >= 0)
        {
            // meta entry to run the plugin hook
            gfxDriver->AddRenderEvent(t.RenderStage, 0);
        }
    }

    set_our_eip(1100);
}

bool GfxDriverSpriteEvtCallback(int evt, intptr_t data)
{
    if (displayed_room < 0)
    {
        // if no room loaded, various stuff won't be initialized yet
        return false;
    }
    return (pl_run_plugin_hooks(evt, data) != 0);
}

void GfxDriverOnInitCallback(void *data)
{
    pl_run_plugin_init_gfx_hooks(gfxDriver->GetDriverID(), data);
}

// Schedule room rendering: background, objects, characters
static void construct_room_view()
{
    draw_preroom_background();
    prepare_room_sprites();
    // reset the Baselines Changed flag now that we've drawn stuff
    walk_behind_baselines_changed = 0;

    for (const auto &viewport : play.GetRoomViewportsZOrdered())
    {
        if (!viewport->IsVisible())
            continue;
        auto camera = viewport->GetCamera();
        if (!camera)
            continue;

        const Rect &view_rc = viewport->GetRect();
        const Rect &cam_rc = camera->GetRect();
        const float view_sx = (float)view_rc.GetWidth() / (float)cam_rc.GetWidth();
        const float view_sy = (float)view_rc.GetHeight() / (float)cam_rc.GetHeight();
        const SpriteTransform view_trans(view_rc.Left, view_rc.Top, view_sx, view_sy);
        const SpriteTransform cam_trans(-cam_rc.Left, -cam_rc.Top);

        if (drawstate.FullFrameRedraw)
        {
            // For hw renderer we draw everything as a sprite stack;
            // viewport-camera pair is done as 2 nested scene nodes,
            // where first defines how camera's image translates into the viewport on screen,
            // and second - how room's image translates into the camera.
            gfxDriver->BeginSpriteBatch(view_rc, view_trans, RENDER_BATCH_ROOM_LAYER);
            gfxDriver->BeginSpriteBatch(Rect(), cam_trans);
            gfxDriver->SetStageScreen(cam_rc.GetSize(), cam_rc.Left, cam_rc.Top);
            put_sprite_list_on_screen(true);
            gfxDriver->EndSpriteBatch();
            gfxDriver->EndSpriteBatch();
        }
        else
        {
            // For software renderer - combine viewport and camera in one batch,
            // due to how the room drawing is implemented currently in the software mode.
            // TODO: review this later?
            gfxDriver->BeginSpriteBatch(view_rc, view_trans, RENDER_BATCH_ROOM_LAYER);

            if (CameraDrawData[viewport->GetID()].Frame == nullptr && CameraDrawData[viewport->GetID()].IsOverlap)
            { // room background is prepended to the sprite stack
              // TODO: here's why we have blit whole piece of background now:
              // if we draw directly to the virtual screen overlapping another
              // viewport, then we'd have to also mark and repaint every our
              // region located directly over their dirty regions. That would
              // require to update regions up the stack, converting their
              // coordinates (cam1 -> screen -> cam2).
              // It's not clear whether this is worth the effort, but if it is,
              // then we'd need to optimise view/cam data first.
                gfxDriver->BeginSpriteBatch(Rect(), cam_trans);
                gfxDriver->DrawSprite(0, 0, roomBackgroundBmp);
            }
            else
            { // room background is drawn by dirty rects system
                PBitmap bg_surface = draw_room_background(viewport.get());
                gfxDriver->BeginSpriteBatch(Rect(), cam_trans, kFlip_None, bg_surface);
            }
            put_sprite_list_on_screen(true);
            gfxDriver->EndSpriteBatch();
            gfxDriver->EndSpriteBatch();
        }
    }

    clear_draw_list();
}

// Schedule ui rendering
static void construct_ui_view()
{
    gfxDriver->BeginSpriteBatch(play.GetUIViewport(), SpriteTransform(), RENDER_BATCH_UI_LAYER);
    draw_gui_and_overlays();
    gfxDriver->EndSpriteBatch();
    clear_draw_list();
}

// Prepares overlay textures;
// but does not put them on screen yet - that's done in respective construct_*_view functions
static void construct_overlays()
{
    const bool is_software_mode = drawstate.SoftwareRender;
    const bool crop_walkbehinds = (drawstate.WalkBehindMethod == DrawOverCharSprite);

    auto &overs = get_overlays();
    if (overtxs.size() < overs.size())
    {
        overtxs.resize(overs.size());
        if (is_software_mode)
            overcache.resize(overs.size(), Point(INT32_MIN, INT32_MIN));
    }

    for (size_t i = 0; i < overs.size(); ++i)
    {
        auto &over = overs[i];
        if (over.type < 0) continue; // empty slot
        if (over.transparency == 255) continue; // skip fully transparent

        auto &overtx = overtxs[i];
        bool has_changed = over.HasChanged();
        // If walk behinds are drawn over the cached object sprite, then check if positions were updated
        if (crop_walkbehinds && over.IsRoomLayer())
        {
            Point pos = get_overlay_position(over);
            has_changed |= (pos.X != overcache[i].X || pos.Y != overcache[i].Y);
            overcache[i].X = pos.X; overcache[i].Y = pos.Y;
        }

        // Test if must recache the unloaded sprite in software mode
        has_changed |= (is_software_mode && spriteset.IsAssetUnloaded(over.GetSpriteNum()));

        if (has_changed || overtx.IsChangeNotified())
        {
            overtx.SpriteID = over.GetSpriteNum();
            // For software mode - prepare transformed bitmap if necessary;
            // for hardware-accelerated - use the sprite ID if possible, to avoid redundant sprite load
            // TODO: find a way to unify this code with the character & object ObjTexture preparation;
            // they use practically same approach, except of different fields cache.
            Bitmap *use_bmp = nullptr;
            if (is_software_mode)
            {
                use_bmp = transform_sprite(overtx.Bmp, over.GetImage(), over.HasAlphaChannel(), Size(over.scaleWidth, over.scaleHeight));
                if (crop_walkbehinds && over.IsRoomLayer())
                {
                    if (use_bmp != overtx.Bmp.get())
                    {
                        recycle_bitmap(overtx.Bmp, use_bmp->GetColorDepth(), use_bmp->GetWidth(), use_bmp->GetHeight(), true);
                        overtx.Bmp->Blit(use_bmp);
                    }
                    Point pos = get_overlay_position(over);
                    walkbehinds_cropout(overtx.Bmp.get(), pos.X, pos.Y, over.zorder);
                    use_bmp = overtx.Bmp.get();
                }
            }

            sync_object_texture(overtx, over.HasAlphaChannel());
#if (AGS_PLATFORM_DEBUG)
            overtx.Ddb->SetTag(String::FromFormat("OVER%d", over.type));
#endif
            over.ClearChanged();
        }

        assert(overtx.Ddb); // Test for missing texture, might happen if not marked for update
        if (!overtx.Ddb) continue;
        overtx.Ddb->SetStretch(over.scaleWidth, over.scaleHeight);
        overtx.Ddb->SetAlpha(GfxDef::LegacyTrans255ToAlpha255(over.transparency));
    }
}

void construct_game_scene(bool full_redraw)
{
    set_our_eip(3);

    // React to changes to viewports and cameras (possibly from script) just before the render
    play.UpdateViewports();

    gfxDriver->RenderSpritesAtScreenResolution(usetup.RenderAtScreenRes);

    pl_run_plugin_hooks(kPluginEvt_PreRender, 0);

    // Possible reasons to invalidate whole screen for the software renderer
    if (full_redraw || play.screen_tint > 0 || play.shakesc_length > 0)
        invalidate_screen();

    // Overlays may be both in rooms and ui layer, prepare their textures beforehand
    construct_overlays();

    // TODO: move to game update! don't call update during rendering pass!
    // IMPORTANT: keep the order same because sometimes script may depend on it
    if (displayed_room >= 0)
        play.UpdateRoomCameras();

    // Begin with the parent scene node, defining global offset and flip
    gfxDriver->BeginSpriteBatch(play.GetMainViewport(),
        play.GetGlobalTransform(drawstate.FullFrameRedraw),
        (GraphicFlip)play.screen_flipped);

    // Stage: room viewports
    if (play.screen_is_faded_out == 0 && play.complete_overlay_on == 0)
    {
        if (displayed_room >= 0)
        {
            construct_room_view();
        }
        else if (!drawstate.FullFrameRedraw)
        {
            // black it out so we don't get cursor trails
            // TODO: this is possible to do with dirty rects system now too (it can paint black rects outside of room viewport)
            gfxDriver->GetMemoryBackBuffer()->Fill(0);
        }
    }

    set_our_eip(4);

    // Stage: UI overlay
    if (play.screen_is_faded_out == 0)
    {
        construct_ui_view();
    }

    // End the parent scene node
    gfxDriver->EndSpriteBatch();
}

void construct_game_screen_overlay(bool draw_cursor)
{
    gfxDriver->BeginSpriteBatch(play.GetMainViewport(),
            play.GetGlobalTransform(drawstate.FullFrameRedraw),
            (GraphicFlip)play.screen_flipped);
    if (pl_any_want_hook(kPluginEvt_PostScreenDraw))
    {
        gfxDriver->AddRenderEvent(kPluginEvt_PostScreenDraw, 0);
    }

    // Mouse cursor
    if ((play.screen_is_faded_out == 0) && draw_cursor && !play.mouse_cursor_hidden)
    {
        if (cursor_gstate.HasChanged() || cursor_tx.IsChangeNotified() ||
            // Test if must recache the unloaded sprite in software mode
            (drawstate.SoftwareRender && (cursor_gstate.GetSpriteNum() >= 0) && spriteset.IsAssetUnloaded(cursor_gstate.GetSpriteNum())))
        {
            cursor_tx.SpriteID = cursor_gstate.GetSpriteNum();
            if (cursor_tx.SpriteID != UINT32_MAX)
                sync_object_texture(cursor_tx, cursor_gstate.HasAlphaChannel());
            else
                cursor_tx.Ddb = recycle_ddb_bitmap(cursor_tx.Ddb, cursor_gstate.GetImage(), cursor_gstate.HasAlphaChannel());
            cursor_gstate.ClearChanged();
        }

        assert(cursor_tx.Ddb); // Test for missing texture, might happen if not marked for update
        if (cursor_tx.Ddb)
        {
            // Exclusive sub-batch for mouse cursor, to let filter it out (CHECKME later?)
            gfxDriver->BeginSpriteBatch(Rect(), SpriteTransform(), kFlip_None, nullptr, RENDER_BATCH_MOUSE_CURSOR);
            gfxDriver->DrawSprite(mousex - mouse_hotx, mousey - mouse_hoty, cursor_tx.Ddb);
            invalidate_sprite(mousex - mouse_hotx, mousey - mouse_hoty, cursor_tx.Ddb, false);
            gfxDriver->EndSpriteBatch();
        }
    }

    // Full screen tint fx, covers everything except for fade fx(?) and engine overlay
    if ((play.screen_tint >= 1) && (play.screen_is_faded_out == 0))
        gfxDriver->SetScreenTint(play.screen_tint & 0xff, (play.screen_tint >> 8) & 0xff, (play.screen_tint >> 16) & 0xff);
    gfxDriver->EndSpriteBatch();

    // For hardware-accelerated renderers: legacy letterbox and global screen fade effect
    if (drawstate.FullFrameRedraw)
    {
        gfxDriver->BeginSpriteBatch(play.GetMainViewport(), SpriteTransform());
        // Stage: legacy letterbox mode borders
        if (play.screen_is_faded_out == 0)
            render_black_borders();
        // Stage: full screen fade fx
        if (play.screen_is_faded_out != 0)
            gfxDriver->SetScreenFade(play.fade_to_red, play.fade_to_green, play.fade_to_blue);
        gfxDriver->EndSpriteBatch();
    }
}

void construct_engine_overlay()
{
    const Rect &viewport = RectWH(game.GetGameRes());
    gfxDriver->BeginSpriteBatch(viewport, SpriteTransform(), kFlip_None, nullptr, RENDER_BATCH_ENGINE_OVERLAY);

    if (display_fps != kFPS_Hide)
        draw_fps(viewport);

    gfxDriver->EndSpriteBatch();
}

void update_shakescreen()
{
    // TODO: unify blocking and non-blocking shake update
    play.shake_screen_yoff = 0;
    if (play.shakesc_length > 0)
    {
        if ((loopcounter % play.shakesc_delay) < (play.shakesc_delay / 2))
            play.shake_screen_yoff = play.shakesc_amount;
    }
}

RoomAreaMask get_room_mask_debugmode()
{
    return debugRoomMask;
}

void debug_draw_room_mask(RoomAreaMask mask)
{
    debugRoomMask = mask;
    if (mask == kRoomAreaNone)
        return;

    Bitmap *bmp;
    switch (mask)
    {
    case kRoomAreaHotspot: bmp = thisroom.HotspotMask.get(); break;
    case kRoomAreaWalkBehind: bmp = thisroom.WalkBehindMask.get(); break;
    case kRoomAreaWalkable: bmp = prepare_walkable_areas(-1); break;
    case kRoomAreaRegion: bmp = thisroom.RegionMask.get(); break;
    default: return;
    }

    const int fin_width = data_to_game_coord(thisroom.Width);
    const int fin_height = data_to_game_coord(thisroom.Height);
    // Software mode scaling
    // note we don't use transparency in software mode - may be slow in hi-res games
    if (drawstate.SoftwareRender &&
        (mask != kRoomAreaWalkBehind) &&
        (bmp->GetSize() != Size(fin_width, fin_height)))
    {
        recycle_bitmap(debugRoomMaskObj.Bmp, bmp->GetColorDepth(), fin_width, fin_height);
        debugRoomMaskObj.Bmp->StretchBlt(bmp, RectWH(debugRoomMaskObj.Bmp->GetSize()));
        bmp = debugRoomMaskObj.Bmp.get();
    }

    debugRoomMaskObj.Ddb = recycle_ddb_bitmap(debugRoomMaskObj.Ddb, bmp, false, true);
    debugRoomMaskObj.Ddb->SetAlpha(150);
    debugRoomMaskObj.Ddb->SetStretch(fin_width, fin_height);
}

void debug_draw_movelist(int charnum)
{
    debugMoveListChar = charnum;
}

void update_room_debug()
{
    if (debugRoomMask == kRoomAreaWalkable)
    {
        debug_draw_room_mask(debugRoomMask);
    }
    if (debugMoveListChar >= 0)
    {
        const int mult = drawstate.SoftwareRender ? 1 : thisroom.MaskResolution;
        if (drawstate.SoftwareRender)
            recycle_bitmap(debugMoveListObj.Bmp, game.GetColorDepth(),
                thisroom.Width, thisroom.Height, true);
        else
            recycle_bitmap(debugMoveListObj.Bmp, game.GetColorDepth(),
                thisroom.WalkAreaMask->GetWidth(), thisroom.WalkAreaMask->GetHeight(), true);

        if (game.chars[debugMoveListChar].is_moving_onpath())
        {
            int mlsnum = game.chars[debugMoveListChar].get_movelist_id();
            const MoveList &cmls = mls[mlsnum];
            for (uint32_t i = 0; i < cmls.GetNumStages() - 1; i++) {
                short srcx = cmls.pos[i].X;
                short srcy = cmls.pos[i].Y;
                short targetx = cmls.pos[i + 1].X;
                short targety = cmls.pos[i + 1].Y;
                debugMoveListObj.Bmp->DrawLine(Line(srcx / mult, srcy / mult, targetx / mult, targety / mult),
                    MakeColor(i + 1));
            }
        }
        sync_object_texture(debugMoveListObj);
        debugMoveListObj.Ddb->SetAlpha(150);
        debugMoveListObj.Ddb->SetStretch(thisroom.Width, thisroom.Height);
    }
}

// Draw everything 
void render_graphics(IDriverDependantBitmap *extraBitmap, int extraX, int extraY)
{
    // Don't render if skipping cutscene
    if (play.fast_forward)
        return;
    // Don't render if we've just entered new room and are before fade-in
    // TODO: find out why this is not skipped for 8-bit games
    if ((in_new_room > 0) & (game.color_depth > 1))
        return;

    // TODO: find out if it's okay to move shake to update function
    update_shakescreen();

    // Disable spritecache's auto disposal for the duration of render;
    // this prevents sprites getting deleted while they are part of the draw lists.
    spriteset.EnableAutoFreeMem(false);

    gfxDriver->ClearDrawLists();
    construct_game_scene(false);
    set_our_eip(5);
    // TODO: extraBitmap is a hack, used to place an additional gui element
    // on top of the screen. Normally this should be a part of the game UI stage.
    if (extraBitmap != nullptr)
    {
        gfxDriver->BeginSpriteBatch(play.GetMainViewport(), play.GetGlobalTransform(drawstate.FullFrameRedraw), (GraphicFlip)play.screen_flipped);
        invalidate_sprite(extraX, extraY, extraBitmap, false);
        gfxDriver->DrawSprite(extraX, extraY, extraBitmap);
        gfxDriver->EndSpriteBatch();
    }
    construct_game_screen_overlay(!in_room_transition);
    render_to_screen();

    spriteset.EnableAutoFreeMem(true);

    if (!play.screen_is_faded_out) {
        // always update the palette, regardless of whether the plugin
        // vetos the screen update
        if (bg_just_changed) {
            setpal();
            bg_just_changed = 0;
        }
    }
}
