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
#include "plugin/agsplugin.h"
#include "plugin/plugin_engine.h"
#include "ac/spritecache.h"
#include "gfx/gfx_util.h"
#include "gfx/graphicsdriver.h"
#include "gfx/ali3dexception.h"
#include "gfx/blender.h"
#include "media/audio/audio_system.h"
#include "ac/game.h"
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

RGB palette[256];

COLOR_MAP maincoltable;

IGraphicsDriver *gfxDriver = nullptr;
IDriverDependantBitmap *blankImage = nullptr;
IDriverDependantBitmap *blankSidebarImage = nullptr;
IDriverDependantBitmap *debugConsole = nullptr;

// ObjTexture is a helper struct that pairs a raw bitmap with
// a renderer's texture and an optional position
struct ObjTexture
{
    // Sprite ID
    uint32_t SpriteID = UINT32_MAX;
    // Raw bitmap; used for software render mode,
    // or when particular object types require generated image.
    std::unique_ptr<Bitmap> Bmp;
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
    bool  in_use = false;
    int   sppic = 0;
    short tintr = 0, tintg = 0, tintb = 0, tintamnt = 0, tintlight = 0;
    short lightlev = 0, zoom = 0;
    bool  mirrored = 0;
    int   x = 0, y = 0;
};

// actsps is used for temporary storage of the bitmap and texture
// of the latest version of the sprite (room objects and characters);
// objects sprites begin with index 0, characters are after ACTSP_OBJSOFF
std::vector<ObjTexture> actsps;
// Walk-behind textures (3D renderers only)
std::vector<ObjTexture> walkbehindobj;
// GUI surfaces
std::vector<ObjTexture> guibg;
// GUI render texture, for rendering all controls on same texture buffer
std::vector<IDriverDependantBitmap*> gui_render_tex;
// GUI control surfaces
std::vector<ObjTexture> guiobjbg;
// first control texture index of each GUI
std::vector<int> guiobjddbref;
// Overlay's cached transformed bitmap, for software mode
std::vector<std::unique_ptr<Bitmap>> overlaybmp;
// For debugging room masks
RoomAreaMask debugRoomMask = kRoomAreaNone;
ObjTexture debugRoomMaskObj;
int debugMoveListChar = -1;
ObjTexture debugMoveListObj;

// Cached character and object states, used to determine
// whether these require texture update
std::vector<ObjectCache> charcache;
ObjectCache objcache[MAX_ROOM_OBJECTS];
std::vector<Point> screenovercache;

bool current_background_is_dirty = false;

// Room background sprite
IDriverDependantBitmap* roomBackgroundBmp = nullptr;
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


Bitmap *debugConsoleBuffer = nullptr;

// whether there are currently remnants of a DisplaySpeech
bool screen_is_dirty = false;

Bitmap *raw_saved_screen = nullptr;
Bitmap *dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];


void setpal() {
    set_palette_range(palette, 0, 255, 0);
}

int _places_r = 3, _places_g = 2, _places_b = 3;

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

// NOTE: Some of these conversions are required even when using
// D3D and OpenGL rendering, for two reasons:
// 1) certain raw drawing operations are still performed by software
// Allegro methods, hence bitmaps should be kept compatible to any native
// software operations, such as blitting two bitmaps of different formats.
// 2) OpenGL renderer assumes native bitmaps are in OpenGL-compatible format,
// so that it could copy them to texture without additional changes.
//
// TODO: make gfxDriver->GetCompatibleBitmapFormat describe all necessary
// conversions, so that we did not have to guess.
//
Bitmap *AdjustBitmapForUseWithDisplayMode(Bitmap* bitmap, bool has_alpha)
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
    const int sys_col_depth = gfxDriver->GetDisplayMode().ColorDepth;
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
    if (game_col_depth == 32 && bmp_col_depth == 32)
    {
        if (has_alpha) 
            set_rgb_mask_using_alpha_channel(new_bitmap);
    }
    // In 32-bit game hicolor bitmaps must be converted to the true color
    else if (game_col_depth == 32 && (bmp_col_depth > 8 && bmp_col_depth <= 16))
    {
        new_bitmap = BitmapHelper::CreateBitmapCopy(bitmap, compat_col_depth);
    }
    // In non-32-bit game truecolor bitmaps must be downgraded
    else if (game_col_depth <= 16 && bmp_col_depth > 16)
    {
        if (has_alpha) // if has valid alpha channel, convert it to regular transparency mask
            new_bitmap = remove_alpha_channel(bitmap);
        else // else simply convert bitmap
            new_bitmap = BitmapHelper::CreateBitmapCopy(bitmap, compat_col_depth);
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

Bitmap *PrepareSpriteForUse(Bitmap* bitmap, bool has_alpha)
{
    Bitmap *new_bitmap = AdjustBitmapForUseWithDisplayMode(bitmap, has_alpha);
    if (new_bitmap != bitmap)
        delete bitmap;
    return new_bitmap;
}

PBitmap PrepareSpriteForUse(PBitmap bitmap, bool has_alpha)
{
    Bitmap *new_bitmap = AdjustBitmapForUseWithDisplayMode(bitmap.get(), has_alpha);
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


// Begin resolution system functions

// Multiplies up the number of pixels depending on the current 
// resolution, to give a relatively fixed size at any game res
AGS_INLINE int get_fixed_pixel_size(int pixels)
{
    return pixels * game.GetRelativeUIMult();
}

AGS_INLINE int data_to_game_coord(int coord)
{
    return coord * game.GetDataUpscaleMult();
}

AGS_INLINE void data_to_game_coords(int *x, int *y)
{
    const int mul = game.GetDataUpscaleMult();
    x[0] *= mul;
    y[0] *= mul;
}

AGS_INLINE void data_to_game_round_up(int *x, int *y)
{
    const int mul = game.GetDataUpscaleMult();
    x[0] = x[0] * mul + (mul - 1);
    y[0] = y[0] * mul + (mul - 1);
}

AGS_INLINE int game_to_data_coord(int coord)
{
    return coord / game.GetDataUpscaleMult();
}

AGS_INLINE void game_to_data_coords(int &x, int &y)
{
    const int mul = game.GetDataUpscaleMult();
    x /= mul;
    y /= mul;
}

AGS_INLINE int game_to_data_round_up(int coord)
{
    const int mul = game.GetDataUpscaleMult();
    return (coord / mul) + (mul - 1);
}

AGS_INLINE void ctx_data_to_game_coord(int &x, int &y, bool hires_ctx)
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

AGS_INLINE void ctx_data_to_game_size(int &w, int &h, bool hires_ctx)
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

AGS_INLINE int ctx_data_to_game_size(int size, bool hires_ctx)
{
    if (hires_ctx && !game.IsLegacyHiRes())
        return std::max(1, (size / HIRES_COORD_MULTIPLIER));
    if (!hires_ctx && game.IsLegacyHiRes())
        return size * HIRES_COORD_MULTIPLIER;
    return size;
}

AGS_INLINE int game_to_ctx_data_size(int size, bool hires_ctx)
{
    if (hires_ctx && !game.IsLegacyHiRes())
        return size * HIRES_COORD_MULTIPLIER;
    else if (!hires_ctx && game.IsLegacyHiRes())
        return std::max(1, (size / HIRES_COORD_MULTIPLIER));
    return size;
}

AGS_INLINE void defgame_to_finalgame_coords(int &x, int &y)
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
    if (gfxDriver->HasAcceleratedTransform())
    {
        walkBehindMethod = DrawAsSeparateSprite;
        create_blank_image(game.GetColorDepth());
    }
    else
    {
        walkBehindMethod = DrawOverCharSprite;
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

void init_game_drawdata()
{
    // character and object caches
    charcache.resize(game.numcharacters);
    for (int i = 0; i < MAX_ROOM_OBJECTS; ++i)
        objcache[i] = ObjectCache();

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
}

void dispose_game_drawdata()
{
    clear_drawobj_cache();

    charcache.clear();
    actsps.clear();
    walkbehindobj.clear();

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
    screenovercache.clear();

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
    // cleanup Overlay intermediate bitmaps
    overlaybmp.clear();

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
    if (!gfxDriver->RequiresFullRedrawEachFrame())
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
    // Update debug overlays, if any were on
    debug_draw_room_mask(debugRoomMask);
    debug_draw_movelist(debugMoveListChar);

    // Following data is only updated for software renderer
    if (gfxDriver->RequiresFullRedrawEachFrame())
        return;
    // Make sure all frame buffers are created for software drawing
    int view_count = play.GetRoomViewportCount();
    CameraDrawData.resize(view_count);
    for (int i = 0; i < play.GetRoomViewportCount(); ++i)
        sync_roomview(play.GetRoomViewport(i).get());
}

void on_roomviewport_created(int index)
{
    if (!gfxDriver || gfxDriver->RequiresFullRedrawEachFrame())
        return;
    if ((size_t)index < CameraDrawData.size())
        return;
    CameraDrawData.resize(index + 1);
}

void on_roomviewport_deleted(int index)
{
    if (gfxDriver->RequiresFullRedrawEachFrame())
        return;
    CameraDrawData.erase(CameraDrawData.begin() + index);
    delete_invalid_regions(index);
}

void on_roomviewport_changed(Viewport *view)
{
    if (gfxDriver->RequiresFullRedrawEachFrame())
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
    if (gfxDriver->RequiresFullRedrawEachFrame())
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
    if (gfxDriver->RequiresFullRedrawEachFrame())
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

void reset_objcache_for_sprite(int sprnum, bool deleted)
{
    // Check if this sprite is assigned to any game object, and mark these for update;
    // if the sprite was deleted, also mark texture objects as invalid.
    // IMPORTANT!!: do NOT dispose textures themselves here.
    // * if the next valid image is of the same size, then the texture will be reused;
    // * BACKWARD COMPAT: keep last images during room transition out!
    // room objects cache
    if (croom != nullptr)
    {
        for (size_t i = 0; i < (size_t)croom->numobj; ++i)
        {
            if (objcache[i].sppic == sprnum)
                objcache[i].sppic = -1;
            if (deleted && (actsps[i].SpriteID == sprnum))
                actsps[i].SpriteID = UINT32_MAX; // invalid sprite ref
        }
    }
    // character cache
    for (size_t i = 0; i < (size_t)game.numcharacters; ++i)
    {
        if (charcache[i].sppic == sprnum)
            charcache[i].sppic = -1;
        if (deleted && (actsps[ACTSP_OBJSOFF + i].SpriteID == sprnum))
            actsps[ACTSP_OBJSOFF + i].SpriteID = UINT32_MAX; // invalid sprite ref
    }
}

void reset_drawobj_for_overlay(int objnum)
{
    if (objnum < overlaybmp.size())
    {
        // NOTE: we have to move memory here to sync with how overlay indexing works in this engine version
        std::move(overlaybmp.begin() + objnum + 1, overlaybmp.end(), overlaybmp.begin() + objnum);
        std::move(screenovercache.begin() + objnum + 1, screenovercache.end(), screenovercache.begin() + objnum);
    }
}

void mark_screen_dirty()
{
    screen_is_dirty = true;
}

bool is_screen_dirty()
{
    return screen_is_dirty;
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
    wouttext_outline(ds, x1, y1, font, text_color, (char*)text);
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
    const bool full_frame_rend = gfxDriver->RequiresFullRedrawEachFrame();
    // Stage: final plugin callback (still drawn on game screen
    if (pl_any_want_hook(AGSE_FINALSCREENDRAW))
    {
        gfxDriver->BeginSpriteBatch(play.GetMainViewport(),
            play.GetGlobalTransform(full_frame_rend), (GraphicFlip)play.screen_flipped);
        gfxDriver->DrawSprite(AGSE_FINALSCREENDRAW, 0, nullptr);
        gfxDriver->EndSpriteBatch();
    }
    // Stage: engine overlay
    construct_engine_overlay();

    // Try set new vsync value, and remember the actual result
    bool new_vsync = gfxDriver->SetVsync(scsystem.vsync > 0);
    if (new_vsync != scsystem.vsync)
        System_SetVSyncInternal(new_vsync);

    bool succeeded = false;
    while (!succeeded && !want_exit && !abort_engine)
    {
        try
        {
            if (full_frame_rend)
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
    ds->FillRect(Rect(xx, yy, xx + get_fixed_pixel_size(1) - 1, yy + get_fixed_pixel_size(1) - 1), col);
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


Engine::IDriverDependantBitmap* recycle_ddb_sprite(Engine::IDriverDependantBitmap *ddb, uint32_t sprite_id,
    Common::Bitmap *source, bool has_alpha, bool opaque)
{
    // no ddb, - get or create shared object
    if (!ddb)
        return gfxDriver->GetSharedDDB(sprite_id, source, has_alpha, opaque);
    // same sprite id, - use existing
    if ((sprite_id != UINT32_MAX) && (ddb->GetRefID() == sprite_id))
        return ddb;
    // not related to a sprite ID, but has same resolution, -
    // repaint directly from the given bitmap
    if ((sprite_id == UINT32_MAX) &&
        (ddb->GetColorDepth() == source->GetColorDepth()) &&
        (ddb->GetWidth() == source->GetWidth()) && (ddb->GetHeight() == source->GetHeight()))
    {
        gfxDriver->UpdateDDBFromBitmap(ddb, source, has_alpha);
        return ddb;
    }
    // have to recreate ddb
    gfxDriver->DestroyDDB(ddb);
    return gfxDriver->GetSharedDDB(sprite_id, source, has_alpha, opaque);
}

IDriverDependantBitmap* recycle_render_target(IDriverDependantBitmap *ddb, int width, int height, int col_depth, bool opaque)
{
    if (ddb && (ddb->GetWidth() == width) && (ddb->GetHeight() == height))
        return ddb;
    if (ddb)
        gfxDriver->DestroyDDB(ddb);
    return gfxDriver->CreateRenderTargetDDB(width, height, col_depth, opaque);
}

void sync_object_texture(ObjTexture &obj, bool has_alpha = false , bool opaque = false)
{
    Bitmap *use_bmp = obj.Bmp.get() ? obj.Bmp.get() : spriteset[obj.SpriteID];
    obj.Ddb = recycle_ddb_sprite(obj.Ddb, obj.SpriteID, use_bmp, has_alpha, opaque);
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

static void add_to_sprite_list(IDriverDependantBitmap* ddb, int x, int y, int zorder, bool isWalkBehind, int id = -1)
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

    if (walkBehindMethod == DrawAsSeparateSprite)
        sprite.takesPriorityIfEqual = !isWalkBehind;
    else
        sprite.takesPriorityIfEqual = isWalkBehind;

    sprlist.push_back(sprite);
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
void get_local_tint(int xpp, int ypp, int nolight,
                    int *tint_amnt, int *tint_r, int *tint_g,
                    int *tint_b, int *tint_lit,
                    int *light_lev) {

    int tint_level = 0, light_level = 0;
    int tint_amount = 0;
    int tint_red = 0;
    int tint_green = 0;
    int tint_blue = 0;
    int tint_light = 255;

    if (nolight == 0) {

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




// Applies the specified RGB Tint or Light Level to the actsps
// sprite indexed with actspsindex.
// Used for software render mode only.
static void apply_tint_or_light(int actspsindex, int light_level,
                         int tint_amount, int tint_red, int tint_green,
                         int tint_blue, int tint_light, int coldept,
                         Bitmap *blitFrom) {

 // In a 256-colour game, we cannot do tinting or lightening
 // (but we can do darkening, if light_level < 0)
 if (game.color_depth == 1) {
     if ((light_level > 0) || (tint_amount != 0))
         return;
 }

 auto &actsp = actsps[actspsindex];
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
static Bitmap *transform_sprite(Bitmap *src, bool src_has_alpha, std::unique_ptr<Bitmap> &dst,
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
            if ((IS_ANTIALIAS_SPRITES) && !src_has_alpha)
                tempbmp.AAStretchBlt(src, RectWH(dst_sz), kBitmap_Transparency);
            else
                tempbmp.StretchBlt(src, RectWH(dst_sz), kBitmap_Transparency);
            dst->FlipBlt(&tempbmp, 0, 0, kFlip_Horizontal);
        }
        else
        {
            if ((IS_ANTIALIAS_SPRITES) && !src_has_alpha)
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

// Draws the specified 'sppic' sprite onto actsps[useindx] at the
// specified width and height, and flips the sprite if necessary.
// Returns 1 if something was drawn to actsps; returns 0 if no
// scaling or stretching was required, in which case nothing was done.
// Used for software render mode only.
static bool scale_and_flip_sprite(int useindx, int sppic, int newwidth, int newheight, bool hmirror)
{
    Bitmap *src = spriteset[sppic];
    Bitmap *result = transform_sprite(src, (game.SpriteInfos[sppic].Flags & SPF_ALPHACHANNEL) != 0,
        actsps[useindx].Bmp, Size(newwidth, newheight), hmirror ? kFlip_Horizontal : kFlip_None);
    return result != src;
}

// Create the actsps[aa] image with the object drawn correctly.
// Returns true if nothing at all has changed and actsps is still
// intact from last time; false otherwise.
// Hardware-accelerated renderers always return true, because they do not
// require altering the raw bitmap itself.
// Except if alwaysUseSoftware is set, in which case even HW renderers
// construct the image in software mode as well.
bool construct_object_gfx(int aa, int *drawnWidth, int *drawnHeight, bool alwaysUseSoftware) {
    bool hardwareAccelerated = !alwaysUseSoftware && gfxDriver->HasAcceleratedTransform();

    if (spriteset[objs[aa].num] == nullptr)
        quitprintf("There was an error drawing object %d. Its current sprite, %d, is invalid.", aa, objs[aa].num);

    int coldept = spriteset[objs[aa].num]->GetColorDepth();
    const int src_sprwidth = game.SpriteInfos[objs[aa].num].Width;
    const int src_sprheight = game.SpriteInfos[objs[aa].num].Height;
    int sprwidth = src_sprwidth;
    int sprheight = src_sprheight;

    int tint_red, tint_green, tint_blue;
    int tint_level, tint_light, light_level;
    int zoom_level = 100;

    // calculate the zoom level
    if ((objs[aa].flags & OBJF_USEROOMSCALING) == 0)
    {
        zoom_level = objs[aa].zoom;
    }
    else
    {
        int onarea = get_walkable_area_at_location(objs[aa].x, objs[aa].y);
        if ((onarea <= 0) && (thisroom.WalkAreas[0].ScalingFar == 0)) {
            // just off the edge of an area -- use the scaling we had
            // while on the area
            zoom_level = objs[aa].zoom;
        }
        else
            zoom_level = get_area_scaling(onarea, objs[aa].x, objs[aa].y);
    }
    if (zoom_level != 100)
        scale_sprite_size(objs[aa].num, zoom_level, &sprwidth, &sprheight);
    objs[aa].zoom = zoom_level;

    // save width/height into parameters if requested
    if (drawnWidth)
        *drawnWidth = sprwidth;
    if (drawnHeight)
        *drawnHeight = sprheight;

    objs[aa].last_width = sprwidth;
    objs[aa].last_height = sprheight;

    tint_red = tint_green = tint_blue = tint_level = tint_light = light_level = 0;

    if (objs[aa].flags & OBJF_HASTINT) {
        // object specific tint, use it
        tint_red = objs[aa].tint_r;
        tint_green = objs[aa].tint_g;
        tint_blue = objs[aa].tint_b;
        tint_level = objs[aa].tint_level;
        tint_light = objs[aa].tint_light;
        light_level = 0;
    }
    else if (objs[aa].flags & OBJF_HASLIGHT)
    {
        light_level = objs[aa].tint_light;
    }
    else {
        // get the ambient or region tint
        int ignoreRegionTints = 1;
        if (objs[aa].flags & OBJF_USEREGIONTINTS)
            ignoreRegionTints = 0;

        get_local_tint(objs[aa].x, objs[aa].y, ignoreRegionTints,
            &tint_level, &tint_red, &tint_green, &tint_blue,
            &tint_light, &light_level);
    }

    // check whether the image should be flipped
    bool isMirrored = false;
    if ( (objs[aa].view != (uint16_t)-1) &&
        (views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame].pic == objs[aa].num) &&
        ((views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame].flags & VFLG_FLIPSPRITE) != 0)) {
            isMirrored = true;
    }

    const int useindx = aa; // actsps array index
    auto &actsp = actsps[useindx];
    actsp.SpriteID = objs[aa].num; // for texture sharing
    // NOTE: we need cached bitmap if:
    // * it's a software renderer, otherwise
    // * the walk-behind method is DrawOverCharSprite
    if ((hardwareAccelerated) && (walkBehindMethod != DrawOverCharSprite))
    {
        // HW acceleration
        bool is_texture_intact = objcache[aa].sppic == objs[aa].num;
        objcache[aa].sppic = objs[aa].num;
        objcache[aa].tintamnt = tint_level;
        objcache[aa].tintr = tint_red;
        objcache[aa].tintg = tint_green;
        objcache[aa].tintb = tint_blue;
        objcache[aa].tintlight = tint_light;
        objcache[aa].lightlev = light_level;
        objcache[aa].zoom = zoom_level;
        objcache[aa].mirrored = isMirrored;
        return is_texture_intact;
    }

    //
    // Software mode below
    //
    if ((!hardwareAccelerated) && (gfxDriver->HasAcceleratedTransform()))
    {
        // They want to draw it in software mode with the D3D driver, so force a redraw
        objcache[aa].sppic = -389538;
    }

    // If we have the image cached, use it
    if ((objcache[aa].image != nullptr) &&
        (objcache[aa].sppic == objs[aa].num) &&
        (objcache[aa].tintamnt == tint_level) &&
        (objcache[aa].tintlight == tint_light) &&
        (objcache[aa].tintr == tint_red) &&
        (objcache[aa].tintg == tint_green) &&
        (objcache[aa].tintb == tint_blue) &&
        (objcache[aa].lightlev == light_level) &&
        (objcache[aa].zoom == zoom_level) &&
        (objcache[aa].mirrored == isMirrored)) {
            // the image is the same, we can use it cached!
            if ((walkBehindMethod != DrawOverCharSprite) &&
                (actsp.Bmp != nullptr))
                return true;
            // Check if the X & Y co-ords are the same, too -- if so, there
            // is scope for further optimisations
            if ((objcache[aa].x == objs[aa].x) &&
                (objcache[aa].y == objs[aa].y) &&
                (actsp.Bmp != nullptr) &&
                (walk_behind_baselines_changed == 0))
                return true;
            recycle_bitmap(actsp.Bmp, coldept, sprwidth, sprheight);
            actsp.Bmp->Blit(objcache[aa].image.get(), 0, 0, 0, 0, objcache[aa].image->GetWidth(), objcache[aa].image->GetHeight());
            return false; // image was modified
    }

    // Not cached, so draw the image
    bool actspsUsed = false;
    if (!hardwareAccelerated)
    {
        // draw the base sprite, scaled and flipped as appropriate
        actspsUsed = scale_and_flip_sprite(useindx, objs[aa].num, sprwidth, sprheight, isMirrored);
    }
    if (!actspsUsed)
    {
        recycle_bitmap(actsp.Bmp, coldept, src_sprwidth, src_sprheight);
    }

    // direct read from source bitmap, where possible
    Bitmap *comeFrom = nullptr;
    if (!actspsUsed)
        comeFrom = spriteset[objs[aa].num];

    // apply tints or lightenings where appropriate, else just copy
    // the source bitmap
    if (!hardwareAccelerated && ((tint_level > 0) || (light_level != 0)))
    {
        apply_tint_or_light(useindx, light_level, tint_level, tint_red,
            tint_green, tint_blue, tint_light, coldept,
            comeFrom);
    }
    else if (!actspsUsed) {
        actsp.Bmp->Blit(spriteset[objs[aa].num], 0, 0);
    }

    // Re-use the bitmap if it's the same size
    recycle_bitmap(objcache[aa].image, coldept, sprwidth, sprheight);
    // Create the cached image and store it
    objcache[aa].image->Blit(actsp.Bmp.get(), 0, 0);
    objcache[aa].sppic = objs[aa].num;
    objcache[aa].tintamnt = tint_level;
    objcache[aa].tintr = tint_red;
    objcache[aa].tintg = tint_green;
    objcache[aa].tintb = tint_blue;
    objcache[aa].tintlight = tint_light;
    objcache[aa].lightlev = light_level;
    objcache[aa].zoom = zoom_level;
    objcache[aa].mirrored = isMirrored;
    return false; // image was modified
}




// This is only called from draw_screen_background, but it's seperated
// to help with profiling the program
void prepare_objects_for_drawing() {
    our_eip=32;

    for (uint32_t aa=0; aa<croom->numobj; aa++) {
        if (objs[aa].on != 1) continue;
        // offscreen, don't draw
        if ((objs[aa].x >= thisroom.Width) || (objs[aa].y < 1))
            continue;

        int tehHeight;
        bool actspsIntact = construct_object_gfx(aa, nullptr, &tehHeight, false);

        const int useindx = aa; // actsps array index
        auto &actsp = actsps[useindx];

        // update the cache for next time
        objcache[aa].x = objs[aa].x;
        objcache[aa].y = objs[aa].y;
        int atxp = data_to_game_coord(objs[aa].x);
        int atyp = data_to_game_coord(objs[aa].y) - tehHeight;

        int usebasel = objs[aa].get_baseline();

        if (objs[aa].flags & OBJF_NOWALKBEHINDS) {
            // ignore walk-behinds, do nothing
            if (walkBehindMethod == DrawAsSeparateSprite)
            {
                usebasel += thisroom.Height;
            }
        }
        else if ((!actspsIntact) && (walkBehindMethod == DrawOverCharSprite))
        {
            walkbehinds_cropout(actsp.Bmp.get(), atxp, atyp, usebasel);
        }

        if ((!actspsIntact) || (actsp.Ddb == nullptr))
        {
            sync_object_texture(actsp, (game.SpriteInfos[objs[aa].num].Flags & SPF_ALPHACHANNEL) != 0);
        }

        if (gfxDriver->HasAcceleratedTransform())
        {
            actsp.Ddb->SetFlippedLeftRight(objcache[aa].mirrored);
            actsp.Ddb->SetStretch(objs[aa].last_width, objs[aa].last_height);
            actsp.Ddb->SetTint(objcache[aa].tintr, objcache[aa].tintg, objcache[aa].tintb, (objcache[aa].tintamnt * 256) / 100);

            if (objcache[aa].tintamnt > 0)
            {
                if (objcache[aa].tintlight == 0)  // luminance of 0 -- pass 1 to enable
                    actsp.Ddb->SetLightLevel(1);
                else if (objcache[aa].tintlight < 250)
                    actsp.Ddb->SetLightLevel(objcache[aa].tintlight);
                else
                    actsp.Ddb->SetLightLevel(0);
            }
            else if (objcache[aa].lightlev != 0)
                actsp.Ddb->SetLightLevel((objcache[aa].lightlev * 25) / 10 + 256);
            else
                actsp.Ddb->SetLightLevel(0);
        }

        actsp.Ddb->SetAlpha(GfxDef::LegacyTrans255ToAlpha255(objs[aa].transparent));
        add_to_sprite_list(actsp.Ddb, atxp, atyp, usebasel, false);
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




void prepare_characters_for_drawing() {
    int zoom_level,newwidth,newheight,onarea,sppic;
    int light_level,coldept;
    int tint_red, tint_green, tint_blue, tint_amount, tint_light = 255;

    our_eip=33;

    // draw characters
    for (int aa=0; aa < game.numcharacters; aa++) {
        if (game.chars[aa].on==0) continue;
        if (game.chars[aa].room!=displayed_room) continue;
        eip_guinum = aa;

        CharacterInfo*chin=&game.chars[aa];
        our_eip = 330;
        // Test for valid view and loop
        if (chin->view < 0) {
            quitprintf("!The character '%s' was turned on in the current room (room %d) but has not been assigned a view number.",
                chin->name, displayed_room);
        }
        if (chin->loop >= views[chin->view].numLoops) {
            quitprintf("!The character '%s' could not be displayed because there was no loop %d of view %d.",
                chin->name, chin->loop, chin->view + 1);
        }
        // If frame is too high -- fallback to the frame 0;
        // there's always at least 1 dummy frame at index 0
        if (chin->frame >= views[chin->view].loops[chin->loop].numFrames)
            chin->frame = 0;

        sppic=views[chin->view].loops[chin->loop].frames[chin->frame].pic;
        if (sppic < 0)
            sppic = 0;  // in case it's screwed up somehow
        our_eip = 331;
        // sort out the stretching if required
        onarea = get_walkable_area_at_character (aa);
        our_eip = 332;

        // calculate the zoom level
        if (chin->flags & CHF_MANUALSCALING)  // character ignores scaling
            zoom_level = charextra[aa].zoom;
        else if ((onarea <= 0) && (thisroom.WalkAreas[0].ScalingFar == 0)) {
            zoom_level = charextra[aa].zoom;
            // NOTE: room objects don't have this fix
            if (zoom_level == 0)
                zoom_level = 100;
        }
        else
            zoom_level = get_area_scaling (onarea, chin->x, chin->y);

        charextra[aa].zoom = zoom_level;

        tint_red = tint_green = tint_blue = tint_amount = tint_light = light_level = 0;

        if (chin->flags & CHF_HASTINT) {
            // object specific tint, use it
            tint_red = charextra[aa].tint_r;
            tint_green = charextra[aa].tint_g;
            tint_blue = charextra[aa].tint_b;
            tint_amount = charextra[aa].tint_level;
            tint_light = charextra[aa].tint_light;
            light_level = 0;
        }
        else if (chin->flags & CHF_HASLIGHT)
        {
            light_level = charextra[aa].tint_light;
        }
        else {
            get_local_tint(chin->x, chin->y, chin->flags & CHF_NOLIGHTING,
                &tint_amount, &tint_red, &tint_green, &tint_blue,
                &tint_light, &light_level);
        }

        our_eip = 3330;
        bool isMirrored = false;
        int specialpic = sppic;
        bool usingCachedImage = false;

        coldept = spriteset[sppic]->GetColorDepth();

        // adjust the sppic if mirrored, so it doesn't accidentally
        // cache the mirrored frame as the real one
        if (views[chin->view].loops[chin->loop].frames[chin->frame].flags & VFLG_FLIPSPRITE) {
            isMirrored = true;
            specialpic = -sppic;
        }

        our_eip = 3331;

        const int useindx = aa + ACTSP_OBJSOFF; // actsps array index
        auto &actsp = actsps[useindx];
        actsp.SpriteID = sppic; // for texture sharing

        // if the character was the same sprite and scaling last time,
        // just use the cached image
        if ((charcache[aa].in_use) &&
            (charcache[aa].sppic == specialpic) &&
            (charcache[aa].zoom == zoom_level) &&
            (charcache[aa].tintr == tint_red) &&
            (charcache[aa].tintg == tint_green) &&
            (charcache[aa].tintb == tint_blue) &&
            (charcache[aa].tintamnt == tint_amount) &&
            (charcache[aa].tintlight == tint_light) &&
            (charcache[aa].lightlev == light_level)) 
        {
            if (walkBehindMethod == DrawOverCharSprite)
            {
                recycle_bitmap(actsp.Bmp, charcache[aa].image->GetColorDepth(), charcache[aa].image->GetWidth(), charcache[aa].image->GetHeight());
                actsp.Bmp->Blit(charcache[aa].image.get(), 0, 0);
            }
            else
            {
                usingCachedImage = true;
            }
        }
        else if ((charcache[aa].in_use) && 
            (charcache[aa].sppic == specialpic) &&
            (gfxDriver->HasAcceleratedTransform()))
        {
            usingCachedImage = true;
        }
        else if (charcache[aa].in_use) {
            charcache[aa].in_use = false;
        }

        our_eip = 3332;

        const int src_sprwidth = game.SpriteInfos[sppic].Width;
        const int src_sprheight = game.SpriteInfos[sppic].Height;

        if (zoom_level != 100) {
            // it needs to be stretched, so calculate the new dimensions

            scale_sprite_size(sppic, zoom_level, &newwidth, &newheight);
            charextra[aa].width=newwidth;
            charextra[aa].height=newheight;
        }
        else {
            // draw at original size, so just use the sprite width and height
            // TODO: store width and height always, that's much simplier to use for reference!
            charextra[aa].width=0;
            charextra[aa].height=0;
            newwidth = src_sprwidth;
            newheight = src_sprheight;
        }

        our_eip = 3336;

        // Calculate the X & Y co-ordinates of where the sprite will be
        const int atxp =(data_to_game_coord(chin->x)) - newwidth/2;
        const int atyp =(data_to_game_coord(chin->y) - newheight)
            // adjust the Y positioning for the character's Z co-ord
            - data_to_game_coord(chin->z);

        charcache[aa].zoom = zoom_level;
        charcache[aa].sppic = specialpic;
        charcache[aa].tintr = tint_red;
        charcache[aa].tintg = tint_green;
        charcache[aa].tintb = tint_blue;
        charcache[aa].tintamnt = tint_amount;
        charcache[aa].tintlight = tint_light;
        charcache[aa].lightlev = light_level;

        // If cache needs to be re-drawn
        // NOTE: we need cached bitmap if:
        // * it's a software renderer, otherwise
        // * the walk-behind method is DrawOverCharSprite
        if (((!gfxDriver->HasAcceleratedTransform()) || (walkBehindMethod == DrawOverCharSprite))
            && !charcache[aa].in_use)
        {
            // create the base sprite in actsps[useindx], which will
            // be scaled and/or flipped, as appropriate
            bool actspsUsed = false;
            if (!gfxDriver->HasAcceleratedTransform())
            {
                actspsUsed = scale_and_flip_sprite(useindx, sppic, newwidth, newheight, isMirrored);
            }
            if (!actspsUsed)
            {
                // ensure actsps exists // CHECKME: why do we need this in hardware accel mode too?
                recycle_bitmap(actsp.Bmp, coldept, src_sprwidth, src_sprheight);
            }

            our_eip = 335;

            if (((light_level != 0) || (tint_amount != 0)) &&
                (!gfxDriver->HasAcceleratedTransform())) {
                    // apply the lightening or tinting
                    Bitmap *comeFrom = nullptr;
                    // if possible, direct read from the source image
                    if (!actspsUsed)
                        comeFrom = spriteset[sppic];

                    apply_tint_or_light(useindx, light_level, tint_amount, tint_red,
                        tint_green, tint_blue, tint_light, coldept,
                        comeFrom);
            }
            else if (!actspsUsed) {
                // no scaling, flipping or tinting was done, so just blit it normally
                actsp.Bmp->Blit(spriteset[sppic], 0, 0);
            }

            // update the character cache with the new image
            charcache[aa].in_use = true;
            recycle_bitmap(charcache[aa].image, coldept, actsp.Bmp->GetWidth(), actsp.Bmp->GetHeight());
            charcache[aa].image->Blit(actsp.Bmp.get(), 0, 0);

        } // end if !cache.in_use

        int usebasel = chin->get_baseline();

        our_eip = 336;

        const int bgX = atxp + chin->pic_xoffs;
        const int bgY = atyp + chin->pic_yoffs;

        if (chin->flags & CHF_NOWALKBEHINDS) {
            // ignore walk-behinds, do nothing
            if (walkBehindMethod == DrawAsSeparateSprite)
            {
                usebasel += thisroom.Height;
            }
        }
        else if (walkBehindMethod == DrawOverCharSprite)
        {
            walkbehinds_cropout(actsp.Bmp.get(), bgX, bgY, usebasel);
        }

        if ((!usingCachedImage) || (actsp.Ddb == nullptr))
        {
            sync_object_texture(actsp, (game.SpriteInfos[sppic].Flags & SPF_ALPHACHANNEL) != 0);
        }

        if (gfxDriver->HasAcceleratedTransform()) 
        {
            actsp.Ddb->SetStretch(newwidth, newheight);
            actsp.Ddb->SetFlippedLeftRight(isMirrored);
            actsp.Ddb->SetTint(tint_red, tint_green, tint_blue, (tint_amount * 256) / 100);

            if (tint_amount != 0)
            {
                if (tint_light == 0) // tint with 0 luminance, pass as 1 instead
                    actsp.Ddb->SetLightLevel(1);
                else if (tint_light < 250)
                    actsp.Ddb->SetLightLevel(tint_light);
                else
                    actsp.Ddb->SetLightLevel(0);
            }
            else if (light_level != 0)
                actsp.Ddb->SetLightLevel((light_level * 25) / 10 + 256);
            else
                actsp.Ddb->SetLightLevel(0);

        }

        our_eip = 337;

        chin->actx = atxp;
        chin->acty = atyp;

        actsp.Ddb->SetAlpha(GfxDef::LegacyTrans255ToAlpha255(chin->transparency));
        add_to_sprite_list(actsp.Ddb, bgX, bgY, usebasel, false);
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
    for (size_t i = 0; i < screenover.size(); ++i)
    {
        auto &over = screenover[i];
        if (!over.IsRoomLayer()) continue; // not a room layer
        if (over.transparency == 255) continue; // skip fully transparent
        Point pos = get_overlay_position(over);
        add_to_sprite_list(over.ddb, pos.X, pos.Y, over.zorder, false);
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
    if (gfxDriver->RequiresFullRedrawEachFrame())
    {
        if (current_background_is_dirty || walkBehindsCachedForBgNum != play.bg_frame)
        {
            if (walkBehindMethod == DrawAsSeparateSprite)
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

            if (walkBehindMethod == DrawAsSeparateSprite)
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
    if (gfxDriver->RequiresFullRedrawEachFrame())
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
        fpsDisplay = CreateCompatBitmap(viewport.GetWidth(), (get_font_surface_height(font) + get_fixed_pixel_size(5)));
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
        gfxDriver->UpdateDDBFromBitmap(ddb, fpsDisplay, false);
    else
        ddb = gfxDriver->CreateDDBFromBitmap(fpsDisplay, false);
    int yp = viewport.GetHeight() - fpsDisplay->GetHeight();
    gfxDriver->DrawSprite(1, yp, ddb);
    invalidate_sprite_glob(1, yp, ddb);
}

// Draw GUI controls as separate sprites, each on their own texture
static void construct_guictrl_tex(GUIMain &gui)
{
    if ((all_buttons_disabled >= 0) && (GUI::Options.DisabledStyle == kGuiDis_Blackout))
        return; // don't draw GUI controls

    int draw_index = guiobjddbref[gui.ID];
    for (int i = 0; i < gui.GetControlCount(); ++i, ++draw_index)
    {
        GUIObject *obj = gui.GetControl(i);
        if (!obj->IsVisible() ||
            (obj->Width <= 0 || obj->Height <= 0) ||
            (!obj->IsEnabled() && (GUI::Options.DisabledStyle == kGuiDis_Blackout)))
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
    if ((all_buttons_disabled >= 0) && (GUI::Options.DisabledStyle == kGuiDis_Blackout))
    {
        gfxDriver->EndSpriteBatch();
        return;
    }

    // Add all the GUI controls
    const int draw_index = guiobjddbref[gui_id];
    for (const auto &obj_id : gui.GetControlsDrawOrder())
    {
        GUIObject *obj = gui.GetControl(obj_id);
        if (!obj->IsVisible() ||
            (obj->Width <= 0 || obj->Height <= 0) ||
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
    // Draw gui controls on separate textures if:
    // - it is a 3D renderer (software one may require adjustments -- needs testing)
    // - not legacy alpha blending (may we implement specific texture blend?)
    const bool draw_controls_as_textures =
           gfxDriver->HasAcceleratedTransform()
        && (game.options[OPT_NEWGUIALPHA] == kGuiAlphaRender_Proper);

    if(pl_any_want_hook(AGSE_PREGUIDRAW))
        gfxDriver->DrawSprite(AGSE_PREGUIDRAW, 0, nullptr); // render stage

    clear_sprite_list();

    const bool is_software_mode = !gfxDriver->HasAcceleratedTransform();
    // Add active overlays to the sprite list
    for (size_t i = 0; i < screenover.size(); ++i)
    {
        auto &over = screenover[i];
        if (over.IsRoomLayer()) continue; // not a ui layer
        if (over.transparency == 255) continue; // skip fully transparent
        Point pos = get_overlay_position(over);
        add_to_sprite_list(over.ddb, pos.X, pos.Y, over.zorder, false);
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
            const auto &gui = guis[index];
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
            add_to_sprite_list(gui_ddb, gui.X, gui.Y, gui.ZOrder, false, index);
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
            gfxDriver->DrawSprite(t.x, t.y, t.ddb);
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
        const SpriteTransform cam_trans(-cam_rc.Left, -cam_rc.Top);

        if (gfxDriver->RequiresFullRedrawEachFrame())
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
    const bool is_software_mode = !gfxDriver->HasAcceleratedTransform();
    if (is_software_mode && (overlaybmp.size() < screenover.size()))
    {
        overlaybmp.resize(screenover.size());
        screenovercache.resize(screenover.size(), Point(INT32_MIN, INT32_MIN));
    }
    for (size_t i = 0; i < screenover.size(); ++i)
    {
        auto &over = screenover[i];
        if (over.transparency == 255) continue; // skip fully transparent

        bool has_changed = over.HasChanged();
        if (over.IsRoomLayer() && (walkBehindMethod == DrawOverCharSprite))
        {
            Point pos = get_overlay_position(over);
            has_changed |= (pos.X != screenovercache[i].X || pos.Y != screenovercache[i].Y);
            screenovercache[i].X = pos.X; screenovercache[i].Y = pos.Y;
        }

        if (has_changed)
        {
            // For software mode - prepare transformed bitmap if necessary
            Bitmap *use_bmp = is_software_mode ?
                transform_sprite(over.GetImage(), over.HasAlphaChannel(), overlaybmp[i], Size(over.scaleWidth, over.scaleHeight)) :
                over.GetImage();

            if ((walkBehindMethod == DrawOverCharSprite) && over.IsRoomLayer())
            {
                if (use_bmp != overlaybmp[i].get())
                {
                    recycle_bitmap(overlaybmp[i], use_bmp->GetColorDepth(), use_bmp->GetWidth(), use_bmp->GetHeight(), true);
                    overlaybmp[i]->Blit(use_bmp);
                }
                Point pos = get_overlay_position(over);
                walkbehinds_cropout(overlaybmp[i].get(), pos.X, pos.Y, over.zorder);
                use_bmp = overlaybmp[i].get();
            }

            over.ddb = recycle_ddb_sprite(over.ddb, over.GetSpriteNum(), use_bmp, over.HasAlphaChannel());
            over.ClearChanged();
        }

        assert(over.ddb); // Test for missing texture, might happen if not marked for update
        if (!over.ddb) continue;
        over.ddb->SetStretch(over.scaleWidth, over.scaleHeight);
        over.ddb->SetAlpha(GfxDef::LegacyTrans255ToAlpha255(over.transparency));
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
    bool full_frame_rend = gfxDriver->RequiresFullRedrawEachFrame();
    gfxDriver->BeginSpriteBatch(play.GetMainViewport(),
        play.GetGlobalTransform(full_frame_rend),
        (GraphicFlip)play.screen_flipped);

    // Stage: room viewports
    if (play.screen_is_faded_out == 0 && play.complete_overlay_on == 0)
    {
        if (displayed_room >= 0)
        {
            construct_room_view();
        }
        else if (!full_frame_rend)
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
}

void construct_game_screen_overlay(bool draw_mouse)
{
    const bool full_frame_rend = gfxDriver->RequiresFullRedrawEachFrame();
    gfxDriver->BeginSpriteBatch(play.GetMainViewport(),
            play.GetGlobalTransform(full_frame_rend), (GraphicFlip)play.screen_flipped);
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
    if (full_frame_rend)
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

    // draw the debug console, if appropriate
    if ((play.debug_mode > 0) && (display_console != 0))
    {
        const int font = FONT_NORMAL;
        int ypp = 1;
        int txtspacing = get_font_linespacing(font);
        int barheight = get_text_lines_surf_height(font, DEBUG_CONSOLE_NUMLINES - 1) + 4;

        if (debugConsoleBuffer == nullptr)
        {
            debugConsoleBuffer = CreateCompatBitmap(viewport.GetWidth(), barheight);
        }

        color_t draw_color = debugConsoleBuffer->GetCompatibleColor(15);
        debugConsoleBuffer->FillRect(Rect(0, 0, viewport.GetWidth() - 1, barheight), draw_color);
        color_t text_color = debugConsoleBuffer->GetCompatibleColor(16);
        for (int jj = first_debug_line; jj != last_debug_line; jj = (jj + 1) % DEBUG_CONSOLE_NUMLINES) {
            wouttextxy(debugConsoleBuffer, 1, ypp, font, text_color, debug_line[jj].GetCStr());
            ypp += txtspacing;
        }

        if (debugConsole == nullptr)
            debugConsole = gfxDriver->CreateDDBFromBitmap(debugConsoleBuffer, false, true);
        else
            gfxDriver->UpdateDDBFromBitmap(debugConsole, debugConsoleBuffer, false);

        gfxDriver->DrawSprite(0, 0, debugConsole);
        invalidate_sprite_glob(0, 0, debugConsole);
    }

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
    if (!gfxDriver->HasAcceleratedTransform() &&
        (mask != kRoomAreaWalkBehind) &&
        (bmp->GetSize() != Size(thisroom.Width, thisroom.Height)))
    {
        recycle_bitmap(debugRoomMaskObj.Bmp,
            bmp->GetColorDepth(), thisroom.Width, thisroom.Height);
        debugRoomMaskObj.Bmp->StretchBlt(bmp, RectWH(0, 0, thisroom.Width, thisroom.Height));
        bmp = debugRoomMaskObj.Bmp.get();
    }

    debugRoomMaskObj.Ddb = recycle_ddb_bitmap(debugRoomMaskObj.Ddb, bmp, false, true);
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
        if (!gfxDriver->HasAcceleratedTransform() && (thisroom.MaskResolution > 1))
        {
            recycle_bitmap(debugRoomMaskObj.Bmp,
                bmp->GetColorDepth(), thisroom.Width, thisroom.Height);
            debugRoomMaskObj.Bmp->StretchBlt(bmp, RectWH(0, 0, thisroom.Width, thisroom.Height));
            bmp = debugRoomMaskObj.Bmp.get();
        }
        debugRoomMaskObj.Ddb = recycle_ddb_bitmap(debugRoomMaskObj.Ddb, bmp, false, true);
        debugRoomMaskObj.Ddb->SetAlpha(150);
        debugRoomMaskObj.Ddb->SetStretch(thisroom.Width, thisroom.Height);
    }
    if (debugMoveListChar >= 0)
    {
        const int mult = gfxDriver->HasAcceleratedTransform() ? thisroom.MaskResolution : 1;
        if (gfxDriver->HasAcceleratedTransform())
            recycle_bitmap(debugMoveListObj.Bmp, game.GetColorDepth(),
                thisroom.WalkAreaMask->GetWidth(), thisroom.WalkAreaMask->GetHeight(), true);
        else
            recycle_bitmap(debugMoveListObj.Bmp, game.GetColorDepth(),
                thisroom.Width, thisroom.Height, true);

        if (game.chars[debugMoveListChar].walking > 0)
        {
            int mlsnum = game.chars[debugMoveListChar].walking;
            if (game.chars[debugMoveListChar].walking >= TURNING_AROUND)
                mlsnum %= TURNING_AROUND;
            const MoveList &cmls = mls[mlsnum];
            for (int i = 0; i < cmls.numstage - 1; i++) {
                short srcx = short((cmls.pos[i] >> 16) & 0x00ffff);
                short srcy = short(cmls.pos[i] & 0x00ffff);
                short targetx = short((cmls.pos[i + 1] >> 16) & 0x00ffff);
                short targety = short(cmls.pos[i + 1] & 0x00ffff);
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
        gfxDriver->BeginSpriteBatch(play.GetMainViewport(), play.GetGlobalTransform(gfxDriver->RequiresFullRedrawEachFrame()), (GraphicFlip)play.screen_flipped);
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

    screen_is_dirty = false;
}
