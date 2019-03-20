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

#include <algorithm>
#include "aastr.h"
#include "ac/common.h"
#include "util/compress.h"
#include "ac/view.h"
#include "ac/charactercache.h"
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
#include "ac/objectcache.h"
#include "ac/overlay.h"
#include "ac/sys_events.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/runtime_defines.h"
#include "ac/screenoverlay.h"
#include "ac/sprite.h"
#include "ac/spritelistentry.h"
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
#include "media/audio/audio.h"
#include "media/audio/soundclip.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "plugin/plugin_engine.h"
#include "ac/spritecache.h"
#include "gfx/gfx_util.h"
#include "gfx/graphicsdriver.h"
#include "gfx/ali3dexception.h"
#include "gfx/blender.h"

using namespace AGS::Common;
using namespace AGS::Engine;

#if defined(ANDROID_VERSION)
#include <sys/stat.h>
#include <android/log.h>

extern "C" void android_render();
#endif

#if defined(IOS_VERSION)
extern "C" void ios_render();
#endif

extern GameSetup usetup;
extern GameSetupStruct game;
extern GameState play;
extern int convert_16bit_bgr;
extern ScriptSystem scsystem;
extern AGSPlatformDriver *platform;
extern RoomStruct thisroom;
extern char noWalkBehindsAtAll;
extern unsigned int loopcounter;
extern char *walkBehindExists;  // whether a WB area is in this column
extern int *walkBehindStartY, *walkBehindEndY;
extern int walkBehindLeft[MAX_WALK_BEHINDS], walkBehindTop[MAX_WALK_BEHINDS];
extern int walkBehindRight[MAX_WALK_BEHINDS], walkBehindBottom[MAX_WALK_BEHINDS];
extern IDriverDependantBitmap *walkBehindBitmap[MAX_WALK_BEHINDS];
extern int walkBehindsCachedForBgNum;
extern WalkBehindMethodEnum walkBehindMethod;
extern int walk_behind_baselines_changed;
extern SpriteCache spriteset;
extern RoomStatus*croom;
extern int our_eip;
extern int in_new_room;
extern RoomObject*objs;
extern ViewStruct*views;
extern CharacterCache *charcache;
extern ObjectCache objcache[MAX_ROOM_OBJECTS];
extern int displayed_room;
extern CharacterExtras *charextra;
extern CharacterInfo*playerchar;
extern int eip_guinum;
extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern int numscreenover;
extern int screen_reset;
extern int is_complete_overlay;
extern int cur_mode,cur_cursor;
extern int mouse_frame,mouse_delay;
extern int lastmx,lastmy;
extern IDriverDependantBitmap *mouseCursor;
extern int hotx,hoty;
extern int bg_just_changed;

// How is this actually used??
// We need COLOR_DEPTH_24 to allow it to load the preload PCX in hi-col
BEGIN_COLOR_DEPTH_LIST
    COLOR_DEPTH_8
    COLOR_DEPTH_15
    COLOR_DEPTH_16
    COLOR_DEPTH_24
    COLOR_DEPTH_32
END_COLOR_DEPTH_LIST


color palette[256];

COLOR_MAP maincoltable;

IGraphicsDriver *gfxDriver;
IDriverDependantBitmap *blankImage = NULL;
IDriverDependantBitmap *blankSidebarImage = NULL;
IDriverDependantBitmap *debugConsole = NULL;

// actsps is used for temporary storage of the bitamp image
// of the latest version of the sprite
int actSpsCount = 0;
Bitmap **actsps;
IDriverDependantBitmap* *actspsbmp;
// temporary cache of walk-behind for this actsps image
Bitmap **actspswb;
IDriverDependantBitmap* *actspswbbmp;
CachedActSpsData* actspswbcache;

bool current_background_is_dirty = false;

Bitmap *sub_vscreen = NULL;
int wasShakingScreen = 0;

// Room background sprite
IDriverDependantBitmap* roomBackgroundBmp = NULL;
// Intermediate bitmap for the software drawing method.
// We use this bitmap in case room camera has scaling enabled, we draw dirty room rects on it,
// and then pass to software renderer which draws sprite on top and then either blits or stretch-blits
// to the virtual screen.
// For more details see comment in ALSoftwareGraphicsDriver::RenderToBackBuffer().
PBitmap RoomCameraBuffer;  // this is the actual bitmap
PBitmap RoomCameraFrame;   // this is either same bitmap reference or sub-bitmap


std::vector<SpriteListEntry> sprlist;
std::vector<SpriteListEntry> thingsToDrawList;

Bitmap **guibg = NULL;
IDriverDependantBitmap **guibgbmp = NULL;


Bitmap *debugConsoleBuffer = NULL;

// whether there are currently remnants of a DisplaySpeech
bool screen_is_dirty = false;

Bitmap *raw_saved_screen = NULL;
Bitmap *dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];


SpriteListEntry::SpriteListEntry()
    : bmp(NULL)
    , pic(NULL)
    , baseline(0), x(0), y(0)
    , transparent(0)
    , takesPriorityIfEqual(false), hasAlphaChannel(false)
{
}


// TODO: move to test unit
extern Bitmap *test_allegro_bitmap;
extern IDriverDependantBitmap *test_allegro_ddb;
void allegro_bitmap_test_draw()
{
	if (test_allegro_bitmap)
	{
        test_allegro_bitmap->FillTransparent();
		test_allegro_bitmap->FillRect(Rect(50,50,150,150), 15);

		if (test_allegro_ddb == NULL) 
        {
            test_allegro_ddb = gfxDriver->CreateDDBFromBitmap(test_allegro_bitmap, false, true);
        }
        else
        {
            gfxDriver->UpdateDDBFromBitmap(test_allegro_ddb, test_allegro_bitmap, false);
        }
        gfxDriver->DrawSprite(-play.GetRoomCamera().Left, -play.GetRoomCamera().Top, test_allegro_ddb);
	}
}

void setpal() {
    set_palette_range(palette, 0, 255, 0);
}

int _places_r = 3, _places_g = 2, _places_b = 3;

// convert RGB to BGR for strange graphics cards
Bitmap *convert_16_to_16bgr(Bitmap *tempbl) {

    int x,y;
    unsigned short c,r,ds,b;

    for (y=0; y < tempbl->GetHeight(); y++) {
        unsigned short*p16 = (unsigned short *)tempbl->GetScanLine(y);

        for (x=0; x < tempbl->GetWidth(); x++) {
            c = p16[x];
            if (c != MASK_COLOR_16) {
                b = _rgb_scale_5[c & 0x1F];
                ds = _rgb_scale_6[(c >> 5) & 0x3F];
                r = _rgb_scale_5[(c >> 11) & 0x1F];
                // allegro assumes 5-6-5 for 16-bit
                p16[x] = (((r >> _places_r) << _rgb_r_shift_16) |
                    ((ds >> _places_g) << _rgb_g_shift_16) |
                    ((b >> _places_b) << _rgb_b_shift_16));

            }
        }
    }

    return tempbl;
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
Bitmap *AdjustBitmapForUseWithDisplayMode(Bitmap* bitmap, bool has_alpha)
{
    const int bmp_col_depth = bitmap->GetColorDepth();
    const int sys_col_depth = System_GetColorDepth();
    const int game_col_depth = game.GetColorDepth();
    Bitmap *new_bitmap = bitmap;

    //
    // The only special case when bitmap needs to be prepared for graphics driver
    //
    // In 32-bit display mode, 32-bit bitmaps may require component conversion
    // to match graphics driver expectation about pixel format.
    // TODO: make GetCompatibleBitmapFormat tell this somehow
#if defined (AGS_INVERTED_COLOR_ORDER)
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
        new_bitmap = BitmapHelper::CreateBitmapCopy(bitmap, game_col_depth);
    }
    // In non-32-bit game truecolor bitmaps must be downgraded
    else if (game_col_depth <= 16 && bmp_col_depth > 16)
    {
        if (has_alpha) // if has valid alpha channel, convert it to regular transparency mask
            new_bitmap = remove_alpha_channel(bitmap);
        else // else simply convert bitmap
            new_bitmap = BitmapHelper::CreateBitmapCopy(bitmap, game_col_depth);
    }
    // Special case when we must convert 16-bit RGB to BGR
    else if (convert_16bit_bgr == 1 && bmp_col_depth == 16)
    {
        new_bitmap = convert_16_to_16bgr(bitmap);
    }
    return new_bitmap;
}

Bitmap *ReplaceBitmapWithSupportedFormat(Bitmap *bitmap)
{
    Bitmap *new_bitmap = GfxUtil::ConvertBitmap(bitmap, gfxDriver->GetCompatibleBitmapFormat(bitmap->GetColorDepth()));
    if (new_bitmap != bitmap)
        delete bitmap;
    return new_bitmap;
}

Bitmap *PrepareSpriteForUse(Bitmap* bitmap, bool has_alpha)
{
    bool must_switch_palette = bitmap->GetColorDepth() == 8 && game.GetColorDepth() > 8;
    if (must_switch_palette)
        select_palette(palette);

    Bitmap *new_bitmap = AdjustBitmapForUseWithDisplayMode(bitmap, has_alpha);
    if (new_bitmap != bitmap)
        delete bitmap;
    new_bitmap = ReplaceBitmapWithSupportedFormat(new_bitmap);

    if (must_switch_palette)
        unselect_palette();
    return new_bitmap;
}

PBitmap PrepareSpriteForUse(PBitmap bitmap, bool has_alpha)
{
    bool must_switch_palette = bitmap->GetColorDepth() == 8 && System_GetColorDepth() > 8;
    if (must_switch_palette)
        select_palette(palette);

    Bitmap *new_bitmap = AdjustBitmapForUseWithDisplayMode(bitmap.get(), has_alpha);
    new_bitmap = ReplaceBitmapWithSupportedFormat(new_bitmap);

    if (must_switch_palette)
        unselect_palette();
    return new_bitmap == bitmap.get() ? bitmap : PBitmap(new_bitmap); // if bitmap is same, don't create new smart ptr!
}

Bitmap *CopyScreenIntoBitmap(int width, int height, bool at_native_res)
{
    Bitmap *dst = new Bitmap(width, height, game.GetColorDepth());
    Size want_size;
    // If the size and color depth are supported we may copy right into our bitmap
    if (gfxDriver->GetCopyOfScreenIntoBitmap(dst, at_native_res, &want_size))
        return dst;
    // Otherwise we might need to copy between few bitmaps...
    Bitmap *buf_screenfmt = new Bitmap(want_size.Width, want_size.Height, gfxDriver->GetDisplayMode().ColorDepth);
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
    return pixels * game.GetDataUpscaleMult();
}

AGS_INLINE int room_to_mask_coord(int coord)
{
    return coord / thisroom.MaskResolution;
}

AGS_INLINE int mask_to_room_coord(int coord)
{
    return coord * thisroom.MaskResolution;
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
    if (hires_ctx && !game.IsHiRes())
    {
        x /= HIRES_COORD_MULTIPLIER;
        y /= HIRES_COORD_MULTIPLIER;
    }
    else if (!hires_ctx && game.IsHiRes())
    {
        x *= HIRES_COORD_MULTIPLIER;
        y *= HIRES_COORD_MULTIPLIER;
    }
}

AGS_INLINE void ctx_data_to_game_size(int &w, int &h, bool hires_ctx)
{
    if (hires_ctx && !game.IsHiRes())
    {
        w = Math::Max(1, (w / HIRES_COORD_MULTIPLIER));
        h = Math::Max(1, (h / HIRES_COORD_MULTIPLIER));
    }
    else if (!hires_ctx && game.IsHiRes())
    {
        w *= HIRES_COORD_MULTIPLIER;
        h *= HIRES_COORD_MULTIPLIER;
    }
}

AGS_INLINE int ctx_data_to_game_size(int size, bool hires_ctx)
{
    if (hires_ctx && !game.IsHiRes())
        return Math::Max(1, (size / HIRES_COORD_MULTIPLIER));
    if (!hires_ctx && game.IsHiRes())
        return size * HIRES_COORD_MULTIPLIER;
    return size;
}

AGS_INLINE int game_to_ctx_data_size(int size, bool hires_ctx)
{
    if (hires_ctx && !game.IsHiRes())
        return size * HIRES_COORD_MULTIPLIER;
    else if (!hires_ctx && game.IsHiRes())
        return Math::Max(1, (size / HIRES_COORD_MULTIPLIER));
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
        Bitmap *blank = BitmapHelper::CreateBitmap(16, 16, coldepth);
        blank = ReplaceBitmapWithSupportedFormat(blank);
        blank->Clear();
        blankImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
        blankSidebarImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
        delete blank;
    }
    catch (Ali3DException gfxException)
    {
        quit((char*)gfxException._message);
    }
}

void destroy_blank_image()
{
    if (blankImage)
        gfxDriver->DestroyDDB(blankImage);
    if (blankSidebarImage)
        gfxDriver->DestroyDDB(blankSidebarImage);
    blankImage = NULL;
    blankSidebarImage = NULL;
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
    on_roomviewport_changed();
    on_camera_size_changed();
}

void dispose_draw_method()
{
    dispose_room_drawdata();
    destroy_invalid_regions();
    destroy_blank_image();
}

void dispose_room_drawdata()
{
    RoomCameraBuffer.reset();
    RoomCameraFrame.reset();
}

void on_mainviewport_changed()
{
    if (gfxDriver->UsesMemoryBackBuffer())
    {
        const Rect &main_view = play.GetMainViewport();
        gfxDriver->SetMemoryBackBuffer(NULL); // make it restore original virtual screen
        if (main_view.GetSize() != game.GetGameRes())
        {
            delete sub_vscreen;
            sub_vscreen = BitmapHelper::CreateSubBitmap(gfxDriver->GetMemoryBackBuffer(), main_view);
            gfxDriver->SetMemoryBackBuffer(sub_vscreen, main_view.Left, main_view.Top);
        }
    }

    if (!gfxDriver->RequiresFullRedrawEachFrame())
    {
        init_invalid_regions(-1, play.GetMainViewport().GetSize(), RectWH(play.GetMainViewport().GetSize()));
        if (game.GetGameRes().ExceedsByAny(play.GetMainViewport().GetSize()))
            clear_letterbox_borders();
    }
}

// Syncs room viewport and camera in case anything has changed
void sync_roomview()
{
    const Size &cam_sz = play.GetRoomCamera().GetSize();
    const Size &view_sz = play.GetRoomViewport().GetSize();
    init_invalid_regions(0, cam_sz, play.GetRoomViewport());
    if (cam_sz == view_sz)
    { // note we keep the buffer allocated in case it will become useful later
        RoomCameraFrame.reset();
    }
    else
    {
        if (!RoomCameraBuffer || RoomCameraBuffer->GetWidth() < cam_sz.Width || RoomCameraBuffer->GetHeight() < cam_sz.Height)
        {
            // Allocate new buffer bitmap with an extra size in case they will want to zoom out
            int room_width = data_to_game_coord(thisroom.Width);
            int room_height = data_to_game_coord(thisroom.Height);
            Size alloc_sz = Size::Clamp(cam_sz * 2, Size(1, 1), Size(room_width, room_height));
            RoomCameraBuffer.reset(new Bitmap(alloc_sz.Width, alloc_sz.Height));
        }

        if (!RoomCameraFrame || RoomCameraFrame->GetSize() != cam_sz)
        {
            RoomCameraFrame.reset(BitmapHelper::CreateSubBitmap(RoomCameraBuffer.get(), RectWH(cam_sz)));
        }
    }
}

void on_roomviewport_changed()
{
    if (!gfxDriver->RequiresFullRedrawEachFrame())
    {
        sync_roomview();
        invalidate_screen();
        // TODO: don't have to do this all the time, perhaps do "dirty rect" method
        // and only clear previous viewport location?
        gfxDriver->GetMemoryBackBuffer()->Clear();
    }
}

void on_camera_size_changed()
{
    if (!gfxDriver->RequiresFullRedrawEachFrame())
    {
        sync_roomview();
        invalidate_screen();
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

void invalidate_rect(int x1, int y1, int x2, int y2, bool in_room)
{
    //if (!in_room)
    invalidate_rect_ds(x1, y1, x2, y2, in_room);
}

void invalidate_sprite(int x1, int y1, IDriverDependantBitmap *pic, bool in_room)
{
    //if (!in_room)
    invalidate_rect_ds(x1, y1, x1 + pic->GetWidth(), y1 + pic->GetHeight(), in_room);
}

void mark_current_background_dirty()
{
    current_background_is_dirty = true;
}


void draw_and_invalidate_text(Bitmap *ds, int x1, int y1, int font, color_t text_color, const char *text)
{
    wouttext_outline(ds, x1, y1, font, text_color, (char*)text);
    invalidate_rect(x1, y1, x1 + wgettextwidth_compensate(text, font), y1 + getfontheight_outlined(font) + get_fixed_pixel_size(1), false);
}

void render_black_borders(int atx, int aty)
{
    const Rect &viewport = play.GetMainViewport();
    if (!gfxDriver->UsesMemoryBackBuffer())
    {
        if (aty > 0)
        {
            // letterbox borders
            blankImage->SetStretch(game.GetGameRes().Width, aty, false);
            gfxDriver->DrawSprite(-atx, -aty, blankImage);
            gfxDriver->DrawSprite(0, viewport.GetHeight(), blankImage);
        }
        if (atx > 0)
        {
            // sidebar borders for widescreen
            blankSidebarImage->SetStretch(atx, viewport.GetHeight(), false);
            gfxDriver->DrawSprite(-atx, 0, blankSidebarImage);
            gfxDriver->DrawSprite(viewport.GetWidth(), 0, blankSidebarImage);
        }
    }
}


void render_to_screen(int atx, int aty)
{
    gfxDriver->SetNativeRenderOffset(atx, aty);
    const Rect &viewport = play.GetMainViewport();
    // For software renderer, need to blacken upper part of the game frame when shaking screen moves image down
    if (aty > 0 && wasShakingScreen && gfxDriver->UsesMemoryBackBuffer())
        gfxDriver->ClearRectangle(viewport.Left, viewport.Top, viewport.GetWidth() - 1, aty, NULL);
    render_black_borders(atx, aty);

    if(pl_any_want_hook(AGSE_FINALSCREENDRAW))
        gfxDriver->DrawSprite(AGSE_FINALSCREENDRAW, 0, NULL);

    // only vsync in full screen mode, it makes things worse
    // in a window
    gfxDriver->EnableVsyncBeforeRender((scsystem.vsync > 0) && (!scsystem.windowed));

    bool succeeded = false;
    while (!succeeded)
    {
        try
        {
            gfxDriver->Render((GlobalFlipType)play.screen_flipped);

#if defined(ANDROID_VERSION)
            if (game.color_depth == 1)
                android_render();
#elif defined(IOS_VERSION)
            if (game.color_depth == 1)
                ios_render();
#endif

            succeeded = true;
        }
        catch (Ali3DFullscreenLostException) 
        { 
            platform->Delay(500);
        }
    }
}

// Blanks out borders around main viewport in case it became smaller (e.g. after loading another room)
void clear_letterbox_borders()
{
    const Rect &viewport = play.GetMainViewport();
    gfxDriver->ClearRectangle(0, 0, game.GetGameRes().Width - 1, viewport.Top - 1, NULL);
    gfxDriver->ClearRectangle(0, viewport.Bottom + 1, game.GetGameRes().Width - 1, game.GetGameRes().Height - 1, NULL);
}

// writes the virtual screen to the screen, converting colours if
// necessary
void write_screen() {

    if (play.fast_forward)
        return;

    int at_yp = 0;

    if (play.shakesc_length > 0) {
        wasShakingScreen = 1;
        if ( (loopcounter % play.shakesc_delay) < (play.shakesc_delay / 2) )
            at_yp = data_to_game_coord(play.shakesc_amount);
        invalidate_screen();
    }
    else if (wasShakingScreen) {
        wasShakingScreen = 0;

        if (!gfxDriver->RequiresFullRedrawEachFrame())
        {
            clear_letterbox_borders();
        }
    }

    if (play.screen_tint < 1)
        gfxDriver->SetScreenTint(0, 0, 0);
    else
        gfxDriver->SetScreenTint(play.screen_tint & 0xff, (play.screen_tint >> 8) & 0xff, (play.screen_tint >> 16) & 0xff);

    render_to_screen(0, at_yp);
}



void draw_screen_callback()
{
    construct_virtual_screen(false);

    render_black_borders(play.GetMainViewport().Left, play.GetMainViewport().Top);
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

    if (game.options[OPT_SPRITEALPHA] == kSpriteAlphaRender_Improved)
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


IDriverDependantBitmap* recycle_ddb_bitmap(IDriverDependantBitmap *bimp, Bitmap *source, bool hasAlpha, bool opaque) {
    if (bimp != NULL) {
        // same colour depth, width and height -> reuse
        if (((bimp->GetColorDepth() + 1) / 8 == source->GetBPP()) && 
            (bimp->GetWidth() == source->GetWidth()) && (bimp->GetHeight() == source->GetHeight()))
        {
            gfxDriver->UpdateDDBFromBitmap(bimp, source, hasAlpha);
            return bimp;
        }

        gfxDriver->DestroyDDB(bimp);
    }
    bimp = gfxDriver->CreateDDBFromBitmap(source, hasAlpha, opaque);
    return bimp;
}

void invalidate_cached_walkbehinds() 
{
    memset(&actspswbcache[0], 0, sizeof(CachedActSpsData) * actSpsCount);
}

// sort_out_walk_behinds: modifies the supplied sprite by overwriting parts
// of it with transparent pixels where there are walk-behind areas
// Returns whether any pixels were updated
int sort_out_walk_behinds(Bitmap *sprit,int xx,int yy,int basel, Bitmap *copyPixelsFrom = NULL, Bitmap *checkPixelsFrom = NULL, int zoom=100) {
    if (noWalkBehindsAtAll)
        return 0;

    if ((!thisroom.WalkBehindMask->IsMemoryBitmap()) ||
        (!sprit->IsMemoryBitmap()))
        quit("!sort_out_walk_behinds: wb bitmap not linear");

    int rr,tmm, toheight;//,tcol;
    // precalculate this to try and shave some time off
    int maskcol = sprit->GetMaskColor();
    int spcoldep = sprit->GetColorDepth();
    int screenhit = thisroom.WalkBehindMask->GetHeight();
    short *shptr, *shptr2;
    int *loptr, *loptr2;
    int pixelsChanged = 0;
    int ee = 0;
    if (xx < 0)
        ee = 0 - xx;

    if ((checkPixelsFrom != NULL) && (checkPixelsFrom->GetColorDepth() != spcoldep))
        quit("sprite colour depth does not match background colour depth");

    for ( ; ee < sprit->GetWidth(); ee++) {
        if (ee + xx >= thisroom.WalkBehindMask->GetWidth())
            break;

        if ((!walkBehindExists[ee+xx]) ||
            (walkBehindEndY[ee+xx] <= yy) ||
            (walkBehindStartY[ee+xx] > yy+sprit->GetHeight()))
            continue;

        toheight = sprit->GetHeight();

        if (walkBehindStartY[ee+xx] < yy)
            rr = 0;
        else
            rr = (walkBehindStartY[ee+xx] - yy);

        // Since we will use _getpixel, ensure we only check within the screen
        if (rr + yy < 0)
            rr = 0 - yy;
        if (toheight + yy > screenhit)
            toheight = screenhit - yy;
        if (toheight + yy > walkBehindEndY[ee+xx])
            toheight = walkBehindEndY[ee+xx] - yy;
        if (rr < 0)
            rr = 0;

        for ( ; rr < toheight;rr++) {

            // we're ok with _getpixel because we've checked the screen edges
            //tmm = _getpixel(thisroom.WalkBehindMask,ee+xx,rr+yy);
            // actually, _getpixel is well inefficient, do it ourselves
            // since we know it's 8-bit bitmap
            tmm = thisroom.WalkBehindMask->GetScanLine(rr+yy)[ee+xx];
            if (tmm<1) continue;
            if (croom->walkbehind_base[tmm] <= basel) continue;

            if (copyPixelsFrom != NULL)
            {
                if (spcoldep <= 8)
                {
                    if (checkPixelsFrom->GetScanLine((rr * 100) / zoom)[(ee * 100) / zoom] != maskcol) {
                        sprit->GetScanLineForWriting(rr)[ee] = copyPixelsFrom->GetScanLine(rr + yy)[ee + xx];
                        pixelsChanged = 1;
                    }
                }
                else if (spcoldep <= 16) {
                    shptr = (short*)&sprit->GetScanLine(rr)[0];
                    shptr2 = (short*)&checkPixelsFrom->GetScanLine((rr * 100) / zoom)[0];
                    if (shptr2[(ee * 100) / zoom] != maskcol) {
                        shptr[ee] = ((short*)(&copyPixelsFrom->GetScanLine(rr + yy)[0]))[ee + xx];
                        pixelsChanged = 1;
                    }
                }
                else if (spcoldep == 24) {
                    char *chptr = (char*)&sprit->GetScanLine(rr)[0];
                    char *chptr2 = (char*)&checkPixelsFrom->GetScanLine((rr * 100) / zoom)[0];
                    if (memcmp(&chptr2[((ee * 100) / zoom) * 3], &maskcol, 3) != 0) {
                        memcpy(&chptr[ee * 3], &copyPixelsFrom->GetScanLine(rr + yy)[(ee + xx) * 3], 3);
                        pixelsChanged = 1;
                    }
                }
                else if (spcoldep <= 32) {
                    loptr = (int*)&sprit->GetScanLine(rr)[0];
                    loptr2 = (int*)&checkPixelsFrom->GetScanLine((rr * 100) / zoom)[0];
                    if (loptr2[(ee * 100) / zoom] != maskcol) {
                        loptr[ee] = ((int*)(&copyPixelsFrom->GetScanLine(rr + yy)[0]))[ee + xx];
                        pixelsChanged = 1;
                    }
                }
            }
            else
            {
                pixelsChanged = 1;
                if (spcoldep <= 8)
                    sprit->GetScanLineForWriting(rr)[ee] = maskcol;
                else if (spcoldep <= 16) {
                    shptr = (short*)&sprit->GetScanLine(rr)[0];
                    shptr[ee] = maskcol;
                }
                else if (spcoldep == 24) {
                    char *chptr = (char*)&sprit->GetScanLine(rr)[0];
                    memcpy(&chptr[ee * 3], &maskcol, 3);
                }
                else if (spcoldep <= 32) {
                    loptr = (int*)&sprit->GetScanLine(rr)[0];
                    loptr[ee] = maskcol;
                }
                else
                    quit("!Sprite colour depth >32 ??");
            }
        }
    }
    return pixelsChanged;
}

void sort_out_char_sprite_walk_behind(int actspsIndex, int xx, int yy, int basel, int zoom, int width, int height)
{
    if (noWalkBehindsAtAll)
        return;

    if ((!actspswbcache[actspsIndex].valid) ||
        (actspswbcache[actspsIndex].xWas != xx) ||
        (actspswbcache[actspsIndex].yWas != yy) ||
        (actspswbcache[actspsIndex].baselineWas != basel))
    {
        actspswb[actspsIndex] = recycle_bitmap(actspswb[actspsIndex], thisroom.BgFrames[play.bg_frame].Graphic->GetColorDepth(), width, height, true);
        Bitmap *wbSprite = actspswb[actspsIndex];

        actspswbcache[actspsIndex].isWalkBehindHere = sort_out_walk_behinds(wbSprite, xx, yy, basel, thisroom.BgFrames[play.bg_frame].Graphic.get(), actsps[actspsIndex], zoom);
        actspswbcache[actspsIndex].xWas = xx;
        actspswbcache[actspsIndex].yWas = yy;
        actspswbcache[actspsIndex].baselineWas = basel;
        actspswbcache[actspsIndex].valid = 1;

        if (actspswbcache[actspsIndex].isWalkBehindHere)
        {
            actspswbbmp[actspsIndex] = recycle_ddb_bitmap(actspswbbmp[actspsIndex], actspswb[actspsIndex], false);
        }
    }

    if (actspswbcache[actspsIndex].isWalkBehindHere)
    {
        // TODO: perhaps do not add camera position here, instead let the renderer do coordinate transform
        add_to_sprite_list(actspswbbmp[actspsIndex], xx - play.GetRoomCamera().Left, yy - play.GetRoomCamera().Top, basel, 0, -1, true);
    }
}

void clear_draw_list() {
    thingsToDrawList.clear();
}
void add_thing_to_draw(IDriverDependantBitmap* bmp, int x, int y, int trans, bool alphaChannel) {
    SpriteListEntry sprite;
    sprite.pic = NULL;
    sprite.bmp = bmp;
    sprite.x = x;
    sprite.y = y;
    sprite.transparent = trans;
    sprite.hasAlphaChannel = alphaChannel;
    thingsToDrawList.push_back(sprite);
}

// the sprite list is an intermediate list used to order 
// objects and characters by their baselines before everything
// is added to the Thing To Draw List
void clear_sprite_list() {
    sprlist.clear();
}
void add_to_sprite_list(IDriverDependantBitmap* spp, int xx, int yy, int baseline, int trans, int sprNum, bool isWalkBehind) {

    if (spp == NULL)
        quit("add_to_sprite_list: attempted to draw NULL sprite");
    // completely invisible, so don't draw it at all
    if (trans == 255)
        return;

    SpriteListEntry sprite;
    if ((sprNum >= 0) && ((game.SpriteInfos[sprNum].Flags & SPF_ALPHACHANNEL) != 0))
        sprite.hasAlphaChannel = true;
    else
        sprite.hasAlphaChannel = false;

    sprite.bmp = spp;
    sprite.baseline = baseline;
    sprite.x=xx;
    sprite.y=yy;
    sprite.transparent=trans;

    if (walkBehindMethod == DrawAsSeparateSprite)
        sprite.takesPriorityIfEqual = !isWalkBehind;
    else
        sprite.takesPriorityIfEqual = isWalkBehind;

    sprlist.push_back(sprite);
}

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
void draw_gui_sprite(Bitmap *ds, int pic, int x, int y, bool use_alpha, BlendMode blend_mode) 
{
    Bitmap *sprite = spriteset[pic];
    const bool ds_has_alpha  = ds->GetColorDepth() == 32;
    const bool src_has_alpha = (game.SpriteInfos[pic].Flags & SPF_ALPHACHANNEL) != 0;

    if (use_alpha && game.options[OPT_NEWGUIALPHA] == kGuiAlphaRender_Improved)
    {
        GfxUtil::DrawSpriteBlend(ds, Point(x, y), sprite, blend_mode, ds_has_alpha, src_has_alpha);
    }
    // Backwards-compatible drawing
    else if (use_alpha && ds_has_alpha && game.options[OPT_NEWGUIALPHA] == kGuiAlphaRender_AdditiveAlpha)
    {
        if (src_has_alpha)
            set_additive_alpha_blender();
        else
            set_opaque_alpha_blender();
        ds->TransBlendBlt(sprite, x, y);
    }
    else
    {
        GfxUtil::DrawSpriteWithTransparency(ds, sprite, x, y);
    }
}

void draw_gui_sprite_v330(Bitmap *ds, int pic, int x, int y, bool use_alpha, BlendMode blend_mode)
{
    draw_gui_sprite(ds, pic, x, y, use_alpha && (loaded_game_file_version >= kGameVersion_330), blend_mode);
}

// function to sort the sprites into baseline order
bool spritelistentry_less(const SpriteListEntry &e1, const SpriteListEntry &e2)
{
    if (e1.baseline == e2.baseline) 
    { 
        if (e1.takesPriorityIfEqual)
            return false;
        if (e2.takesPriorityIfEqual)
            return true;
    }
    return e1.baseline < e2.baseline;
}




void draw_sprite_list() {

    if (walkBehindMethod == DrawAsSeparateSprite)
    {
        for (int ee = 1; ee < MAX_WALK_BEHINDS; ee++)
        {
            if (walkBehindBitmap[ee] != NULL)
            {
                // TODO: perhaps do not add camera position here, instead let the renderer do coordinate transform
                add_to_sprite_list(walkBehindBitmap[ee], walkBehindLeft[ee] - play.GetRoomCamera().Left, walkBehindTop[ee] - play.GetRoomCamera().Top,
                    croom->walkbehind_base[ee], 0, -1, true);
            }
        }
    }

    std::sort(sprlist.begin(), sprlist.end(), spritelistentry_less);

    clear_draw_list();

    if(pl_any_want_hook(AGSE_PRESCREENDRAW))
        add_thing_to_draw(NULL, AGSE_PRESCREENDRAW, 0, TRANS_RUN_PLUGIN, false);

    // copy the sorted sprites into the Things To Draw list
    thingsToDrawList.insert(thingsToDrawList.end(), sprlist.begin(), sprlist.end());
}

// Avoid freeing and reallocating the memory if possible
Bitmap *recycle_bitmap(Bitmap *bimp, int coldep, int wid, int hit, bool make_transparent) {
    if (bimp != NULL) {
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
// sprite indexed with actspsindex
void apply_tint_or_light(int actspsindex, int light_level,
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
 if (game.GetColorDepth() == actsps[actspsindex]->GetColorDepth()) {
     Bitmap *oldwas;
     // if the caller supplied a source bitmap, ->Blit from it
     // (used as a speed optimisation where possible)
     if (blitFrom) 
         oldwas = blitFrom;
     // otherwise, make a new target bmp
     else {
         oldwas = actsps[actspsindex];
         actsps[actspsindex] = BitmapHelper::CreateBitmap(oldwas->GetWidth(), oldwas->GetHeight(), coldept);
     }
     Bitmap *active_spr = actsps[actspsindex];

     if (tint_amount) {
         // It is an RGB tint
         tint_image (active_spr, oldwas, tint_red, tint_green, tint_blue, tint_amount, tint_light);
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

         active_spr->LitBlendBlt(oldwas, 0, 0, lit_amnt);
     }

     if (oldwas != blitFrom)
         delete oldwas;

 }
 else if (blitFrom) {
     // sprite colour depth != game colour depth, so don't try and tint
     // but we do need to do something, so copy the source
     Bitmap *active_spr = actsps[actspsindex];
     active_spr->Blit(blitFrom, 0, 0, 0, 0, active_spr->GetWidth(), active_spr->GetHeight());
 }

}

// Draws the specified 'sppic' sprite onto actsps[useindx] at the
// specified width and height, and flips the sprite if necessary.
// Returns 1 if something was drawn to actsps; returns 0 if no
// scaling or stretching was required, in which case nothing was done
int scale_and_flip_sprite(int useindx, int coldept, int zoom_level,
                          int sppic, int newwidth, int newheight,
                          int isMirrored) {

  int actsps_used = 1;

  // create and blank out the new sprite
  actsps[useindx] = recycle_bitmap(actsps[useindx], coldept, newwidth, newheight, true);
  Bitmap *active_spr = actsps[useindx];

  if (zoom_level != 100) {
      // Scaled character

      our_eip = 334;

      // Ensure that anti-aliasing routines have a palette to
      // use for mapping while faded out
      if (in_new_room)
          select_palette (palette);


      if (isMirrored) {
          Bitmap *tempspr = BitmapHelper::CreateBitmap(newwidth, newheight,coldept);
          tempspr->Fill (actsps[useindx]->GetMaskColor());
          if ((IS_ANTIALIAS_SPRITES) && ((game.SpriteInfos[sppic].Flags & SPF_ALPHACHANNEL) == 0))
              tempspr->AAStretchBlt (spriteset[sppic], RectWH(0, 0, newwidth, newheight), Common::kBitmap_Transparency);
          else
              tempspr->StretchBlt (spriteset[sppic], RectWH(0, 0, newwidth, newheight), Common::kBitmap_Transparency);
          active_spr->FlipBlt(tempspr, 0, 0, Common::kBitmap_HFlip);
          delete tempspr;
      }
      else if ((IS_ANTIALIAS_SPRITES) && ((game.SpriteInfos[sppic].Flags & SPF_ALPHACHANNEL) == 0))
          active_spr->AAStretchBlt(spriteset[sppic],RectWH(0,0,newwidth,newheight), Common::kBitmap_Transparency);
      else
          active_spr->StretchBlt(spriteset[sppic],RectWH(0,0,newwidth,newheight), Common::kBitmap_Transparency);

      /*  AASTR2 version of code (doesn't work properly, gives black borders)
      if (IS_ANTIALIAS_SPRITES) {
      int aa_mode = AA_MASKED; 
      if (game.spriteflags[sppic] & SPF_ALPHACHANNEL)
      aa_mode |= AA_ALPHA | AA_RAW_ALPHA;
      if (isMirrored)
      aa_mode |= AA_HFLIP;

      aa_set_mode(aa_mode);
      ->AAStretchBlt(actsps[useindx],spriteset[sppic],0,0,newwidth,newheight);
      }
      else if (isMirrored) {
      Bitmap *tempspr = BitmapHelper::CreateBitmap_ (coldept, newwidth, newheight);
      ->Clear (tempspr, ->GetMaskColor(actsps[useindx]));
      ->StretchBlt (tempspr, spriteset[sppic], 0, 0, newwidth, newheight);
      ->FlipBlt(Common::kBitmap_HFlip, (actsps[useindx], tempspr, 0, 0);
      wfreeblock (tempspr);
      }
      else
      ->StretchBlt(actsps[useindx],spriteset[sppic],0,0,newwidth,newheight);
      */
      if (in_new_room)
          unselect_palette();

  } 
  else {
      // Not a scaled character, draw at normal size

      our_eip = 339;

      if (isMirrored)
          active_spr->FlipBlt(spriteset[sppic], 0, 0, Common::kBitmap_HFlip);
      else
          actsps_used = 0;
      //->Blit (spriteset[sppic], actsps[useindx], 0, 0, 0, 0, actsps[useindx]->GetWidth(), actsps[useindx]->GetHeight());
  }

  return actsps_used;
}



// create the actsps[aa] image with the object drawn correctly
// returns 1 if nothing at all has changed and actsps is still
// intact from last time; 0 otherwise
int construct_object_gfx(int aa, int *drawnWidth, int *drawnHeight, bool alwaysUseSoftware) {
    int useindx = aa;
    bool hardwareAccelerated = !alwaysUseSoftware && gfxDriver->HasAcceleratedTransform();

    if (spriteset[objs[aa].num] == NULL)
        quitprintf("There was an error drawing object %d. Its current sprite, %d, is invalid.", aa, objs[aa].num);

    int coldept = spriteset[objs[aa].num]->GetColorDepth();
    int sprwidth = game.SpriteInfos[objs[aa].num].Width;
    int sprheight = game.SpriteInfos[objs[aa].num].Height;

    int tint_red, tint_green, tint_blue;
    int tint_level, tint_light, light_level;
    int zoom_level = 100;

    // calculate the zoom level
    if (objs[aa].flags & OBJF_USEROOMSCALING) {
        int onarea = get_walkable_area_at_location(objs[aa].x, objs[aa].y);

        if ((onarea <= 0) && (thisroom.WalkAreas[0].ScalingFar == 0)) {
            // just off the edge of an area -- use the scaling we had
            // while on the area
            zoom_level = objs[aa].last_zoom;
        }
        else
            zoom_level = get_area_scaling(onarea, objs[aa].x, objs[aa].y);

        if (zoom_level != 100)
            scale_sprite_size(objs[aa].num, zoom_level, &sprwidth, &sprheight);

    }
    // save the zoom level for next time
    objs[aa].last_zoom = zoom_level;

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
    int isMirrored = 0;
    if ( (objs[aa].view >= 0) &&
        (views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame].pic == objs[aa].num) &&
        ((views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame].flags & VFLG_FLIPSPRITE) != 0)) {
            isMirrored = 1;
    }

    if ((hardwareAccelerated) &&
        (walkBehindMethod != DrawOverCharSprite) &&
        (objcache[aa].image != NULL) &&
        (objcache[aa].sppic == objs[aa].num) &&
        (actsps[useindx] != NULL))
    {
        // HW acceleration
        objcache[aa].tintamntwas = tint_level;
        objcache[aa].tintredwas = tint_red;
        objcache[aa].tintgrnwas = tint_green;
        objcache[aa].tintbluwas = tint_blue;
        objcache[aa].tintlightwas = tint_light;
        objcache[aa].lightlevwas = light_level;
        objcache[aa].zoomWas = zoom_level;
        objcache[aa].mirroredWas = isMirrored;

        return 1;
    }

    if ((!hardwareAccelerated) && (gfxDriver->HasAcceleratedTransform()))
    {
        // They want to draw it in software mode with the D3D driver,
        // so force a redraw
        objcache[aa].sppic = -389538;
    }

    // If we have the image cached, use it
    if ((objcache[aa].image != NULL) &&
        (objcache[aa].sppic == objs[aa].num) &&
        (objcache[aa].tintamntwas == tint_level) &&
        (objcache[aa].tintlightwas == tint_light) &&
        (objcache[aa].tintredwas == tint_red) &&
        (objcache[aa].tintgrnwas == tint_green) &&
        (objcache[aa].tintbluwas == tint_blue) &&
        (objcache[aa].lightlevwas == light_level) &&
        (objcache[aa].zoomWas == zoom_level) &&
        (objcache[aa].mirroredWas == isMirrored)) {
            // the image is the same, we can use it cached!
            if ((walkBehindMethod != DrawOverCharSprite) &&
                (actsps[useindx] != NULL))
                return 1;
            // Check if the X & Y co-ords are the same, too -- if so, there
            // is scope for further optimisations
            if ((objcache[aa].xwas == objs[aa].x) &&
                (objcache[aa].ywas == objs[aa].y) &&
                (actsps[useindx] != NULL) &&
                (walk_behind_baselines_changed == 0))
                return 1;
            actsps[useindx] = recycle_bitmap(actsps[useindx], coldept, sprwidth, sprheight);
            actsps[useindx]->Blit(objcache[aa].image, 0, 0, 0, 0, objcache[aa].image->GetWidth(), objcache[aa].image->GetHeight());
            return 0;
    }

    // Not cached, so draw the image

    int actspsUsed = 0;
    if (!hardwareAccelerated)
    {
        // draw the base sprite, scaled and flipped as appropriate
        actspsUsed = scale_and_flip_sprite(useindx, coldept, zoom_level,
            objs[aa].num, sprwidth, sprheight, isMirrored);
    }
    else
    {
        // ensure actsps exists
        actsps[useindx] = recycle_bitmap(actsps[useindx], coldept, game.SpriteInfos[objs[aa].num].Width, game.SpriteInfos[objs[aa].num].Height);
    }

    // direct read from source bitmap, where possible
    Bitmap *comeFrom = NULL;
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
        actsps[useindx]->Blit(spriteset[objs[aa].num],0,0,0,0,game.SpriteInfos[objs[aa].num].Width, game.SpriteInfos[objs[aa].num].Height);
    }

    // Re-use the bitmap if it's the same size
    objcache[aa].image = recycle_bitmap(objcache[aa].image, coldept, sprwidth, sprheight);
    // Create the cached image and store it
    objcache[aa].image->Blit(actsps[useindx], 0, 0, 0, 0, sprwidth, sprheight);
    objcache[aa].sppic = objs[aa].num;
    objcache[aa].tintamntwas = tint_level;
    objcache[aa].tintredwas = tint_red;
    objcache[aa].tintgrnwas = tint_green;
    objcache[aa].tintbluwas = tint_blue;
    objcache[aa].tintlightwas = tint_light;
    objcache[aa].lightlevwas = light_level;
    objcache[aa].zoomWas = zoom_level;
    objcache[aa].mirroredWas = isMirrored;
    return 0;
}




// This is only called from draw_screen_background, but it's seperated
// to help with profiling the program
void prepare_objects_for_drawing() {
    int aa,atxp,atyp,useindx;
    our_eip=32;

    for (aa=0;aa<croom->numobj;aa++) {
        if (objs[aa].on != 1) continue;
        // offscreen, don't draw
        if ((objs[aa].x >= thisroom.Width) || (objs[aa].y < 1))
            continue;

        useindx = aa;
        int tehHeight;

        int actspsIntact = construct_object_gfx(aa, NULL, &tehHeight, false);

        // update the cache for next time
        objcache[aa].xwas = objs[aa].x;
        objcache[aa].ywas = objs[aa].y;

        // TODO: perhaps do not add camera position here, instead let the renderer do coordinate transform
        const Rect &camera = play.GetRoomCamera();
        int offsetx = camera.Left;
        int offsety = camera.Top;
        atxp = data_to_game_coord(objs[aa].x) - offsetx;
        atyp = (data_to_game_coord(objs[aa].y) - tehHeight) - offsety;

        int usebasel = objs[aa].get_baseline();

        if (objs[aa].flags & OBJF_NOWALKBEHINDS) {
            // ignore walk-behinds, do nothing
            if (walkBehindMethod == DrawAsSeparateSprite)
            {
                usebasel += thisroom.Height;
            }
        }
        else if (walkBehindMethod == DrawAsSeparateCharSprite) 
        {
            sort_out_char_sprite_walk_behind(useindx, atxp+offsetx, atyp+offsety, usebasel, objs[aa].last_zoom, objs[aa].last_width, objs[aa].last_height);
        }
        else if ((!actspsIntact) && (walkBehindMethod == DrawOverCharSprite))
        {
            sort_out_walk_behinds(actsps[useindx],atxp+offsetx,atyp+offsety,usebasel);
        }

        if ((!actspsIntact) || (actspsbmp[useindx] == NULL))
        {
            bool hasAlpha = (game.SpriteInfos[objs[aa].num].Flags & SPF_ALPHACHANNEL) != 0;

            if (actspsbmp[useindx] != NULL)
                gfxDriver->DestroyDDB(actspsbmp[useindx]);
            actspsbmp[useindx] = gfxDriver->CreateDDBFromBitmap(actsps[useindx], hasAlpha);
        }

        if (gfxDriver->HasAcceleratedTransform())
        {
            actspsbmp[useindx]->SetFlippedLeftRight(objcache[aa].mirroredWas != 0);
            actspsbmp[useindx]->SetStretch(objs[aa].last_width, objs[aa].last_height);
            actspsbmp[useindx]->SetTint(objcache[aa].tintredwas, objcache[aa].tintgrnwas, objcache[aa].tintbluwas, (objcache[aa].tintamntwas * 256) / 100);

            if (objcache[aa].tintamntwas > 0)
            {
                if (objcache[aa].tintlightwas == 0)  // luminance of 0 -- pass 1 to enable
                    actspsbmp[useindx]->SetLightLevel(1);
                else if (objcache[aa].tintlightwas < 250)
                    actspsbmp[useindx]->SetLightLevel(objcache[aa].tintlightwas);
                else
                    actspsbmp[useindx]->SetLightLevel(0);
            }
            else if (objcache[aa].lightlevwas != 0)
                actspsbmp[useindx]->SetLightLevel((objcache[aa].lightlevwas * 25) / 10 + 256);
            else
                actspsbmp[useindx]->SetLightLevel(0);
        }

        add_to_sprite_list(actspsbmp[useindx],atxp,atyp,usebasel,objs[aa].transparent,objs[aa].num);
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
    int zoom_level,newwidth,newheight,onarea,sppic,atxp,atyp,useindx;
    int light_level,coldept,aa;
    int tint_red, tint_green, tint_blue, tint_amount, tint_light = 255;

    our_eip=33;

    // TODO: perhaps do not add camera position here, instead let the renderer do coordinate transform
    const Rect &camera = play.GetRoomCamera();
    int offsetx = camera.Left;
    int offsety = camera.Top;

    // draw characters
    for (aa=0;aa<game.numcharacters;aa++) {
        if (game.chars[aa].on==0) continue;
        if (game.chars[aa].room!=displayed_room) continue;
        eip_guinum = aa;
        useindx = aa + MAX_ROOM_OBJECTS;

        CharacterInfo*chin=&game.chars[aa];
        our_eip = 330;
        // if it's on but set to view -1, they're being silly
        if (chin->view < 0) {
            quitprintf("!The character '%s' was turned on in the current room (room %d) but has not been assigned a view number.",
                chin->name, displayed_room);
        }

        if (chin->frame >= views[chin->view].loops[chin->loop].numFrames)
            chin->frame = 0;

        if ((chin->loop >= views[chin->view].numLoops) ||
            (views[chin->view].loops[chin->loop].numFrames < 1)) {
                quitprintf("!The character '%s' could not be displayed because there were no frames in loop %d of view %d.",
                    chin->name, chin->loop, chin->view + 1);
        }

        sppic=views[chin->view].loops[chin->loop].frames[chin->frame].pic;
        if (sppic < 0)
            sppic = 0;  // in case it's screwed up somehow
        our_eip = 331;
        // sort out the stretching if required
        onarea = get_walkable_area_at_character (aa);
        our_eip = 332;

        if (chin->flags & CHF_MANUALSCALING)  // character ignores scaling
            zoom_level = charextra[aa].zoom;
        else if ((onarea <= 0) && (thisroom.WalkAreas[0].ScalingFar == 0)) {
            zoom_level = charextra[aa].zoom;
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

        /*if (actsps[useindx]!=NULL) {
        wfreeblock(actsps[useindx]);
        actsps[useindx] = NULL;
        }*/

        our_eip = 3330;
        int isMirrored = 0, specialpic = sppic;
        bool usingCachedImage = false;

        coldept = spriteset[sppic]->GetColorDepth();

        // adjust the sppic if mirrored, so it doesn't accidentally
        // cache the mirrored frame as the real one
        if (views[chin->view].loops[chin->loop].frames[chin->frame].flags & VFLG_FLIPSPRITE) {
            isMirrored = 1;
            specialpic = -sppic;
        }

        our_eip = 3331;

        // if the character was the same sprite and scaling last time,
        // just use the cached image
        if ((charcache[aa].inUse) &&
            (charcache[aa].sppic == specialpic) &&
            (charcache[aa].scaling == zoom_level) &&
            (charcache[aa].tintredwas == tint_red) &&
            (charcache[aa].tintgrnwas == tint_green) &&
            (charcache[aa].tintbluwas == tint_blue) &&
            (charcache[aa].tintamntwas == tint_amount) &&
            (charcache[aa].tintlightwas == tint_light) &&
            (charcache[aa].lightlevwas == light_level)) 
        {
            if (walkBehindMethod == DrawOverCharSprite)
            {
                actsps[useindx] = recycle_bitmap(actsps[useindx], charcache[aa].image->GetColorDepth(), charcache[aa].image->GetWidth(), charcache[aa].image->GetHeight());
                actsps[useindx]->Blit (charcache[aa].image, 0, 0, 0, 0, actsps[useindx]->GetWidth(), actsps[useindx]->GetHeight());
            }
            else 
            {
                usingCachedImage = true;
            }
        }
        else if ((charcache[aa].inUse) && 
            (charcache[aa].sppic == specialpic) &&
            (gfxDriver->HasAcceleratedTransform()))
        {
            usingCachedImage = true;
        }
        else if (charcache[aa].inUse) {
            //destroy_bitmap (charcache[aa].image);
            charcache[aa].inUse = 0;
        }

        our_eip = 3332;

        if (zoom_level != 100) {
            // it needs to be stretched, so calculate the new dimensions

            scale_sprite_size(sppic, zoom_level, &newwidth, &newheight);
            charextra[aa].width=newwidth;
            charextra[aa].height=newheight;
        }
        else {
            // draw at original size, so just use the sprite width and height
            charextra[aa].width=0;
            charextra[aa].height=0;
            newwidth = game.SpriteInfos[sppic].Width;
            newheight = game.SpriteInfos[sppic].Height;
        }

        our_eip = 3336;

        // Calculate the X & Y co-ordinates of where the sprite will be
        atxp=(data_to_game_coord(chin->x) - offsetx) - newwidth/2;
        atyp=(data_to_game_coord(chin->y) - newheight) - offsety;

        charcache[aa].scaling = zoom_level;
        charcache[aa].sppic = specialpic;
        charcache[aa].tintredwas = tint_red;
        charcache[aa].tintgrnwas = tint_green;
        charcache[aa].tintbluwas = tint_blue;
        charcache[aa].tintamntwas = tint_amount;
        charcache[aa].tintlightwas = tint_light;
        charcache[aa].lightlevwas = light_level;

        // If cache needs to be re-drawn
        if (!charcache[aa].inUse) {

            // create the base sprite in actsps[useindx], which will
            // be scaled and/or flipped, as appropriate
            int actspsUsed = 0;
            if (!gfxDriver->HasAcceleratedTransform())
            {
                actspsUsed = scale_and_flip_sprite(
                    useindx, coldept, zoom_level, sppic,
                    newwidth, newheight, isMirrored);
            }
            else 
            {
                // ensure actsps exists
                actsps[useindx] = recycle_bitmap(actsps[useindx], coldept, game.SpriteInfos[sppic].Width, game.SpriteInfos[sppic].Height);
            }

            our_eip = 335;

            if (((light_level != 0) || (tint_amount != 0)) &&
                (!gfxDriver->HasAcceleratedTransform())) {
                    // apply the lightening or tinting
                    Bitmap *comeFrom = NULL;
                    // if possible, direct read from the source image
                    if (!actspsUsed)
                        comeFrom = spriteset[sppic];

                    apply_tint_or_light(useindx, light_level, tint_amount, tint_red,
                        tint_green, tint_blue, tint_light, coldept,
                        comeFrom);
            }
            else if (!actspsUsed) {
                // no scaling, flipping or tinting was done, so just blit it normally
                actsps[useindx]->Blit (spriteset[sppic], 0, 0, 0, 0, actsps[useindx]->GetWidth(), actsps[useindx]->GetHeight());
            }

            // update the character cache with the new image
            charcache[aa].inUse = 1;
            //charcache[aa].image = BitmapHelper::CreateBitmap_ (coldept, actsps[useindx]->GetWidth(), actsps[useindx]->GetHeight());
            charcache[aa].image = recycle_bitmap(charcache[aa].image, coldept, actsps[useindx]->GetWidth(), actsps[useindx]->GetHeight());
            charcache[aa].image->Blit (actsps[useindx], 0, 0, 0, 0, actsps[useindx]->GetWidth(), actsps[useindx]->GetHeight());

        } // end if !cache.inUse

        int usebasel = chin->get_baseline();

        // adjust the Y positioning for the character's Z co-ord
        atyp -= data_to_game_coord(chin->z);

        our_eip = 336;

        int bgX = atxp + offsetx + chin->pic_xoffs;
        int bgY = atyp + offsety + chin->pic_yoffs;

        if (chin->flags & CHF_NOWALKBEHINDS) {
            // ignore walk-behinds, do nothing
            if (walkBehindMethod == DrawAsSeparateSprite)
            {
                usebasel += thisroom.Height;
            }
        }
        else if (walkBehindMethod == DrawAsSeparateCharSprite) 
        {
            sort_out_char_sprite_walk_behind(useindx, bgX, bgY, usebasel, charextra[aa].zoom, newwidth, newheight);
        }
        else if (walkBehindMethod == DrawOverCharSprite)
        {
            sort_out_walk_behinds(actsps[useindx], bgX, bgY, usebasel);
        }

        if ((!usingCachedImage) || (actspsbmp[useindx] == NULL))
        {
            bool hasAlpha = (game.SpriteInfos[sppic].Flags & SPF_ALPHACHANNEL) != 0;

            actspsbmp[useindx] = recycle_ddb_bitmap(actspsbmp[useindx], actsps[useindx], hasAlpha);
        }

        if (gfxDriver->HasAcceleratedTransform()) 
        {
            actspsbmp[useindx]->SetStretch(newwidth, newheight);
            actspsbmp[useindx]->SetFlippedLeftRight(isMirrored != 0);
            actspsbmp[useindx]->SetTint(tint_red, tint_green, tint_blue, (tint_amount * 256) / 100);

            if (tint_amount != 0)
            {
                if (tint_light == 0) // tint with 0 luminance, pass as 1 instead
                    actspsbmp[useindx]->SetLightLevel(1);
                else if (tint_light < 250)
                    actspsbmp[useindx]->SetLightLevel(tint_light);
                else
                    actspsbmp[useindx]->SetLightLevel(0);
            }
            else if (light_level != 0)
                actspsbmp[useindx]->SetLightLevel((light_level * 25) / 10 + 256);
            else
                actspsbmp[useindx]->SetLightLevel(0);

        }

        our_eip = 337;
        // disable alpha blending with tinted sprites (because the
        // alpha channel was lost in the tinting process)
        //if (((tint_level) && (tint_amount < 100)) || (light_level))
        //sppic = -1;
        add_to_sprite_list(actspsbmp[useindx], atxp + chin->pic_xoffs, atyp + chin->pic_yoffs, usebasel, chin->transparency, sppic);

        chin->actx=atxp+offsetx;
        chin->acty=atyp+offsety;
    }
}



// Draws the room and its contents: background, objects, characters
//
// NOTE that the bitmap arguments are **strictly** for software rendering.
// ds is a full game screen surface, and roomcam_surface is a surface for drawing room camera content to.
// ds and roomcam_surface may be the same bitmap.
// TODO: if we support multiple cameras we would in fact need to split this function into calling
// dirty rects drawing for the full surface and room drawing on room surface.
// (maybe even split it into software/hardware variant)
// no_transform flag tells to copy dirty regions on roomcam_surface without any coordinate conversion
// whatsoever.
void draw_room(Bitmap *ds, Bitmap *roomcam_surface, bool no_transform) {
    // TODO: dont use static vars!!
    static int offsetxWas = -100, offsetyWas = -100;

    screen_reset = 1;

    if (is_complete_overlay) {
        // this is normally called as part of drawing sprites - clear it
        // here instead
        clear_draw_list();
        return;
    }

    // don't draw it before the room fades in
    /*  if ((in_new_room > 0) & (game.color_depth > 1)) {
    clear(ds);
    return;
    }*/
    our_eip=30;
    play.UpdateRoomCamera();

    our_eip=31;

    const Rect &camera = play.GetRoomCamera();
    const int offsetx = camera.Left;
    const int offsety = camera.Top;

    if ((offsetx != offsetxWas) || (offsety != offsetyWas)) {
        invalidate_screen();

        offsetxWas = offsetx;
        offsetyWas = offsety;
    }

    if (play.screen_tint >= 0)
        invalidate_screen();

    if (gfxDriver->RequiresFullRedrawEachFrame())
    {
        if (roomBackgroundBmp == NULL) 
        {
            update_polled_stuff_if_runtime();
            roomBackgroundBmp = gfxDriver->CreateDDBFromBitmap(thisroom.BgFrames[play.bg_frame].Graphic.get(), false, true);

            if ((walkBehindMethod == DrawAsSeparateSprite) && (walkBehindsCachedForBgNum != play.bg_frame))
            {
                update_walk_behind_images();
            }
        }
        else if (current_background_is_dirty)
        {
            update_polled_stuff_if_runtime();
            gfxDriver->UpdateDDBFromBitmap(roomBackgroundBmp, thisroom.BgFrames[play.bg_frame].Graphic.get(), false);
            current_background_is_dirty = false;
            if (walkBehindMethod == DrawAsSeparateSprite)
            {
                update_walk_behind_images();
            }
        }
        gfxDriver->DrawSprite(-offsetx, -offsety, roomBackgroundBmp);
    }
    else
    {
        // For software renderer: copy dirty rects onto the virtual screen.
        // TODO: that would be SUPER NICE to reorganize the code and move this operation into SoftwareGraphicDriver somehow.
        // Because basically we duplicate sprite batch transform here.

        set_invalidrects_cameraoffs(0, offsetx, offsety);

        // TODO: (by CJ)
        // the following line takes up to 50% of the game CPU time at
        // high resolutions and colour depths - if we can optimise it
        // somehow, significant performance gains to be had

        update_black_invreg_and_reset(ds);
        update_room_invreg_and_reset(0, roomcam_surface, thisroom.BgFrames[play.bg_frame].Graphic.get(), no_transform);
        // TODO: remember that we also would need to rotate here if we support camera rotation!
    }

    clear_sprite_list();

    if ((debug_flags & DBG_NOOBJECTS)==0) {

        prepare_objects_for_drawing();

        prepare_characters_for_drawing ();

        if ((debug_flags & DBG_NODRAWSPRITES)==0) {
            our_eip=34;
            draw_sprite_list();
        }
    }
    our_eip=36;

	//allegro_bitmap_test_draw();
}


void draw_fps()
{
    static IDriverDependantBitmap* ddb = NULL;
    static Bitmap *fpsDisplay = NULL;

    const Rect &ui_view = play.GetUIViewport();
    if (fpsDisplay == NULL)
    {
        fpsDisplay = BitmapHelper::CreateBitmap(ui_view.GetWidth(), (getfontheight_outlined(FONT_SPEECH) + get_fixed_pixel_size(5)), game.GetColorDepth());
        fpsDisplay = ReplaceBitmapWithSupportedFormat(fpsDisplay);
    }
    fpsDisplay->ClearTransparent();
    
    char tbuffer[60];
    sprintf(tbuffer,"FPS: %d",fps);
    color_t text_color = fpsDisplay->GetCompatibleColor(14);
    wouttext_outline(fpsDisplay, 1, 1, FONT_SPEECH, text_color, tbuffer);
    sprintf(tbuffer, "Loop %u", loopcounter);
    int textw = wgettextwidth(tbuffer, FONT_SPEECH);
    wouttext_outline(fpsDisplay, ui_view.GetWidth() / 2, 1, FONT_SPEECH, text_color, tbuffer);

    if (ddb)
        gfxDriver->UpdateDDBFromBitmap(ddb, fpsDisplay, false);
    else
        ddb = gfxDriver->CreateDDBFromBitmap(fpsDisplay, false);
    int yp = ui_view.GetHeight() - fpsDisplay->GetHeight();
    gfxDriver->DrawSprite(1, yp, ddb);
    invalidate_sprite(1, yp, ddb, false);
}

// Draw GUI and overlays of all kinds, anything outside the room space
void draw_gui_and_overlays() {
    int gg;

    if(pl_any_want_hook(AGSE_PREGUIDRAW))
        add_thing_to_draw(NULL, AGSE_PREGUIDRAW, 0, TRANS_RUN_PLUGIN, false);

    // draw overlays, except text boxes and portraits
    for (gg=0;gg<numscreenover;gg++) {
        // complete overlay draw in non-transparent mode
        if (screenover[gg].type == OVER_COMPLETE)
            add_thing_to_draw(screenover[gg].bmp, screenover[gg].x, screenover[gg].y, TRANS_OPAQUE, false);
        else if (screenover[gg].type != OVER_TEXTMSG && screenover[gg].type != OVER_PICTURE) {
            int tdxp, tdyp;
            get_overlay_position(gg, &tdxp, &tdyp);
            add_thing_to_draw(screenover[gg].bmp, tdxp, tdyp, 0, screenover[gg].hasAlphaChannel);
        }
    }

    // Draw GUIs - they should always be on top of overlays like
    // speech background text
    our_eip=35;
    if (((debug_flags & DBG_NOIFACE)==0) && (displayed_room >= 0)) {
        int aa;

        if (playerchar->activeinv >= MAX_INV) {
            quit("!The player.activeinv variable has been corrupted, probably as a result\n"
                "of an incorrect assignment in the game script.");
        }
        if (playerchar->activeinv < 1) gui_inv_pic=-1;
        else gui_inv_pic=game.invinfo[playerchar->activeinv].pic;
        /*    for (aa=0;aa<game.numgui;aa++) {
        if (guis[aa].on<1) continue;
        guis[aa].draw();
        guis[aa].poll();
        }*/
        our_eip = 37;
        if (guis_need_update) {
            //Bitmap *abufwas = ds;
            guis_need_update = 0;
            for (aa=0;aa<game.numgui;aa++) {
                if (!guis[aa].IsDisplayed()) continue;

                if (guibg[aa] == NULL)
                    recreate_guibg_image(&guis[aa]);

                eip_guinum = aa;
                our_eip = 370;
                guibg[aa]->ClearTransparent();
                //ds = guibg[aa];
                our_eip = 372;
                guis[aa].DrawAt(guibg[aa], 0,0);
                our_eip = 373;

                bool isAlpha = false;
                if (guis[aa].HasAlphaChannel()) 
                {
                    isAlpha = true;

                    if ((game.options[OPT_NEWGUIALPHA] == kGuiAlphaRender_Classic) && (guis[aa].BgImage > 0))
                    {
                        // old-style (pre-3.0.2) GUI alpha rendering
                        repair_alpha_channel(guibg[aa], spriteset[guis[aa].BgImage]);
                    }
                }

                if (guibgbmp[aa] != NULL) 
                {
                    gfxDriver->UpdateDDBFromBitmap(guibgbmp[aa], guibg[aa], isAlpha);
                }
                else
                {
                    guibgbmp[aa] = gfxDriver->CreateDDBFromBitmap(guibg[aa], isAlpha);
                }
                our_eip = 374;
            }
            //ds = abufwas;
        }
        our_eip = 38;
        // Draw the GUIs
        for (gg = 0; gg < game.numgui; gg++) {
            aa = play.gui_draw_order[gg];
            if (!guis[aa].IsDisplayed()) continue;

            // Don't draw GUI if "GUIs Turn Off When Disabled"
            if ((game.options[OPT_DISABLEOFF] == 3) &&
                (all_buttons_disabled > 0) &&
                (guis[aa].PopupStyle != kGUIPopupNoAutoRemove))
                continue;

            add_thing_to_draw(guibgbmp[aa], guis[aa].X, guis[aa].Y, guis[aa].Transparency, guis[aa].HasAlphaChannel());

            // only poll if the interface is enabled (mouseovers should not
            // work while in Wait state)
            if (IsInterfaceEnabled())
                guis[aa].Poll();
        }
    }

    // draw speech and portraits (so that they appear over GUIs)
    for (gg=0;gg<numscreenover;gg++) 
    {
        if (screenover[gg].type == OVER_TEXTMSG || screenover[gg].type == OVER_PICTURE)
        {
            int tdxp, tdyp;
            get_overlay_position(gg, &tdxp, &tdyp);
            add_thing_to_draw(screenover[gg].bmp, tdxp, tdyp, 0, false);
        }
    }

    our_eip = 1099;
}

// Push the gathered list of sprites into the active graphic renderer
void put_sprite_list_on_screen(bool in_room)
{
    // *** Draw the Things To Draw List ***

    SpriteListEntry *thisThing;

    for (size_t i = 0; i < thingsToDrawList.size(); ++i)
    {
        thisThing = &thingsToDrawList[i];

        if (thisThing->bmp != NULL) {
            // mark the image's region as dirty
            invalidate_sprite(thisThing->x, thisThing->y, thisThing->bmp, in_room);
        }
        else if ((thisThing->transparent != TRANS_RUN_PLUGIN) &&
            (thisThing->bmp == NULL)) 
        {
            quit("Null pointer added to draw list");
        }

        if (thisThing->bmp != NULL)
        {
            if (thisThing->transparent <= 255)
            {
                thisThing->bmp->SetTransparency(thisThing->transparent);
            }

            gfxDriver->DrawSprite(thisThing->x, thisThing->y, thisThing->bmp);
        }
        else if (thisThing->transparent == TRANS_RUN_PLUGIN) 
        {
            // meta entry to run the plugin hook
            gfxDriver->DrawSprite(thisThing->x, thisThing->y, NULL);
        }
        else
            quit("Unknown entry in draw list");
    }

    clear_draw_list();

    our_eip = 1100;
}

void draw_misc_info()
{
    if (display_fps)
        draw_fps();

    our_eip = 1101;
}

bool GfxDriverNullSpriteCallback(int x, int y)
{
    if (displayed_room < 0)
    {
        // if no room loaded, various stuff won't be initialized yet
        return 1;
    }
    return (pl_run_plugin_hooks(x, y) != 0);
}

void GfxDriverOnInitCallback(void *data)
{
    pl_run_plugin_init_gfx_hooks(gfxDriver->GetDriverID(), data);
}




// update_screen: copies the contents of the virtual screen to the actual
// screen, and draws the mouse cursor on.
void update_screen() {
    // cos hi-color doesn't fade in, don't draw it the first time
    if ((in_new_room > 0) & (game.color_depth > 1))
        return;

    if(pl_any_want_hook(AGSE_POSTSCREENDRAW))
        gfxDriver->DrawSprite(AGSE_POSTSCREENDRAW, 0, NULL);

    // update animating mouse cursor
    if (game.mcurs[cur_cursor].view>=0) {
        ags_domouse(DOMOUSE_NOCURSOR);
        // only on mousemove, and it's not moving
        if (((game.mcurs[cur_cursor].flags & MCF_ANIMMOVE)!=0) &&
            (mousex==lastmx) && (mousey==lastmy)) ;
        // only on hotspot, and it's not on one
        else if (((game.mcurs[cur_cursor].flags & MCF_HOTSPOT)!=0) &&
            (GetLocationType(game_to_data_coord(mousex), game_to_data_coord(mousey)) == 0))
            set_new_cursor_graphic(game.mcurs[cur_cursor].pic);
        else if (mouse_delay>0) mouse_delay--;
        else {
            int viewnum=game.mcurs[cur_cursor].view;
            int loopnum=0;
            if (loopnum >= views[viewnum].numLoops)
                quitprintf("An animating mouse cursor is using view %d which has no loops", viewnum + 1);
            if (views[viewnum].loops[loopnum].numFrames < 1)
                quitprintf("An animating mouse cursor is using view %d which has no frames in loop %d", viewnum + 1, loopnum);

            mouse_frame++;
            if (mouse_frame >= views[viewnum].loops[loopnum].numFrames)
                mouse_frame=0;
            set_new_cursor_graphic(views[viewnum].loops[loopnum].frames[mouse_frame].pic);
            mouse_delay = views[viewnum].loops[loopnum].frames[mouse_frame].speed + 5;
            CheckViewFrame (viewnum, loopnum, mouse_frame);
        }
        lastmx=mousex; lastmy=mousey;
    }

    // draw the debug console, if appropriate
    if ((play.debug_mode > 0) && (display_console != 0)) 
    {
        //int otextc = ds->GetTextColor();
        int ypp = 1;
        int txtspacing= getfontspacing_outlined(0);
        int barheight = getheightoflines(0, DEBUG_CONSOLE_NUMLINES - 1) + 4;

        const Rect &viewport = play.GetMainViewport();
        if (debugConsoleBuffer == NULL)
        {
            debugConsoleBuffer = BitmapHelper::CreateBitmap(viewport.GetWidth(), barheight,game.GetColorDepth());
            debugConsoleBuffer = ReplaceBitmapWithSupportedFormat(debugConsoleBuffer);
        }

        color_t draw_color = debugConsoleBuffer->GetCompatibleColor(15);
        debugConsoleBuffer->FillRect(Rect (0, 0, viewport.GetWidth() - 1, barheight), draw_color);
        color_t text_color = debugConsoleBuffer->GetCompatibleColor(16);
        for (int jj = first_debug_line; jj != last_debug_line; jj = (jj + 1) % DEBUG_CONSOLE_NUMLINES) {
            wouttextxy(debugConsoleBuffer, 1, ypp, 0, text_color, debug_line[jj]);
            ypp += txtspacing;
        }

        if (debugConsole == NULL)
            debugConsole = gfxDriver->CreateDDBFromBitmap(debugConsoleBuffer, false, true);
        else
            gfxDriver->UpdateDDBFromBitmap(debugConsole, debugConsoleBuffer, false);

        gfxDriver->DrawSprite(0, 0, debugConsole);
        invalidate_sprite(0, 0, debugConsole, false);
    }

    ags_domouse(DOMOUSE_NOCURSOR);

    if (!play.mouse_cursor_hidden && play.screen_is_faded_out == 0)
    {
        gfxDriver->DrawSprite(mousex - hotx, mousey - hoty, mouseCursor);
        invalidate_sprite(mousex - hotx, mousey - hoty, mouseCursor, false);
    }

    /*
    ags_domouse(DOMOUSE_ENABLE);
    // if the cursor is hidden, remove it again. However, it needs
    // to go on-off in order to update the stored mouse coordinates
    if (play.mouse_cursor_hidden)
    ags_domouse(DOMOUSE_DISABLE);*/

    write_screen();

    if (!play.screen_is_faded_out) {
        // always update the palette, regardless of whether the plugin
        // vetos the screen update
        if (bg_just_changed) {
            setpal ();
            bg_just_changed = 0;
        }
    }

    //if (!play.mouse_cursor_hidden)
    //    ags_domouse(DOMOUSE_DISABLE);

    screen_is_dirty = false;
}

void construct_virtual_screen(bool fullRedraw) 
{
    gfxDriver->ClearDrawLists();

    if (play.fast_forward)
        return;

    our_eip=3;

    gfxDriver->UseSmoothScaling(IS_ANTIALIAS_SPRITES);
    gfxDriver->RenderSpritesAtScreenResolution(usetup.RenderAtScreenRes, usetup.Supersampling);

    pl_run_plugin_hooks(AGSE_PRERENDER, 0);

    //
    // Batch 1: room viewports
    //
    const Rect &main_viewport = play.GetMainViewport();
    const Rect &room_viewport = play.GetRoomViewportAbs();
    const Rect &camera = play.GetRoomCamera();
    // TODO: have camera Left/Top used as a prescale (source) offset!
    SpriteTransform room_trans(0, 0,
        (float)room_viewport.GetWidth() / (float)camera.GetWidth(),
        (float)room_viewport.GetHeight() / (float)camera.GetHeight(),
        0.f);
    gfxDriver->BeginSpriteBatch(room_viewport, room_trans, RoomCameraFrame);
    Bitmap *ds = gfxDriver->GetMemoryBackBuffer();
    if (displayed_room >= 0 && play.screen_is_faded_out == 0)
    {
        if (fullRedraw)
            invalidate_screen();

        // For the sake of software renderer, if there is any kind of camera transform required
        // except screen offset, we tell it to draw on separate bitmap first with zero transformation.
        // There are few reasons for this, primary is that Allegro does not support StretchBlt
        // between different colour depths (i.e. it won't correctly stretch blit 16-bit rooms to
        // 32-bit virtual screen).
        // Also see comment to ALSoftwareGraphicsDriver::RenderToBackBuffer().
        bool translate_only = (room_trans.ScaleX == 1.f && room_trans.ScaleY == 1.f);
        Bitmap *room_bmp = translate_only ? ds : RoomCameraFrame.get();
        draw_room(ds, room_bmp, !translate_only);
        // reset the Baselines Changed flag now that we've drawn stuff
        walk_behind_baselines_changed = 0;
        put_sprite_list_on_screen(true);
    }
    else if (!gfxDriver->RequiresFullRedrawEachFrame()) 
    {
        // if the driver is not going to redraw the screen,
        // black it out so we don't get cursor trails
        // TODO: this is possible to do with dirty rects system now too (it can paint black rects outside of room viewport)
        ds->Fill(0);
    }

    // make sure that the mp3 is always playing smoothly
    update_mp3();
    our_eip=4;

    //
    // Batch 2: UI overlay
    //
    const Rect &ui_viewport = play.GetUIViewportAbs();
    gfxDriver->BeginSpriteBatch(ui_viewport, SpriteTransform());
    if (play.screen_is_faded_out == 0)
    {
        draw_gui_and_overlays();
        put_sprite_list_on_screen(false);
    }

    //
    // Batch 3: auxiliary info
    //
    gfxDriver->BeginSpriteBatch(main_viewport, SpriteTransform());
    draw_misc_info();

    if (fullRedraw)
    {
        // ensure the virtual screen is reconstructed
        // in case we want to take any screenshots before
        // the next game loop
        if (gfxDriver->UsesMemoryBackBuffer())
            gfxDriver->RenderToBackBuffer();
    }
}

// Draw everything 
void render_graphics(IDriverDependantBitmap *extraBitmap, int extraX, int extraY) {

    construct_virtual_screen(false);
    our_eip=5;

    if (extraBitmap != NULL) {
        invalidate_sprite(extraX, extraY, extraBitmap, false);
        gfxDriver->DrawSprite(extraX, extraY, extraBitmap);
    }

    update_screen();
}
