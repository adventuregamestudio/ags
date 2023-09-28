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
#include "plugin/agsplugin_evts.h"
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
extern GameState play;
extern ScriptSystem scsystem;
extern AGSPlatformDriver *platform;
extern RoomStruct thisroom;
extern unsigned int loopcounter;
extern SpriteCache spriteset;
extern RoomStatus*croom;
extern int our_eip;
extern int in_new_room;
extern RoomObject*objs;
extern std::vector<ViewStruct> views;
extern int displayed_room;
extern CharacterInfo*playerchar;
extern int eip_guinum;
extern int cur_mode,cur_cursor;
extern IDriverDependantBitmap *mouseCursor;
extern int hotx,hoty;
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
    // Whether there are currently remnants of a on-screen effect
    bool ScreenIsDirty = false;
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
    // Raw bitmaps; used for software render mode,
    // or when particular object types require generated image.
    std::unique_ptr<Bitmap> Bmp;
    std::unique_ptr<Bitmap> Bmp2;
    // Corresponding texture, created by renderer
    IDriverDependantBitmap *Ddb = nullptr;
    // Sprite's position
    Point Pos;
    // Texture's offset, *relative* to the logical sprite's position;
    // may be used in case the texture's size is different for any reason
    Point Off;

    ObjTexture() = default;
    ObjTexture(uint32_t sprite_id, Bitmap *bmp, IDriverDependantBitmap *ddb, int x, int y, int xoff = 0, int yoff = 0)
        : SpriteID(sprite_id), Bmp(bmp), Ddb(ddb), Pos(x, y), Off(xoff, yoff) {}
    ObjTexture(const ObjTexture&) = default;
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

    ObjTexture &operator =(ObjTexture &&o)
    {
        SpriteID = o.SpriteID;
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
    std::unique_ptr<Bitmap> image;
    bool  in_use = false; // CHECKME: possibly may be removed
    int   sppic = 0;
    // TODO: pickout tint settings, maybe even share with Char/Obj structs,
    // if plugin API permits!
    short tintr = 0, tintg = 0, tintb = 0, tintamnt = 0, tintlight = 0;
    short lightlev = 0, zoom = 0;
    float rotation = 0.f;
    bool  mirrored = 0;
    int   x = 0, y = 0;

    ObjectCache() = default;
    ObjectCache(int pic_, int tintr_, int tintg_, int tintb_, int tint_amnt_, int tint_light_,
                int light_, int zoom_, float rotation_, bool mirror_, int posx_, int posy_)
        : sppic(pic_), tintr(tintr_), tintg(tintg_), tintb(tintb_)
        , tintamnt(tint_amnt_), tintlight(tint_light_), lightlev(light_)
        , zoom(zoom_), rotation(rotation_), mirrored(mirror_), x(posx_), y(posy_) { }
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
    std::shared_ptr<Texture> GetOrLoad(uint32_t sprite_id, Bitmap *source, bool opaque)
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
        Bitmap *bitmap = source ? source : spriteset[sprite_id];
        if (!bitmap)
            return nullptr;

        txdata.reset(gfxDriver->CreateTexture(bitmap, opaque));
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

    // Texture short-term cache:
    // - caches textures while they are in the immediate use;
    // - this lets to share same texture data among multiple sprites on screen.
    typedef std::weak_ptr<Texture> TexDataRef;
    std::unordered_map<uint32_t, TexDataRef> _txRefs;
} texturecache;

// actsps is used for temporary storage of the bitmap and texture
// of the latest version of the sprite (room objects and characters);
// objects sprites begin with index 0, characters are after ACTSP_OBJSOFF
std::vector<ObjTexture> actsps;
// Walk-behind textures (3D renderers only)
std::vector<ObjTexture> walkbehindobj;
// GUI surfaces
std::vector<ObjTexture> guibg;
// Temp GUI surfaces, in case GUI have to be transformed in software drawing mode
std::vector<std::unique_ptr<Bitmap>> guihelpbg;
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
    int id = -1; // user identifier, for any custom purpose
    IDriverDependantBitmap *ddb = nullptr;
    int x = 0, y = 0;
    Rect aabb;
    int zorder = 0;
    // Tells if this item should take priority during sort if z1 == z2
    // TODO: this is some compatibility feature - find out if may be omited and done without extra struct?
    bool takesPriorityIfEqual = false;
    // Mark for the render stage callback (if >= 0 other fields are ignored)
    int renderStage = -1;
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

// PSP: convert 32 bit RGB to BGR.
Bitmap *convert_32_to_32bgr(Bitmap *tempbl) {

    int i = 0;
    int j = 0;
    unsigned char* current;
    while (i < tempbl->GetHeight())
    {
        current = tempbl->GetScanLineForWriting(i);
        while (j < tempbl->GetWidth())
        {
            current[0] ^= current[2];
            current[2] ^= current[0];
            current[0] ^= current[2];
            current += 4;
            j++;
        }
        i++;
        j = 0;
    }

    return tempbl;
}

// NOTE: Some of these conversions are required  even when using
// D3D and OpenGL rendering, for two reasons:
// 1) certain raw drawing operations are still performed by software
// Allegro methods, hence bitmaps should be kept compatible to any native
// software operations, such as blitting two bitmaps of different formats.
// 2) mobile ports feature an OpenGL renderer built in Allegro library,
// that assumes native bitmaps are in OpenGL-compatible format, so that it
// could copy them to texture without additional changes.
// AGS own OpenGL renderer tries to sync its behavior with the former one.
//
// TODO: make gfxDriver->GetCompatibleBitmapFormat describe all necessary
// conversions, so that we did not have to guess.
//
static Bitmap *AdjustBitmapForUseWithDisplayMode(Bitmap* bitmap, bool make_opaque = false)
{
    const int bmp_col_depth = bitmap->GetColorDepth();
    const int game_col_depth = game.GetColorDepth();
    const int compat_col_depth = gfxDriver->GetCompatibleBitmapFormat(game_col_depth);

    const bool must_switch_palette = bitmap->GetColorDepth() == 8 && game_col_depth > 8;
    if (must_switch_palette)
        select_palette(palette);

    Bitmap *new_bitmap = bitmap;

    //
    // The only special case when bitmap needs to be prepared for graphics driver
    //
    // In 32-bit display mode, 32-bit bitmaps may require component conversion
    // to match graphics driver expectation about pixel format.
    // TODO: make GetCompatibleBitmapFormat tell this somehow
#if defined (AGS_INVERTED_COLOR_ORDER)
    const int sys_col_depth = System_GetColorDepth();
    if (sys_col_depth > 16 && bmp_col_depth == 32)
    {
        // Convert RGB to BGR.
        new_bitmap = convert_32_to_32bgr(bitmap);
    }
#endif

    //
    // The rest is about bringing bitmaps to the native game's format
    // (has no dependency on display mode).
    //
    // In 32-bit game 32-bit bitmaps should have transparent pixels marked
    // (this adjustment is probably needed for DrawingSurface ops)
    if ((game_col_depth == 32) && (bmp_col_depth == 32))
    {
        // TODO: find out if this may be removed at some point
        if (make_opaque)
            BitmapHelper::MakeOpaque(new_bitmap);
        else
            BitmapHelper::ReplaceAlphaWithRGBMask(new_bitmap);
    }
    // In 32-bit game hicolor bitmaps must be converted to the true color
    else if (game_col_depth == 32 && (bmp_col_depth > 8 && bmp_col_depth <= 16))
    { // FIXME: optimize by passing a "how to copy pixels" function into CreateCopy
        new_bitmap = BitmapHelper::CreateBitmapCopy(bitmap, compat_col_depth);
        BitmapHelper::MakeOpaqueSkipMask(new_bitmap);
    }
    // In non-32-bit game truecolor bitmaps must be downgraded
    else if ((game_col_depth <= 16) && (bmp_col_depth > 16))
    {
        // convert alpha channel to a 8/16-bit transparency mask
        new_bitmap = remove_alpha_channel(bitmap);
    }
    
    // Finally, if we did not create a new copy already, - convert to driver compatible format
    if (new_bitmap == bitmap)
        new_bitmap = GfxUtil::ConvertBitmap(bitmap, gfxDriver->GetCompatibleBitmapFormat(bitmap->GetColorDepth()));

    if (must_switch_palette)
        unselect_palette();

    return new_bitmap;
}

Bitmap *CreateCompatBitmap(int width, int height, int col_depth)
{
    return new Bitmap(width, height,
        gfxDriver->GetCompatibleBitmapFormat(col_depth == 0 ? game.GetColorDepth() : col_depth));
}

Bitmap *ReplaceBitmapWithSupportedFormat(Bitmap *bitmap)
{
    return GfxUtil::ConvertBitmap(bitmap, gfxDriver->GetCompatibleBitmapFormat(bitmap->GetColorDepth()));
}

Bitmap *PrepareSpriteForUse(Bitmap* bitmap, bool make_opaque)
{
    Bitmap *new_bitmap = AdjustBitmapForUseWithDisplayMode(bitmap, make_opaque);
    if (new_bitmap != bitmap)
        delete bitmap;
    return new_bitmap;
}

PBitmap PrepareSpriteForUse(PBitmap bitmap, bool make_opaque)
{
    Bitmap *new_bitmap = AdjustBitmapForUseWithDisplayMode(bitmap.get(), make_opaque);
    return new_bitmap == bitmap.get() ? bitmap : PBitmap(new_bitmap); // if bitmap is same, don't create new smart ptr!
}

Bitmap *CopyScreenIntoBitmap(int width, int height, bool at_native_res)
{
    Bitmap *dst = new Bitmap(width, height, game.GetColorDepth());
    GraphicResolution want_fmt;
    // If the size and color depth are supported we may copy right into our bitmap
    if (gfxDriver->GetCopyOfScreenIntoBitmap(dst, at_native_res, &want_fmt))
        return dst;
    // Otherwise we might need to copy between few bitmaps...
    Bitmap *buf_screenfmt = new Bitmap(want_fmt.Width, want_fmt.Height, want_fmt.ColorDepth);
    gfxDriver->GetCopyOfScreenIntoBitmap(buf_screenfmt, at_native_res);
    // If at least size matches then we may blit
    if (dst->GetSize() == buf_screenfmt->GetSize())
    {
        dst->Blit(buf_screenfmt);
    }
    // Otherwise we need to go through another bitmap of the matching format
    else
    {
        Bitmap *buf_dstfmt = new Bitmap(buf_screenfmt->GetWidth(), buf_screenfmt->GetHeight(), dst->GetColorDepth());
        buf_dstfmt->Blit(buf_screenfmt);
        dst->StretchBlt(buf_dstfmt, RectWH(dst->GetSize()));
        delete buf_dstfmt;
    }
    delete buf_screenfmt;
    return dst;
}

void create_blank_image(int coldepth)
{
    // this is the first time that we try to use the graphics driver,
    // so it's the most likey place for a crash
    try
    {
        Bitmap *blank = CreateCompatBitmap(16, 16, coldepth);
        blank->Clear();
        blankImage = gfxDriver->CreateDDBFromBitmap(blank, true /*opaque*/);
        blankSidebarImage = gfxDriver->CreateDDBFromBitmap(blank, true /*opaque*/);
        delete blank;
    }
    catch (Ali3DException gfxException)
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
    color_t real_color = 0;
    __my_setcolor(&real_color, color_index, game.GetColorDepth());
    return real_color;
}

void init_draw_method()
{
    drawstate.SoftwareRender = !gfxDriver->HasAcceleratedTransform();
    drawstate.FullFrameRedraw = gfxDriver->RequiresFullRedrawEachFrame();

    if (drawstate.SoftwareRender)
    {
        drawstate.SoftwareRender = true;
        drawstate.WalkBehindMethod = DrawOverCharSprite;
    }
    else
    {
        drawstate.WalkBehindMethod = DrawAsSeparateSprite;
        create_blank_image(game.GetColorDepth());
        size_t tx_cache_size = usetup.TextureCacheSize * 1024;
        // If graphics driver can report available texture memory,
        // then limit the setting by, let's say, 66% of it (we use it for other things)
        size_t avail_tx_mem = gfxDriver->GetAvailableTextureMemory();
        if (avail_tx_mem > 0)
            tx_cache_size = std::min<size_t>(tx_cache_size, avail_tx_mem * 0.66);
        texturecache.SetMaxCacheSize(tx_cache_size);
        Debug::Printf("Texture cache set: %zu KB", tx_cache_size / 1024);
    }

    on_mainviewport_changed();
    init_room_drawdata();
    if (gfxDriver->UsesMemoryBackBuffer())
        gfxDriver->GetMemoryBackBuffer()->Clear();

    play.spritemodified.resize(game.SpriteInfos.size());
}

void dispose_draw_method()
{
    dispose_room_drawdata();
    dispose_invalid_regions(false);
    destroy_blank_image();
}

void init_game_drawdata()
{
    // character and object caches
    charcache.resize(game.numcharacters);
    for (int i = 0; i < MAX_ROOM_OBJECTS; ++i)
        objcache[i] = ObjectCache();

    size_t actsps_num = game.numcharacters + MAX_ROOM_OBJECTS;
    actsps.resize(actsps_num);
    guihelpbg.resize(game.numgui);
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
}

void dispose_game_drawdata()
{
    clear_drawobj_cache();

    charcache.clear();
    actsps.clear();
    walkbehindobj.clear();
    guihelpbg.clear();
    texturecache_clear();
    guibg.clear();
    gui_render_tex.clear();
    guiobjbg.clear();
    guiobjddbref.clear();
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
    for (auto &hbg : guihelpbg) hbg.reset();
    overtxs.clear();

    // Clear "modified sprite" flags
    play.spritemodifiedlist.clear();
    std::fill(play.spritemodified.begin(), play.spritemodified.end(), false);

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
    const bool has_rotation = view->GetCamera()->GetRotation() != 0.f;
    RoomCameraDrawData &draw_dat = CameraDrawData[view_index];
    // We use intermediate bitmap to render camera/viewport pair in software mode under these conditions:
    // * camera size and viewport size are different (this may be suboptimal to paint dirty rects stretched,
    //   and also Allegro backend cannot stretch background of different colour depth).
    // * viewport is located outside of the virtual screen (even if partially): subbitmaps cannot contain
    //   regions outside of master bitmap, and we must not clamp surface size to virtual screen because
    //   plugins may want to also use viewport bitmap, therefore it should retain full size.
    if (cam_sz == view_sz && !draw_dat.IsOffscreen && !has_rotation)
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
            Size alloc_sz = Size::Clamp(cam_sz * 2, Size(1, 1), Size(thisroom.Width, thisroom.Height));
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
}

void on_roomviewport_created(int index)
{
    if (!gfxDriver || drawstate.FullFrameRedraw)
        return;
    if ((size_t)index < CameraDrawData.size())
        return;
    CameraDrawData.resize(index + 1);
}

void on_roomviewport_deleted(int index)
{
    if (drawstate.FullFrameRedraw)
        return;
    CameraDrawData.erase(CameraDrawData.begin() + index);
    delete_invalid_regions(index);
}

void on_roomviewport_changed(Viewport *view)
{
    if (drawstate.FullFrameRedraw)
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
    if (drawstate.FullFrameRedraw)
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
    if (drawstate.FullFrameRedraw)
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
    assert(sprnum >= 0 && sprnum < game.SpriteInfos.size());
    // Update texture cache (regen texture or clear from cache)
    if (deleted)
        clear_shared_texture(sprnum);
    else
        update_shared_texture(sprnum);

    // For texture-based renderers updating a shared texture will already
    // update all the related drawn objects on screen.
    // For software renderer we should notify drawables that currently
    // reference this sprite.
    if (drawstate.SoftwareRender)
    {
        assert(sprnum < play.spritemodified.size());
        play.spritemodified[sprnum] = true;
        play.spritemodifiedlist.push_back(sprnum);
    }
}

void texturecache_get_state(size_t &max_size, size_t &cur_size, size_t &locked_size, size_t &ext_size)
{
    max_size = texturecache.GetMaxCacheSize();
    cur_size = texturecache.GetCacheSize();
    locked_size = texturecache.GetLockedSize();
    ext_size = texturecache.GetExternalSize();
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
        gfxDriver->UpdateTexture(txdata.get(), spriteset[sprite_id], false);
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

void mark_screen_dirty()
{
    drawstate.ScreenIsDirty = true;
}

bool is_screen_dirty()
{
    return drawstate.ScreenIsDirty;
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
        y1 + get_font_height_outlined(font) + 1, false);
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
    // Stage: final plugin callback (still drawn on game screen
    if (pl_any_want_hook(AGSE_FINALSCREENDRAW))
    {
        gfxDriver->BeginSpriteBatch(play.GetMainViewport(),
            play.GetGlobalTransform(drawstate.FullFrameRedraw), (GraphicFlip)play.screen_flipped);
        gfxDriver->DrawSprite(AGSE_FINALSCREENDRAW, 0, nullptr);
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
        if (new_vsync != scsystem.vsync)
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
        catch (Ali3DFullscreenLostException e) 
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

void draw_game_screen_callback()
{
    construct_game_scene(true);
    construct_game_screen_overlay(false);
}

void putpixel_compensate (Bitmap *ds, int xx,int yy, int col) {
    if ((ds->GetColorDepth() == 32) && (col != 0)) {
        // ensure the alpha channel is preserved if it has one
        int alphaval = geta32(ds->GetPixel(xx, yy));
        col = makeacol32(getr32(col), getg32(col), getb32(col), alphaval);
    }
    ds->FillRect(Rect(xx, yy, xx, yy), col);
}

void draw_sprite_support_alpha(Bitmap *ds, int xpos, int ypos, Bitmap *image,
                               BlendMode blend_mode, int alpha)
{
    if (alpha <= 0)
        return;

    GfxUtil::DrawSpriteBlend(ds, Point(xpos, ypos), image, blend_mode, alpha);
}

void draw_sprite_slot_support_alpha(Bitmap *ds, int xpos, int ypos, int src_slot,
                                    BlendMode blend_mode, int alpha)
{
    draw_sprite_support_alpha(ds, xpos, ypos, spriteset[src_slot], blend_mode, alpha);
}

IDriverDependantBitmap* recycle_ddb_bitmap(IDriverDependantBitmap *ddb,
    Common::Bitmap *source, bool opaque)
{
    assert(source);
    if (ddb && (drawstate.SoftwareRender || (ddb->GetColorDepth() == source->GetColorDepth()) &&
            (ddb->GetWidth() == source->GetWidth()) && (ddb->GetHeight() == source->GetHeight())))
        gfxDriver->UpdateDDBFromBitmap(ddb, source);
    else if (ddb)
        ddb->AttachData(std::shared_ptr<Texture>(gfxDriver->CreateTexture(source, opaque)), opaque);
    else
        ddb = gfxDriver->CreateDDBFromBitmap(source, opaque);
    return ddb;
}

IDriverDependantBitmap* recycle_ddb_sprite(IDriverDependantBitmap *ddb, uint32_t sprite_id,
    Common::Bitmap *source, bool opaque)
{
    // If sprite_id is not cachable, then fallback to a simpler variant
    if (drawstate.SoftwareRender || sprite_id == UINT32_MAX)
    {
        if (!source && (sprite_id < UINT32_MAX))
            source = spriteset[sprite_id];
        return recycle_ddb_bitmap(ddb, source, opaque);
    }

    if (ddb && ddb->GetRefID() == sprite_id)
        return ddb; // texture in sync

    auto txdata = texturecache.GetOrLoad(sprite_id, source, opaque);
    if (!txdata)
    {
        // On failure - invalidate ddb (we don't want to draw old pixels)
        if (ddb)
            ddb->DetachData();
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

// FIXME: make opaque a property of ObjTexture?!
static void sync_object_texture(ObjTexture &obj, bool opaque = false)
{
    obj.Ddb = recycle_ddb_sprite(obj.Ddb, obj.SpriteID, obj.Bmp.get(), opaque);
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
    SpriteListEntry sprite;
    sprite.ddb = ddb;
    sprite.x = x;
    sprite.y = y;
    sprite.aabb = RectWH(x, y, ddb->GetWidth(), ddb->GetHeight());
    thingsToDrawList.push_back(sprite);
}

static void add_render_stage(int stage)
{
    SpriteListEntry sprite;
    sprite.renderStage = stage;
    thingsToDrawList.push_back(sprite);
}

static void clear_sprite_list()
{
    sprlist.clear();
}

static void add_to_sprite_list(IDriverDependantBitmap* ddb, int x, int y, const Rect &aabb,
    int zorder, bool isWalkBehind, int id = -1)
{
    assert(ddb);
    // completely invisible, so don't draw it at all
    if (ddb->GetAlpha() == 0)
        return;

    SpriteListEntry sprite;
    sprite.id = id;
    sprite.ddb = ddb;
    sprite.zorder = zorder;
    sprite.x = x;
    sprite.y = y;
    sprite.aabb = aabb;

    if (drawstate.WalkBehindMethod == DrawAsSeparateSprite)
        sprite.takesPriorityIfEqual = !isWalkBehind;
    else
        sprite.takesPriorityIfEqual = isWalkBehind;

    sprlist.push_back(sprite);
}

static void add_to_sprite_list(IDriverDependantBitmap* spp, int xx, int yy, int zorder, bool isWalkBehind)
{
    add_to_sprite_list(spp, xx, yy, RectWH(xx, yy, spp->GetWidth(), spp->GetHeight()), zorder, isWalkBehind);
}

// Sprite drawing order sorting function,
// where equal zorder is resolved by comparing optional IDs too.
static bool spritelistentry_less(const SpriteListEntry &e1, const SpriteListEntry &e2)
{
    return (e1.zorder < e2.zorder) ||
        (e1.zorder == e2.zorder) && (e1.id < e2.id);
}

// Room-specialized function to sort the sprites into baseline order;
// does not account for IDs, but has special handling for walk-behinds.
static bool spritelistentry_room_less(const SpriteListEntry &e1, const SpriteListEntry &e2)
{
    if (e1.zorder == e2.zorder)
    {
        if (e1.takesPriorityIfEqual)
            return false;
        if (e2.takesPriorityIfEqual)
            return true;
    }
    return e1.zorder < e2.zorder;
}

// copy the sorted sprites into the Things To Draw list
static void draw_sprite_list(bool is_room)
{
    std::sort(sprlist.begin(), sprlist.end(), is_room ? spritelistentry_room_less : spritelistentry_less);
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
void draw_gui_sprite(Bitmap *ds, int pic, int x, int y, BlendMode blend_mode)
{
    draw_gui_sprite(ds, x, y, spriteset[pic], blend_mode);
}

void draw_gui_sprite(Bitmap *ds, int x, int y, Bitmap *sprite, BlendMode blend_mode, int alpha)
{
    if (alpha <= 0)
        return;

    const bool use_alpha = (ds->GetColorDepth() == 32);
    if (use_alpha)
    {
        GfxUtil::DrawSpriteBlend(ds, Point(x, y), sprite, blend_mode, alpha);
    }
    else
    {
        GfxUtil::DrawSpriteWithTransparency(ds, sprite, x, y, alpha);
    }
}

void draw_gui_sprite_flipped(Bitmap *ds, int pic, int x, int y, BlendMode blend_mode, bool is_flipped)
{
    draw_gui_sprite_flipped(ds, x, y, spriteset[pic],
        blend_mode, 0xFF, is_flipped);
}

void draw_gui_sprite_flipped(Bitmap *ds, int x, int y, Bitmap *sprite,
    BlendMode blend_mode, int alpha, bool is_flipped)
{
    if (alpha <= 0)
        return;

    std::unique_ptr<Bitmap> tempspr;
    if (is_flipped) {
        tempspr.reset(new Bitmap(sprite->GetWidth(), sprite->GetHeight(), sprite->GetColorDepth()));
        tempspr->ClearTransparent();
        tempspr->FlipBlt(sprite, 0, 0, Common::kFlip_Horizontal);
        sprite = tempspr.get();
    }

    if (ds->GetColorDepth() == 32) // ds has alpha?
    {
        GfxUtil::DrawSpriteBlend(ds, Point(x, y), sprite, blend_mode, alpha);
    }
    else
    {
        GfxUtil::DrawSpriteWithTransparency(ds, sprite, x, y, alpha);
    }
}

// Avoid freeing and reallocating the memory if possible
Bitmap *recycle_bitmap(Bitmap *bimp, int coldep, int wid, int hit, bool make_transparent) {
    if (bimp != nullptr) {
        // same colour depth, width and height -> reuse
        if ((bimp->GetColorDepth() == coldep) && (bimp->GetWidth() == wid)
                && (bimp->GetHeight() == hit))
        {
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

// Allocates texture for the GUI
void recreate_drawobj_bitmap(Bitmap *&raw, IDriverDependantBitmap *&ddb, int width, int height, int rot_degrees)
{
    // Calculate all supported GUI transforms
    Size final_sz = drawstate.SoftwareRender ?
        RotateSize(Size(width, height), rot_degrees) :
        Size(width, height);

    if (raw && raw->GetSize() == final_sz)
        return; // all is fine
    delete raw;
    raw = CreateCompatBitmap(final_sz.Width, final_sz.Height);
    if (ddb != nullptr)
    {
        gfxDriver->DestroyDDB(ddb);
        ddb = nullptr;
    }
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

// FIXME: refactor this function in similar way to how it was done in ags3 branch:
// split transform_sprite out.
// transform_sprite - should contain most of the scale_and_flip_sprite.
// find ags3 variants of transform_sprite and scale_and_flip_sprite in code below.
//
// Draws the specified 'sppic' sprite onto actsps[useindx] at the
// specified width and height, and flips the sprite if necessary.
// Returns 1 if something was drawn to actsps; returns 0 if no
// scaling or stretching was required, in which case nothing was done
bool scale_and_flip_sprite(ObjTexture &actsp, int sppic, int newwidth, int newheight,
                          float rotation, bool isMirrored)
{
  bool actsps_used = true;

  Bitmap *src_sprite = spriteset[sppic];
  Bitmap *temp_rot = nullptr;
  const int coldept = src_sprite->GetColorDepth();
  const bool scaled = src_sprite->GetSize() != Size(newwidth, newheight);

  if (rotation != 0.f) { 
      Size rot_sz = RotateSize(Size(newwidth, newheight), rotation);
      newwidth = rot_sz.Width;
      newheight = rot_sz.Height;

      // TODO: allegro does not provide a ready function to combine stretch & rotate
      // (only rotate & scale, but scale is a uniform factor, so not dst_width/dst_height),
      // so we create another intermediate bitmap for rotation here...
      // might investigate methods for optimizing this later.
      if (scaled || isMirrored) {
          Size src_sz = Size(src_sprite->GetWidth(), src_sprite->GetHeight());
          Size rot_sz = RotateSize(src_sz, rotation);
          temp_rot = BitmapHelper::CreateTransparentBitmap(rot_sz.Width, rot_sz.Height, coldept);
          if (isMirrored) rotation = -rotation;
          // (+ width%2 fixes one pixel offset problem)
          temp_rot->RotateBlt(src_sprite, rot_sz.Width / 2 + rot_sz.Width % 2, rot_sz.Height / 2,
              src_sz.Width / 2, src_sz.Height / 2, rotation); // clockwise
          src_sprite = temp_rot;
      }
  }

  // create and blank out the new sprite
  recycle_bitmap(actsp.Bmp, coldept, newwidth, newheight, true);
  Bitmap *active_spr = actsp.Bmp.get();

  if (scaled) {
      // Scaled character

      our_eip = 334;

      // Ensure that anti-aliasing routines have a palette to
      // use for mapping while faded out
      if (in_new_room)
          select_palette (palette);


      if (isMirrored) {
          // TODO: "flip self" function may allow to optimize this
          Bitmap *tempspr = BitmapHelper::CreateTransparentBitmap(newwidth, newheight, coldept);
          if ((IS_ANTIALIAS_SPRITES) && (coldept < 32))
              tempspr->AAStretchBlt (src_sprite, RectWH(0, 0, newwidth, newheight), Common::kBitmap_Transparency);
          else
              tempspr->StretchBlt (src_sprite, RectWH(0, 0, newwidth, newheight), Common::kBitmap_Transparency);
          active_spr->FlipBlt(tempspr, 0, 0, Common::kFlip_Horizontal);
          delete tempspr;
      }
      else if ((IS_ANTIALIAS_SPRITES) && (coldept < 32))
          active_spr->AAStretchBlt(src_sprite,RectWH(0,0,newwidth,newheight), Common::kBitmap_Transparency);
      else
          active_spr->StretchBlt(src_sprite,RectWH(0,0,newwidth,newheight), Common::kBitmap_Transparency);
      if (in_new_room)
          unselect_palette();

  } 
  else {
      // Not a scaled character, draw at normal size

      our_eip = 339;

      if (isMirrored)
          active_spr->FlipBlt(src_sprite, 0, 0, Common::kFlip_Horizontal);
      else if (rotation != 0.f)
          // (+ width%2 fixes one pixel offset problem)
          active_spr->RotateBlt(src_sprite, newwidth / 2 + newwidth % 2, newheight / 2,
              src_sprite->GetWidth() / 2, src_sprite->GetHeight() / 2, rotation); // clockwise
      else
          actsps_used = false; // can use original sprite
  }
  delete temp_rot;

  return actsps_used;
}

// Generates a transformed sprite, using src image and parameters;
// * if transformation is necessary - writes into dst and returns dst;
// * if no transformation is necessary - simply returns src;
// Used for software render mode only.
static Bitmap *transform_sprite(Bitmap *src, std::unique_ptr<Bitmap> &dst,
    const Size dst_sz, GraphicFlip flip = Common::kFlip_None)
{
    if ((src->GetSize() == dst_sz) && (flip == kFlip_None))
        return src; // No transform: return source image

    recycle_bitmap(dst, src->GetColorDepth(), dst_sz.Width, dst_sz.Height, true);
    our_eip = 339;

    // If scaled: first scale then optionally mirror
    if (src->GetSize() != dst_sz)
    {
        // 8-bit support: ensure that anti-aliasing routines have a palette
        // to use for mapping while faded out.
        // TODO: find out if this may be moved out and not repeated?
        if (in_new_room > 0)
            select_palette(palette);

        if (flip != kFlip_None)
        {
            Bitmap tempbmp;
            tempbmp.CreateTransparent(dst_sz.Width, dst_sz.Height, src->GetColorDepth());
            if ((IS_ANTIALIAS_SPRITES) && (src->GetColorDepth() < 32))
                tempbmp.AAStretchBlt(src, RectWH(dst_sz), kBitmap_Transparency);
            else
                tempbmp.StretchBlt(src, RectWH(dst_sz), kBitmap_Transparency);
            dst->FlipBlt(&tempbmp, 0, 0, kFlip_Horizontal);
        }
        else
        {
            if ((IS_ANTIALIAS_SPRITES) && (src->GetColorDepth() < 32))
                dst->AAStretchBlt(src, RectWH(dst_sz), kBitmap_Transparency);
            else
                dst->StretchBlt(src, RectWH(dst_sz), kBitmap_Transparency);
        }

        if (in_new_room > 0)
            unselect_palette();
    }
    else
    {
        // If not scaled, then simply blit mirrored
        dst->FlipBlt(src, 0, 0, kFlip_Horizontal);
    }
    return dst.get(); // return transformed result
}

// Draws the specified 'sppic' sprite onto ObjTexture 'actsp' at the
// specified width and height, and flips the sprite if necessary.
// Returns 1 if something was drawn to actsps; returns 0 if no
// scaling or stretching was required, in which case nothing was done.
// Used for software render mode only.
static bool scale_and_flip_sprite(ObjTexture &actsp, int sppic, int width, int height, bool hmirror)
{
    Bitmap *src = spriteset[sppic];
    Bitmap *result = transform_sprite(src, actsp.Bmp, Size(width, height),
        hmirror ? kFlip_Horizontal : kFlip_None);
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
        const bool is_texture_intact = objsav.sppic == specialpic;
        objsav.sppic = specialpic;
        objsav.tintamnt = tint_level;
        objsav.tintr = tint_red;
        objsav.tintg = tint_green;
        objsav.tintb = tint_blue;
        objsav.tintlight = tint_light;
        objsav.lightlev = light_level;
        objsav.zoom = objsrc.zoom;
        objsav.rotation = objsrc.rotation;
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
        (!play.spritemodified[objsav.sppic]) &&
        (objsav.tintamnt == tint_level) &&
        (objsav.tintlight == tint_light) &&
        (objsav.tintr == tint_red) &&
        (objsav.tintg == tint_green) &&
        (objsav.tintb == tint_blue) &&
        (objsav.lightlev == light_level) &&
        (objsav.zoom == objsrc.zoom) &&
        (objsav.rotation == objsrc.rotation) &&
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
    // draw the base sprite, scaled and flipped as appropriate
    bool actsps_used = scale_and_flip_sprite(actsp, pic, scale_size.Width, scale_size.Height, objsrc.rotation, is_mirrored);
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
    recycle_bitmap(objsav.image, actsp.Bmp->GetColorDepth(), actsp.Bmp->GetWidth(), actsp.Bmp->GetHeight());
    objsav.image->Blit(actsp.Bmp.get(), 0, 0);
    objsav.in_use = true;
    objsav.sppic = specialpic;
    objsav.tintamnt = tint_level;
    objsav.tintr = tint_red;
    objsav.tintg = tint_green;
    objsav.tintb = tint_blue;
    objsav.tintlight = tint_light;
    objsav.lightlev = light_level;
    objsav.zoom = objsrc.zoom;
    objsav.rotation = objsrc.rotation;
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
    Pointf origin, int transparency, BlendMode blend_mode, bool hw_accel)
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
        sync_object_texture(actsp);
    }

    // Now when we have a ready texture, assign texture properties
    // (transform, effects, and so forth)
    actsp.Ddb->SetOrigin(origin.X, origin.Y);
    if (hw_accel)
    {
        actsp.Ddb->SetStretch(scale_size.Width, scale_size.Height);
        actsp.Ddb->SetRotation(objsav.rotation);
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
    actsp.Ddb->SetBlendMode(blend_mode);
}

// Prepares a actsps element for RoomObject; updates object cache.
// Software mode draws an actual bitmap in actsp, hardware mode only
// assigns parameters, which may further be assigned to a texture.
bool construct_object_gfx(int objid, bool force_software)
{
    const RoomObject &obj = objs[objid];
    if (!spriteset.DoesSpriteExist(obj.num))
        quitprintf("There was an error drawing object %d. Its current sprite, %d, is invalid.", objid, obj.num);

    ObjectCache objsrc(obj.num, obj.tint_r, obj.tint_g, obj.tint_b,
        obj.tint_level, obj.tint_light, 0 /* skip */, obj.zoom, obj.rotation,
        false /* skip */, obj.x, obj.y);

    return construct_object_gfx(
        (obj.view != UINT16_MAX) ? &views[obj.view].loops[obj.loop].frames[obj.frame] : nullptr,
        obj.num,
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
    our_eip=32;
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

        // we must use actual image's top-left position here
        const Rect &aabb = obj.GetGraphicSpace().AABB();
        const int imgx = aabb.Left;
        const int imgy = aabb.Top;
        int usebasel = obj.get_baseline();

        // Generate raw bitmap in ObjTexture and store parameters in ObjectCache.
        const bool actsp_modified = !construct_object_gfx(objid, false);
        // Prepare the object texture
        prepare_and_add_object_gfx(objsav, actsp, actsp_modified,
            Size(obj.last_width, obj.last_height), imgx, imgy, usebasel,
            (obj.flags & OBJF_NOWALKBEHINDS) == 0,
            obj.GetOrigin(), obj.transparent, obj.blend_mode, hw_accel);
        // Finally, add the texture to the draw list
        add_to_sprite_list(actsp.Ddb, obj.x, obj.y, aabb, usebasel, false);
    }
}



// Draws srcimg onto destimg, tinting to the specified level
// Totally overwrites the contents of the destination image
void tint_image (Bitmap *ds, Bitmap *srcimg, int red, int grn, int blu, int light_level, int luminance) {

    if ((srcimg->GetColorDepth() != ds->GetColorDepth()) ||
        (srcimg->GetColorDepth() <= 8)) {
            debug_script_warn("Image tint failed - images must both be hi-color");
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
    const int pic = vf->pic;
    if (!spriteset.DoesSpriteExist(pic))
        quitprintf("There was an error drawing character %d. Its current frame's sprite, %d, is invalid.", charid, pic);

    ObjectCache chsrc(pic, chex.tint_r, chex.tint_g, chex.tint_b,
        chex.tint_level, chex.tint_light, 0 /* skip */, chex.zoom, chex.rotation,
        false /* skip */, chin.x, chin.y);

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
    our_eip=33;
    const bool hw_accel = !drawstate.SoftwareRender;

    // draw characters
    for (uint32_t charid = 0; charid < game.numcharacters; ++charid)
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

        // we must use actual image's top-left position here
        const Rect &aabb = chex.GetGraphicSpace().AABB();
        const int imgx = aabb.Left;
        const int imgy = aabb.Top;
        int usebasel = chin.get_baseline();

        // Generate raw bitmap in ObjTexture and store parameters in ObjectCache.
        const bool actsp_modified = !construct_char_gfx(charid, false);
        // Prepare the object texture
        prepare_and_add_object_gfx(chsav, actsp, actsp_modified,
            Size(chex.width, chex.height), imgx, imgy, usebasel,
            (chin.flags & CHF_NOWALKBEHINDS) == 0,
            chex.GetOrigin(), chin.transparency, chex.blend_mode, hw_accel);
        // Finally, add the texture to the draw list
        // CHECKME: remind why do we have to recalculate charx/y instead of using GS?
        const int charx = chin.x + chin.pic_xoffs * chex.zoom_offs / 100;
        const int chary = chin.y - chin.z + chin.pic_yoffs * chex.zoom_offs / 100;
        add_to_sprite_list(actsp.Ddb, charx, chary, aabb, usebasel, false);
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
    auto &overs = get_overlays();
    for (auto &over : overs)
    {
        if (over.type < 0) continue; // empty slot
        if (!over.IsRoomLayer()) continue; // not a room layer
        if (over.transparency == 255) continue; // skip fully transparent
        const Point pos = update_overlay_graphicspace(over);
        add_to_sprite_list(overtxs[over.type].Ddb, pos.X, pos.Y, over._gs.AABB(), over.zorder, false);
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
            recycle_ddb_bitmap(roomBackgroundBmp, thisroom.BgFrames[play.bg_frame].Graphic.get(), true /*opaque*/);
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
            our_eip = 34;

            if (drawstate.WalkBehindMethod == DrawAsSeparateSprite)
            {
                for (size_t wb = 1 /* 0 is "no area" */;
                    (wb < MAX_WALK_BEHINDS) && (wb < walkbehindobj.size()); ++wb)
                {
                    const auto &wbobj = walkbehindobj[wb];
                    if (wbobj.Ddb)
                    {
                        add_to_sprite_list(wbobj.Ddb, wbobj.Pos.X, wbobj.Pos.Y,
                            croom->walkbehind_base[wb], true);
                    }
                }
            }

            if (pl_any_want_hook(AGSE_PRESCREENDRAW))
                add_render_stage(AGSE_PRESCREENDRAW);

            draw_sprite_list(true);
        }
    }
    our_eip = 36;

    // Debug room overlay
    update_room_debug();
    if ((debugRoomMask != kRoomAreaNone) && debugRoomMaskObj.Ddb)
        add_thing_to_draw(debugRoomMaskObj.Ddb, 0, 0);
    if ((debugMoveListChar >= 0) && debugMoveListObj.Ddb)
        add_thing_to_draw(debugMoveListObj.Ddb, 0, 0);

    if (pl_any_want_hook(AGSE_POSTROOMDRAW))
        add_render_stage(AGSE_POSTROOMDRAW);
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
    our_eip = 31;

    // For the sake of software renderer, if there is any kind of camera transform required
    // except screen offset, we tell it to draw on separate bitmap first with zero transformation.
    // There are few reasons for this, primary is that Allegro does not support StretchBlt
    // between different colour depths (i.e. it won't correctly stretch blit 16-bit rooms to
    // 32-bit virtual screen).
    // Also see comment to ALSoftwareGraphicsDriver::RenderToBackBuffer().
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


void draw_fps(const Rect &viewport)
{
    // TODO: make allocated "fps struct" instead of using static vars!!
    static IDriverDependantBitmap* ddb = nullptr;
    static Bitmap *fpsDisplay = nullptr;
    const int font = FONT_NORMAL;
    if (fpsDisplay == nullptr)
    {
        fpsDisplay = CreateCompatBitmap(viewport.GetWidth(), (get_font_surface_height(font) + 5));
    }
    fpsDisplay->ClearTransparent();
    
    color_t text_color = fpsDisplay->GetCompatibleColor(14);

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
    wouttext_outline(fpsDisplay, 1, 1, font, text_color, fps_buffer);

    char loop_buffer[60];
    snprintf(loop_buffer, sizeof(loop_buffer), "Loop %u", loopcounter);
    wouttext_outline(fpsDisplay, viewport.GetWidth() / 2, 1, font, text_color, loop_buffer);

    if (ddb)
        gfxDriver->UpdateDDBFromBitmap(ddb, fpsDisplay);
    else
        ddb = gfxDriver->CreateDDBFromBitmap(fpsDisplay);
    int yp = viewport.GetHeight() - fpsDisplay->GetHeight();
    gfxDriver->DrawSprite(1, yp, ddb);
    invalidate_sprite_glob(1, yp, ddb);
}

// Draw GUI controls as separate sprites, each on their own texture
static void construct_guictrl_tex(GUIMain &gui)
{
    if (all_buttons_disabled && (GUI::Options.DisabledStyle == kGuiDis_Blackout))
        return; // don't draw GUI controls

    int draw_index = guiobjddbref[gui.ID];
    for (int i = 0; i < gui.GetControlCount(); ++i, ++draw_index)
    {
        GUIObject *obj = gui.GetControl(i);
        if (!obj->IsVisible() ||
            (obj->GetSize().IsNull()) ||
            (!obj->IsEnabled() && (GUI::Options.DisabledStyle == kGuiDis_Blackout)))
            continue;
        if (!obj->HasChanged())
            continue;

        auto &objbg = guiobjbg[draw_index];
        Rect obj_surf = obj->CalcGraphicRect(GUI::Options.ClipControls);
        recycle_bitmap(objbg.Bmp, game.GetColorDepth(), obj_surf.GetWidth(), obj_surf.GetHeight(), true);
        obj->Draw(objbg.Bmp.get(), -obj_surf.Left, -obj_surf.Top);

        sync_object_texture(objbg);
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
    // Add all the GUI controls
    const int draw_index = guiobjddbref[gui_id];
    for (const auto &obj_id : gui.GetControlsDrawOrder())
    {
        GUIObject *obj = gui.GetControl(obj_id);
        if (!obj->IsVisible() ||
            (obj->GetSize().IsNull()) ||
            (!obj->IsEnabled() && (GUI::Options.DisabledStyle == kGuiDis_Blackout)))
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
    if(pl_any_want_hook(AGSE_PREGUIDRAW))
        gfxDriver->DrawSprite(AGSE_PREGUIDRAW, 0, nullptr); // render stage

    clear_sprite_list();

    const bool is_3d_render = !drawstate.SoftwareRender;
    const bool draw_controls_as_textures = is_3d_render;

    // Add overlays
    auto &overs = get_overlays();
    for (auto &over : overs)
    {
        if (over.type < 0) continue; // empty slot
        if (over.IsRoomLayer()) continue; // not a ui layer
        if (over.transparency == 255) continue; // skip fully transparent
        const Point pos = update_overlay_graphicspace(over);
        add_to_sprite_list(overtxs[over.type].Ddb, pos.X, pos.Y, over._gs.AABB(), over.zorder, false);
    }

    // Add GUIs
    our_eip=35;
    if (((debug_flags & DBG_NOIFACE)==0) && (displayed_room >= 0)) {
        if (playerchar->activeinv >= MAX_INV) {
            quit("!The player.activeinv variable has been corrupted, probably as a result\n"
                "of an incorrect assignment in the game script.");
        }
        if (playerchar->activeinv < 1) gui_inv_pic=-1;
        else gui_inv_pic=game.invinfo[playerchar->activeinv].pic;
        our_eip = 37;
        // Prepare and update GUI textures
        {
            for (int index = 0; index < game.numgui; ++index)
            {
                auto &gui = guis[index];
                if (!gui.IsDisplayed()) continue; // not on screen
                if (!gui.HasChanged() && !gui.HasControlsChanged()) continue; // no changes: no need to update image
                if (gui.Transparency == 255) continue; // 100% transparent

                eip_guinum = index;
                our_eip = 372;
                Bitmap *guibg_final = guibg[index].Bmp.get();
                const bool draw_with_controls = !draw_controls_as_textures;
                if (gui.HasChanged() || (draw_with_controls && gui.HasControlsChanged()))
                {
                    auto &gbg = guibg[index];
                    Bitmap *bmp = gbg.Bmp.release();
                    recreate_drawobj_bitmap(bmp, gbg.Ddb, gui.Width, gui.Height, gui.Rotation);
                    gbg.Bmp.reset(bmp);
                    Bitmap *guibg_final = gbg.Bmp.get();
                    Bitmap *draw_at = guibg_final;
                    // For software drawing, if GUI requires visual transformation,
                    // then we first draw normal GUI on a helper surface, then blit
                    // that surface to the final bitmap
                    if (!is_3d_render && gui.Rotation != 0.f)
                    {
                        guihelpbg[index].reset(
                            recycle_bitmap(guihelpbg[index].release(), game.GetColorDepth(), gui.Width, gui.Height));
                        draw_at = guihelpbg[index].get();
                    }

                    draw_at->ClearTransparent();
                    if (draw_with_controls)
                        gui.DrawWithControls(draw_at);
                    else
                        gui.DrawSelf(draw_at);

                    if (draw_at != guibg_final)
                    {
                        guibg_final->ClearTransparent();
                        if (gui.Rotation != 0.f)
                        {
                            const int dst_w = guibg_final->GetWidth();
                            const int dst_h = guibg_final->GetHeight();
                            // (+ width%2 fixes one pixel offset problem)
                            guibg_final->RotateBlt(draw_at, dst_w / 2 + dst_w % 2, dst_h / 2,
                                gui.Width / 2, gui.Height / 2, gui.Rotation); // clockwise
                        }
                        else
                        {
                            guibg_final->StretchBlt(draw_at, RectWH(guibg_final->GetSize()));
                        }
                    }

                    IDriverDependantBitmap *&ddb = guibg[index].Ddb;
                    if (ddb != nullptr)
                    {
                        gfxDriver->UpdateDDBFromBitmap(ddb, guibg_final);
                    }
                    else
                    {
                        ddb = gfxDriver->CreateDDBFromBitmap(guibg_final);
                    }
                }
                
                our_eip = 373;
                // Update control textures, if they have changed themselves
                if (draw_controls_as_textures && gui.HasControlsChanged())
                {
                    construct_guictrl_tex(gui);
                }


                our_eip = 374;
                gui.ClearChanged();
            }
        }
        our_eip = 38;
        // Draw the GUIs
        for (int index = 0; index < game.numgui; ++index)
        {
            GUIMain &gui = guis[index];
            if (!gui.IsDisplayed()) continue; // not on screen
            if (gui.Transparency == 255) continue; // 100% transparent

            // Don't draw GUI if "GUIs Turn Off When Disabled"
            if ((game.options[OPT_DISABLEOFF] == kGuiDis_Off) &&
                (all_buttons_disabled >= 0) &&
                (gui.PopupStyle != kGUIPopupNoAutoRemove))
                continue;

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
            gui_ddb->SetBlendMode(gui.BlendMode);
            gui_ddb->SetOrigin(0.f, 0.f);
            gui_ddb->SetRotation(gui.Rotation);
            add_to_sprite_list(gui_ddb, gui.X, gui.Y,
                gui.GetGraphicSpace().AABB(), gui.ZOrder, false, index);
        }
    }

    // Move the resulting sprlist with guis and overlays to render
    draw_sprite_list(false);
    put_sprite_list_on_screen(false);
    our_eip = 1099;
}

// Push the gathered list of sprites into the active graphic renderer
void put_sprite_list_on_screen(bool in_room)
{
    for (const auto &t : thingsToDrawList)
    {
        assert(t.ddb || (t.renderStage >= 0));
        if (t.ddb)
        {
            if (t.ddb->GetAlpha() == 0)
                continue; // skip completely invisible things
            // mark the image's region as dirty
            invalidate_sprite(t.x, t.y, t.ddb, in_room);
            // push to the graphics driver
            gfxDriver->DrawSprite(t.x, t.y,
                t.aabb.Left, t.aabb.Top, t.ddb);
        }
        else if (t.renderStage >= 0)
        {
            // meta entry to run the plugin hook
            gfxDriver->DrawSprite(t.renderStage, 0, nullptr);
        }
    }

    our_eip = 1100;
}

bool GfxDriverSpriteEvtCallback(int evt, int data)
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
        const SpriteTransform cam_trans(-cam_rc.Left, -cam_rc.Top, 1.f, 1.f,
            camera->GetRotation(), Point(cam_rc.GetWidth() / 2, cam_rc.GetHeight() / 2));

        if (drawstate.FullFrameRedraw)
        {
            // For hw renderer we draw everything as a sprite stack;
            // viewport-camera pair is done as 2 nested scene nodes,
            // where first defines how camera's image translates into the viewport on screen,
            // and second - how room's image translates into the camera.
            gfxDriver->BeginSpriteBatch(view_rc, view_trans);
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
            gfxDriver->BeginSpriteBatch(view_rc, view_trans);

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
    gfxDriver->BeginSpriteBatch(play.GetUIViewport());
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
        bool has_changed = over.HasChanged() || play.spritemodified[over.GetSpriteNum()];
        // If walk behinds are drawn over the cached object sprite, then check if positions were updated
        if (crop_walkbehinds && over.IsRoomLayer())
        {
            Point pos = get_overlay_position(over);
            has_changed |= (pos.X != overcache[i].X || pos.Y != overcache[i].Y);
            overcache[i].X = pos.X; overcache[i].Y = pos.Y;
        }

        if (has_changed)
        {
            overtx.SpriteID = over.GetSpriteNum();
            // For software mode - prepare transformed bitmap if necessary;
            // for hardware-accelerated - use the sprite ID if possible, to avoid redundant sprite load
            // TODO: find a way to unify this code with the character & object ObjTexture preparation;
            // they use practically same approach, except of different fields cache.
            Bitmap *use_bmp = nullptr;
            if (is_software_mode)
            {
                use_bmp = transform_sprite(over.GetImage(), overtx.Bmp, Size(over.scaleWidth, over.scaleHeight));
                auto *bmp1 = overtx.Bmp.release();
                auto *bmp2 = overtx.Bmp2.release();
                use_bmp = recreate_overlay_image(over, bmp1, bmp2);
                overtx.Bmp.reset(bmp1);
                overtx.Bmp2.reset(bmp2);
                use_bmp = transform_sprite(over.GetImage(), overtx.Bmp, Size(over.scaleWidth, over.scaleHeight));
                if (crop_walkbehinds && over.IsRoomLayer())
                {
                    auto &use_cache = overtx.Bmp2;
                    if (use_bmp != use_cache.get())
                    {
                        recycle_bitmap(use_cache, use_bmp->GetColorDepth(), use_bmp->GetWidth(), use_bmp->GetHeight(), true);
                        use_cache->Blit(use_bmp);
                    }
                    Point pos = get_overlay_position(over);
                    walkbehinds_cropout(use_cache.get(), pos.X, pos.Y, over.zorder);
                    use_bmp = use_cache.get();
                }
            }

            sync_object_texture(overtx);
            over.ClearChanged();
        }

        assert(overtx.Ddb); // Test for missing texture, might happen if not marked for update
        if (!overtx.Ddb) continue;
        overtx.Ddb->SetStretch(over.scaleWidth, over.scaleHeight);
        overtx.Ddb->SetRotation(over.rotation);
        overtx.Ddb->SetAlpha(GfxDef::LegacyTrans255ToAlpha255(over.transparency));
        overtx.Ddb->SetBlendMode(over.blendMode);
    }
}

static void reset_spritemodified()
{
    if (play.spritemodifiedlist.size() > 0)
    {
        // Sort and remove duplicates;
        // CHECKME: or is it more optimal to just run over raw list?
        std::sort(play.spritemodifiedlist.begin(), play.spritemodifiedlist.end());
        play.spritemodifiedlist.erase(
            std::unique(play.spritemodifiedlist.begin(), play.spritemodifiedlist.end()), play.spritemodifiedlist.end());
        for (auto sprnum : play.spritemodifiedlist)
            play.spritemodified[sprnum] = false;
        play.spritemodifiedlist.clear();
    }
}

void construct_game_scene(bool full_redraw)
{
    gfxDriver->ClearDrawLists();

    if (play.fast_forward)
        return;

    our_eip=3;

    // React to changes to viewports and cameras (possibly from script) just before the render
    play.UpdateViewports();

    gfxDriver->UseSmoothScaling(IS_ANTIALIAS_SPRITES);
    gfxDriver->RenderSpritesAtScreenResolution(usetup.RenderAtScreenRes, usetup.Supersampling);

    pl_run_plugin_hooks(AGSE_PRERENDER, 0);

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

    our_eip=4;

    // Stage: UI overlay
    if (play.screen_is_faded_out == 0)
    {
        construct_ui_view();
    }

    // End the parent scene node
    gfxDriver->EndSpriteBatch();

    // Clear "modified sprite" flags
    reset_spritemodified();
}

void construct_game_screen_overlay(bool draw_mouse)
{
    gfxDriver->BeginSpriteBatch(play.GetMainViewport(),
            play.GetGlobalTransform(drawstate.FullFrameRedraw), (GraphicFlip)play.screen_flipped);
    if (pl_any_want_hook(AGSE_POSTSCREENDRAW))
    {
        gfxDriver->DrawSprite(AGSE_POSTSCREENDRAW, 0, nullptr);
    }

    // Add mouse cursor pic, and global screen tint effect
    if (play.screen_is_faded_out == 0)
    {
        // Stage: mouse cursor
        if (draw_mouse && !play.mouse_cursor_hidden)
        {
            gfxDriver->DrawSprite(mousex - hotx, mousey - hoty, mouseCursor);
            invalidate_sprite(mousex - hotx, mousey - hoty, mouseCursor, false);
        }
        // Stage: screen fx
        if (play.screen_tint >= 1)
            gfxDriver->SetScreenTint(play.screen_tint & 0xff, (play.screen_tint >> 8) & 0xff, (play.screen_tint >> 16) & 0xff);
    }
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
    gfxDriver->BeginSpriteBatch(viewport, SpriteTransform());

    if (display_fps != kFPS_Hide)
        draw_fps(viewport);

    gfxDriver->EndSpriteBatch();
}

static void update_shakescreen()
{
    // TODO: unify blocking and non-blocking shake update
    play.shake_screen_yoff = 0;
    if (play.shakesc_length > 0)
    {
        if ((loopcounter % play.shakesc_delay) < (play.shakesc_delay / 2))
            play.shake_screen_yoff = play.shakesc_amount;
    }
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

    // Software mode scaling
    // note we don't use transparency in software mode - may be slow in hi-res games
    if (drawstate.SoftwareRender &&
        (mask != kRoomAreaWalkBehind) &&
        (bmp->GetSize() != Size(thisroom.Width, thisroom.Height)))
    {
        recycle_bitmap(debugRoomMaskObj.Bmp,
            bmp->GetColorDepth(), thisroom.Width, thisroom.Height);
        debugRoomMaskObj.Bmp->StretchBlt(bmp, RectWH(0, 0, thisroom.Width, thisroom.Height));
        bmp = debugRoomMaskObj.Bmp.get();
    }

    debugRoomMaskObj.Ddb = recycle_ddb_bitmap(debugRoomMaskObj.Ddb, bmp, true /*opaque*/);
    debugRoomMaskObj.Ddb->SetAlpha(150);
    debugRoomMaskObj.Ddb->SetStretch(thisroom.Width, thisroom.Height);
}

void debug_draw_movelist(int charnum)
{
    debugMoveListChar = charnum;
}

void update_room_debug()
{
    if (debugRoomMask == kRoomAreaWalkable)
    {
        Bitmap *bmp = prepare_walkable_areas(-1);
        // Software mode scaling
        if (drawstate.SoftwareRender && (thisroom.MaskResolution > 1))
        {
            recycle_bitmap(debugRoomMaskObj.Bmp,
                bmp->GetColorDepth(), thisroom.Width, thisroom.Height);
            debugRoomMaskObj.Bmp->StretchBlt(bmp, RectWH(0, 0, thisroom.Width, thisroom.Height));
            bmp = debugRoomMaskObj.Bmp.get();
        }
        debugRoomMaskObj.Ddb = recycle_ddb_bitmap(debugRoomMaskObj.Ddb, bmp, true /*opaque*/);
        debugRoomMaskObj.Ddb->SetAlpha(150);
        debugRoomMaskObj.Ddb->SetStretch(thisroom.Width, thisroom.Height);
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

        if (game.chars[debugMoveListChar].walking > 0)
        {
            int mlsnum = game.chars[debugMoveListChar].walking;
            if (game.chars[debugMoveListChar].walking >= TURNING_AROUND)
                mlsnum %= TURNING_AROUND;
            const MoveList &cmls = mls[mlsnum];
            for (int i = 0; i < cmls.numstage - 1; i++) {
                int srcx = cmls.pos[i].X;
                int srcy = cmls.pos[i].Y;
                int targetx = cmls.pos[i + 1].X;
                int targety = cmls.pos[i + 1].Y;
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

    construct_game_scene(false);
    our_eip=5;
    // TODO: extraBitmap is a hack, used to place an additional gui element
    // on top of the screen. Normally this should be a part of the game UI stage.
    if (extraBitmap != nullptr)
    {
        gfxDriver->BeginSpriteBatch(play.GetMainViewport(), play.GetGlobalTransform(drawstate.FullFrameRedraw), (GraphicFlip)play.screen_flipped);
        invalidate_sprite(extraX, extraY, extraBitmap, false);
        gfxDriver->DrawSprite(extraX, extraY, extraBitmap);
        gfxDriver->EndSpriteBatch();
    }
    construct_game_screen_overlay(true);
    render_to_screen();

    if (!play.screen_is_faded_out) {
        // always update the palette, regardless of whether the plugin
        // vetos the screen update
        if (bg_just_changed) {
            setpal();
            bg_just_changed = 0;
        }
    }

    drawstate.ScreenIsDirty = false;
}
