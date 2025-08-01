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

//
// Graphics initialization
//

#include <algorithm>
#include <inttypes.h>
#include <SDL.h>
#include "core/platform.h"
#include "ac/draw.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "gfx/ali3dexception.h"
#include "gfx/bitmap.h"
#include "gfx/gfxdriverfactory.h"
#include "gfx/gfxfilter.h"
#include "gfx/graphicsdriver.h"
#include "main/config.h"
#include "main/engine_setup.h"
#include "main/graphics_mode.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"

// Don't try to figure out the window size on the these ports because the port resizes itself.
#if AGS_PLATFORM_MOBILE
#define USE_SIMPLE_GFX_INIT
#endif

using namespace AGS::Common;
using namespace AGS::Engine;

extern int proper_exit;
extern AGSPlatformDriver *platform;
extern IGraphicsDriver *gfxDriver;


IGfxDriverFactory *GfxFactory = nullptr;

// Last saved fullscreen and windowed configs; they are used when switching
// between between fullscreen and windowed modes at runtime.
// If particular mode is modified, e.g. by script command, related config should be overwritten.
ActiveDisplaySetting SavedFullscreenSetting;
ActiveDisplaySetting SavedWindowedSetting;
// Current frame scaling setup
FrameScaleDef        CurFrameSetup;
// The game-to-screen transformation
PlaneScaling         GameScaling;



Size get_desktop_size(int display_index)
{
    Size sz;
    sys_get_desktop_resolution(display_index, sz.Width, sz.Height);
    return sz;
}

Size get_desktop_size()
{
    return get_desktop_size(sys_get_window_display_index());
}

Size get_max_display_size(int display_index, bool windowed)
{
    Size device_size = get_desktop_size(display_index);
    if (windowed)
        device_size = platform->ValidateWindowSize(display_index, device_size, false);
    return device_size;
}

bool create_gfx_driver(const String &gfx_driver_id)
{
    GfxFactory = GetGfxDriverFactory(gfx_driver_id);
    if (!GfxFactory)
    {
        Debug::Printf(kDbgMsg_Error, "Failed to initialize %s graphics factory. Error: %s", gfx_driver_id.GetCStr(), SDL_GetError());
        return false;
    }
    Debug::Printf("Using graphics factory: %s", gfx_driver_id.GetCStr());
    gfxDriver = GfxFactory->GetDriver();
    if (!gfxDriver)
    {
        Debug::Printf(kDbgMsg_Error, "Failed to create graphics driver. Error: %s", SDL_GetError());
        return false;
    }
    Debug::Printf("Created graphics driver: %s", gfxDriver->GetDriverName());
    return true;
}

// Set requested graphics filter, or default filter if the requested one failed
bool graphics_mode_set_filter_any(const GfxFilterSetup &setup)
{
    Debug::Printf("Requested gfx filter: %s", setup.ID.GetCStr());
    if (!graphics_mode_set_filter(setup.ID))
    {
        String def_filter = GfxFactory->GetDefaultFilterID();
        if (def_filter.CompareNoCase(setup.ID) == 0)
            return false;
        Debug::Printf(kDbgMsg_Error, "Failed to apply gfx filter: %s; will try to use factory default filter '%s' instead",
                setup.ID.GetCStr(), def_filter.GetCStr());
        if (!graphics_mode_set_filter(def_filter))
            return false;
    }
    Debug::Printf("Using gfx filter: %s", GfxFactory->GetDriver()->GetGraphicsFilter()->GetInfo().Id.GetCStr());
    return true;
}

bool find_nearest_supported_mode(const IGfxModeList &modes, const Size &wanted_size, const int color_depth,
                                 const Size *ratio_reference, const Size *upper_bound, DisplayMode &dm, int *mode_index)
{
    uint32_t wanted_ratio = 0;
    if (ratio_reference && !ratio_reference->IsNull())
    {
        wanted_ratio = (ratio_reference->Height << kShift) / ratio_reference->Width;
    }
    
    int nearest_width = 0;
    int nearest_height = 0;
    int nearest_width_diff = 0;
    int nearest_height_diff = 0;
    DisplayMode nearest_mode;
    int nearest_mode_index = -1;
    int mode_count = modes.GetModeCount();
    for (int i = 0; i < mode_count; ++i)
    {
        DisplayMode mode;
        if (!modes.GetMode(i, mode))
        {
            continue;
        }
        if (wanted_ratio > 0)
        {
            uint32_t mode_ratio = (mode.Height << kShift) / mode.Width;
            if (mode_ratio != wanted_ratio)
            {
                continue;
            }
        }
        if (upper_bound && (mode.Width > upper_bound->Width || mode.Height > upper_bound->Height))
            continue;
        if (mode.Width == wanted_size.Width && mode.Height == wanted_size.Height)
        {
            nearest_width = mode.Width;
            nearest_height = mode.Height;
            nearest_mode_index = i;
            nearest_mode = mode;
            break;
        }
      
        int diff_w = abs(wanted_size.Width - mode.Width);
        int diff_h = abs(wanted_size.Height - mode.Height);
        bool same_diff_w_higher = (diff_w == nearest_width_diff && nearest_width < wanted_size.Width);
        bool same_diff_h_higher = (diff_h == nearest_height_diff && nearest_height < wanted_size.Height);

        if (nearest_width == 0 ||
            ((diff_w < nearest_width_diff || same_diff_w_higher) && diff_h <= nearest_height_diff) ||
            ((diff_h < nearest_height_diff || same_diff_h_higher) && diff_w <= nearest_width_diff))
        {
            nearest_width = mode.Width;
            nearest_width_diff = diff_w;
            nearest_height = mode.Height;
            nearest_height_diff = diff_h;
            nearest_mode = mode;
            nearest_mode_index = i;
        }
    }

    if (nearest_width > 0 && nearest_height > 0)
    {
        dm = nearest_mode;
        if (mode_index)
            *mode_index = nearest_mode_index;
        return true;
    }
    return false;
}

Size get_game_frame_from_screen_size(const Size &game_size, const Size screen_size,
                            const FrameScaleDef frame, int scale = 0)
{
    // Set game frame as native game resolution scaled by particular method
    switch (frame)
    {
    case kFrame_Stretch: return screen_size;
    case kFrame_Proportional: return ProportionalStretch(screen_size, game_size);
    case kFrame_Round:
        {
            int fp_scale;
            if (scale > 0)
                fp_scale = convert_scaling_to_fp(scale);
            else
                fp_scale = std::max<int32_t>(kUnit,
                    std::min((screen_size.Width / game_size.Width) << kShift,
                              (screen_size.Height / game_size.Height) << kShift));
            Size frame_size = Size(
                (game_size.Width * fp_scale) >> kShift,
                (game_size.Height * fp_scale) >> kShift);
            // If the scaled game size appear larger than the screen,
            // use "proportional stretch" method instead
            if (frame_size.ExceedsByAny(screen_size))
                frame_size = ProportionalStretch(screen_size, game_size);
            return frame_size;
        }
    default: return Size();
    }
}

static Size precalc_screen_size(const Size &game_size, int display_index, const WindowSetup &ws, const FrameScaleDef frame)
{
    const bool windowed = ws.Mode == kWnd_Windowed;
    if (ws.SizeHint == kWndSizeHint_GameNative)
    {
        return game_size;
    }
    else if (!ws.Size.IsNull())
    {
        // Use explicit resolution from user config
        return ws.Size;
    }
    else if (ws.Scale > 0)
    {
        return get_game_frame_from_screen_size(game_size, get_max_display_size(display_index, windowed), frame, ws.Scale);
    }
    // If nothing is set, then for the fullscreen mode set as big as current device/desktop size;
    // for the windowed mode assume maximal size inside desktop using given frame scaling
    if (windowed)
        return get_game_frame_from_screen_size(game_size, get_max_display_size(display_index, windowed), frame);
    return get_max_display_size(display_index, false);
}

static int validate_display_index(int display_index)
{
    if (display_index >= sys_get_display_count())
        return 0;
    return display_index;
}

// Find closest possible compatible display mode and initialize it
static bool try_init_compatible_mode(const DisplayMode &dm)
{
    const Size &screen_size = Size(dm.Width, dm.Height);
    // Find nearest compatible mode and init that
    Debug::Printf("Attempt to find nearest supported resolution for screen size %d x %d (%d-bit) %s, on display %d",
        dm.Width, dm.Height, dm.ColorDepth, dm.IsWindowed() ? "windowed" : "fullscreen", dm.DisplayIndex);
    const Size device_size = get_max_display_size(dm.DisplayIndex, dm.IsWindowed());
    if (dm.IsWindowed())
        Debug::Printf("Maximal allowed window size: %d x %d", device_size.Width, device_size.Height);
    DisplayMode dm_compat = dm;

    std::unique_ptr<IGfxModeList> modes(gfxDriver->GetSupportedModeList(dm.DisplayIndex, dm.ColorDepth));

    // Windowed mode
    if (dm.IsWindowed())
    {
        // If windowed mode, make the resolution stay in the generally supported limits
        dm_compat.Width = std::min(dm_compat.Width, device_size.Width);
        dm_compat.Height = std::min(dm_compat.Height, device_size.Height);
    }
    // Fullscreen mode
    else
    {
        // Try to find any compatible mode from the list of available ones
        bool mode_found = false;
        if (modes.get())
            mode_found = find_nearest_supported_mode(*modes.get(), screen_size, dm.ColorDepth, nullptr, nullptr, dm_compat);
        if (!mode_found)
            Debug::Printf("Could not find compatible fullscreen mode. Will try to force-set mode requested by user and fallback to windowed mode if that fails.");
        dm_compat.Vsync = dm.Vsync;
        dm_compat.Mode = dm.Mode;
    }

    bool result = graphics_mode_set_dm(dm_compat);
    if (!result && dm.IsWindowed())
    {
        // When initializing windowed mode we could start with any random window size;
        // if that did not work, try to find nearest supported mode, as with fullscreen mode,
        // except refering to max window size as an upper bound
        if (find_nearest_supported_mode(*modes.get(), screen_size, dm.ColorDepth, nullptr, &device_size, dm_compat))
        {
            dm_compat.Vsync = dm.Vsync;
            dm_compat.Mode = kWnd_Windowed;
            result = graphics_mode_set_dm(dm_compat);
        }
    }
    return result;
}

// Try to find and initialize compatible display mode as close to given setup as possible
static bool try_init_mode_using_setup(const GraphicResolution &game_res, const WindowSetup &ws,
                               const int col_depth, const FrameScaleDef frame,
                               const GfxFilterSetup &filter,
                               const DisplayParamsEx &params)
{
    // We determine the requested size of the screen using setup options
    const Size screen_size = precalc_screen_size(game_res, params.DisplayIndex, ws, frame);
    DisplayMode dm(GraphicResolution(screen_size.Width, screen_size.Height, col_depth),
        ws.Mode, params.DisplayIndex, params.RefreshRate, params.VSync);
    if (!try_init_compatible_mode(dm))
        return false;

    // Set up native size and render frame
    if (!graphics_mode_set_native_res(game_res) || !graphics_mode_set_render_frame(frame))
        return false;

    // Set up graphics filter
    if (!graphics_mode_set_filter_any(filter))
        return false;
    return true;
}

void log_out_driver_modes(const int display_index, const int color_depth)
{
    IGfxModeList *modes = gfxDriver->GetSupportedModeList(display_index, color_depth);
    if (!modes)
    {
        Debug::Printf(kDbgMsg_Error, "Couldn't get a list of supported resolutions for display %d, color depth = %d", display_index, color_depth);
        return;
    }
    const int mode_count = modes->GetModeCount();
    DisplayMode mode;
    String mode_str;
    for (int i = 0, in_str = 0; i < mode_count; ++i)
    {
        modes->GetMode(i, mode);
        mode_str.Append(String::FromFormat("%dx%d;", mode.Width, mode.Height));
        if (++in_str % 8 == 0)
            mode_str.Append("\n\t");
    }
    delete modes;

    String out_str = String::FromFormat("Supported gfx modes for display %d (%d-bit): ", display_index, color_depth);
    if (!mode_str.IsEmpty())
    {
        out_str.Append("\n\t");
        out_str.Append(mode_str);
    }
    else
        out_str.Append("none");
    Debug::Printf(out_str);
}

// Create requested graphics driver and try to find and initialize compatible display mode as close to user setup as possible;
// if the given setup fails, gets default setup for the opposite type of mode (fullscreen/windowed) and tries that instead.
static bool create_gfx_driver_and_init_mode_any(const String &gfx_driver_id,
    const GraphicResolution &game_res,
    const DisplayModeSetup &setup, const ColorDepthOption &color_depth)
{
    if (!graphics_mode_create_renderer(gfx_driver_id))
        return false;
    if (!sys_is_display_valid(setup.DisplayIndex))
        return false;

    const int use_col_depth =
        color_depth.Forced ? color_depth.Bits : gfxDriver->GetDisplayDepthForNativeDepth(color_depth.Bits);
    // Log out supported driver modes
    log_out_driver_modes(setup.DisplayIndex, use_col_depth);

    bool windowed = setup.Windowed;
    WindowSetup ws = windowed ? setup.WinSetup : setup.FsSetup;
    FrameScaleDef frame = windowed ? setup.WinGameFrame : setup.FsGameFrame;
    bool result = try_init_mode_using_setup(game_res, ws, use_col_depth, frame, setup.Filter, DisplayParamsEx(setup.DisplayIndex, setup.RefreshRate, setup.VSync));
    // Try windowed mode if fullscreen failed, and vice versa
    if (!result && editor_debugging_initialized == 0)
    {
        windowed = !windowed;
        ws = windowed ? setup.WinSetup : setup.FsSetup;
        frame = windowed ? setup.WinGameFrame : setup.FsGameFrame;
        result = try_init_mode_using_setup(game_res, ws, use_col_depth, frame, setup.Filter, DisplayParamsEx(setup.DisplayIndex, setup.RefreshRate, setup.VSync));
    }
    return result;
}

#ifdef USE_SIMPLE_GFX_INIT
static bool simple_create_gfx_driver_and_init_mode(const String &gfx_driver_id,
                                            const GraphicResolution &game_res,
                                            const DisplayModeSetup &setup,
                                            const ColorDepthOption &color_depth)
{
    if (!graphics_mode_create_renderer(gfx_driver_id))
        return false;
    if (!sys_is_display_valid(setup.DisplayIndex))
        return false;

    // FIXME: use precalc_screen_size() here for Desktop systems, don't hardcode to using game_res
    const int col_depth = gfxDriver->GetDisplayDepthForNativeDepth(color_depth.Bits);
    const WindowSetup ws = setup.Windowed ? setup.WinSetup : setup.FsSetup;
    const FrameScaleDef frame = setup.Windowed ? setup.WinGameFrame : setup.FsGameFrame;

    DisplayMode dm(GraphicResolution(game_res.Width, game_res.Height, col_depth),
        ws.Mode, setup.DisplayIndex, setup.RefreshRate, setup.VSync);

    if (!graphics_mode_set_dm(dm)) { return false; }
    if (!graphics_mode_set_native_res(dm)) { return false; }
    if (!graphics_mode_set_render_frame(frame)) { return false; }
    if (!graphics_mode_set_filter_any(setup.Filter)) { return false; }

    return true;
}
#endif // USE_SIMPLE_GFX_INIT


void display_gfx_mode_error(const Size &game_size, const WindowSetup &ws, const int color_depth,
                            const GfxFilterSetup &filter_setup)
{
    proper_exit=1;

    String main_error;
    PGfxFilter filter = gfxDriver ? gfxDriver->GetGraphicsFilter() : PGfxFilter();
    Size wanted_screen;
    if (!ws.Size.IsNull() )
        main_error.Format("There was a problem initializing graphics mode %d x %d (%d-bit), or finding nearest compatible mode, with game size %d x %d and filter '%s'.",
            ws.Size.Width, ws.Size.Height, color_depth, game_size.Width, game_size.Height, filter ? filter->GetInfo().Id.GetCStr() : "Undefined");
    else
        main_error.Format("There was a problem finding and/or creating valid graphics mode for game size %d x %d (%d-bit) and requested filter '%s'.",
            game_size.Width, game_size.Height, color_depth, filter_setup.ID.IsEmpty() ? "Undefined" : filter_setup.ID.GetCStr());

    platform->DisplayAlert("%s\n"
            "(Problem: '%s')\n"
            "Try to correct the problem, or seek help from the AGS homepage."
            "%s",
            main_error.GetCStr(), SDL_GetError(), platform->GetGraphicsTroubleshootingText());
}

bool graphics_mode_init_any(const GraphicResolution &game_res, const DisplayModeSetup &setup, const ColorDepthOption &color_depth,
                            Size *init_desktop_size)
{
    const int use_display_index = validate_display_index(setup.DisplayIndex);
    if (use_display_index != setup.DisplayIndex)
        Debug::Printf(kDbgMsg_Warn, "Requested display index %d is invalid, fall back to default", setup.DisplayIndex);

    // Log out display information
    Size device_size;
    if (sys_get_desktop_resolution(use_display_index, device_size.Width, device_size.Height))
        Debug::Printf("Device display resolution: %d x %d", device_size.Width, device_size.Height);
    else
        Debug::Printf(kDbgMsg_Error, "Unable to obtain device resolution");
    if (init_desktop_size)
        *init_desktop_size = device_size;

    WindowSetup ws = setup.Windowed ? setup.WinSetup : setup.FsSetup;
    FrameScaleDef gameframe = setup.Windowed ? setup.WinGameFrame : setup.FsGameFrame;
    const String scale_option = make_scaling_option(gameframe);
    Debug::Printf(kDbgMsg_Info, "Graphic settings: driver: %s, windowed: %s, screen size: %d x %d, game scale: %s",
        setup.DriverID.GetCStr(),
        setup.Windowed ? "yes" : "no",
        ws.Size.Width, ws.Size.Height,
        scale_option.GetCStr());
    Debug::Printf(kDbgMsg_Info, "Graphic settings: refresh rate (optional): %d, vsync: %d",
        setup.RefreshRate, setup.VSync);

    // Prepare the list of available gfx factories, having the one requested by user at first place
    // TODO: make factory & driver IDs case-insensitive!
    StringV ids;
    GetGfxDriverFactoryNames(ids);
    StringV::iterator it = ids.begin();
    for (; it != ids.end(); ++it)
    {
        if (it->CompareNoCase(setup.DriverID) == 0) break;
    }
    if (it != ids.end())
        std::rotate(ids.begin(), it, ids.end());
    else
        Debug::Printf(kDbgMsg_Error, "Requested graphics driver '%s' not found, will try existing drivers instead", setup.DriverID.GetCStr());

    // Fixup display setup if necessary
    DisplayModeSetup use_setup = setup;
    use_setup.DisplayIndex = use_display_index;

    // Try to create renderer and init gfx mode, choosing one factory at a time
    bool result = false;
    for (const auto &id : ids)
    {
        result =
#ifdef USE_SIMPLE_GFX_INIT
            simple_create_gfx_driver_and_init_mode(id, game_res, use_setup, color_depth);
#else
            create_gfx_driver_and_init_mode_any(id, game_res, use_setup, color_depth);
#endif

        if (result)
            break;
        graphics_mode_shutdown();
    }
    // If all possibilities failed, display error message and quit
    if (!result)
    {
        display_gfx_mode_error(game_res, ws, color_depth.Bits, use_setup.Filter);
        return false;
    }
    return true;
}

ActiveDisplaySetting graphics_mode_get_last_setting(bool windowed)
{
    return windowed ? SavedWindowedSetting : SavedFullscreenSetting;
}

bool graphics_mode_create_renderer(const String &driver_id)
{
    if (!create_gfx_driver(driver_id))
        return false;

    gfxDriver->SetCallbackOnInit(GfxDriverOnInitCallback);
    return true;
}

bool graphics_mode_set_dm_any(const Size &game_size, const WindowSetup &ws,
                              const ColorDepthOption &color_depth,
                              const FrameScaleDef frame, const DisplayParamsEx &params)
{
    const int use_display_index = validate_display_index(params.DisplayIndex);

    // We determine the requested size of the screen using setup options
    const Size screen_size = precalc_screen_size(game_size, use_display_index, ws, frame);
    DisplayMode dm(GraphicResolution(screen_size.Width, screen_size.Height, color_depth.Bits),
        ws.Mode, use_display_index, params.RefreshRate, params.VSync);
    return try_init_compatible_mode(dm);
}

bool graphics_mode_set_dm(const DisplayMode &dm)
{
    Debug::Printf("Attempt to switch gfx mode to %d x %d (%d-bit) %s, on display %d",
        dm.Width, dm.Height, dm.ColorDepth, dm.IsWindowed() ? "windowed" : "fullscreen", dm.DisplayIndex);

    if (!sys_is_display_valid(dm.DisplayIndex))
    {
        Debug::Printf(kDbgMsg_Error, "Requested display index %d is not valid.", dm.DisplayIndex);
        return false;
    }

    // Tell Allegro new default bitmap color depth (must be done before set_gfx_mode)
    // TODO: this is also done inside ALSoftwareGraphicsDriver implementation; can remove one?
    set_color_depth(dm.ColorDepth);

    if (!gfxDriver->SetDisplayMode(dm))
    {
        Debug::Printf(kDbgMsg_Error, "Failed to init gfx mode. Error: %s", SDL_GetError());
        return false;
    }

    DisplayMode rdm = gfxDriver->GetDisplayMode();
    ActiveDisplaySetting &setting = rdm.IsWindowed() ? SavedWindowedSetting : SavedFullscreenSetting;
    setting.Dm = rdm;
    Debug::Printf(kDbgMsg_Info, "Graphics driver set: %s", gfxDriver->GetDriverName());
    Debug::Printf(kDbgMsg_Info, "Graphics mode set: %d x %d (%d-bit) %s, on display %d",
        rdm.Width, rdm.Height, rdm.ColorDepth,
        rdm.IsWindowed() ? "windowed" : (rdm.IsRealFullscreen() ? "fullscreen" : "fullscreen desktop"),
        setting.Dm.DisplayIndex);
    Debug::Printf(kDbgMsg_Info, "Graphics mode set: refresh rate (optional): %d, vsync: %d", rdm.RefreshRate, rdm.Vsync);
    uint64_t tex_mem = gfxDriver->GetAvailableTextureMemory();
    if (tex_mem > 0u)
        Debug::Printf("Graphics driver texture memory (approx): %" PRIu64 " KB", tex_mem / 1024u);
    return true;
}

bool graphics_mode_update_render_frame()
{
    if (!gfxDriver || !gfxDriver->IsModeSet() || !gfxDriver->IsNativeSizeValid())
        return false;

    DisplayMode dm = gfxDriver->GetDisplayMode();
    Size screen_size = Size(dm.Width, dm.Height);
    Size native_size = gfxDriver->GetNativeSize();
    Size frame_size = get_game_frame_from_screen_size(native_size, screen_size, CurFrameSetup);
    Rect render_frame = CenterInRect(RectWH(screen_size), RectWH(frame_size));

    if (!gfxDriver->SetRenderFrame(render_frame))
    {
        Debug::Printf(kDbgMsg_Error, "Failed to set render frame (%d, %d, %d, %d : %d x %d). Error: %s", 
            render_frame.Left, render_frame.Top, render_frame.Right, render_frame.Bottom,
            render_frame.GetWidth(), render_frame.GetHeight(), SDL_GetError());
        return false;
    }

    Rect dst_rect = gfxDriver->GetRenderDestination();
    Debug::Printf("Render frame set, render dest (%d, %d, %d, %d : %d x %d)",
        dst_rect.Left, dst_rect.Top, dst_rect.Right, dst_rect.Bottom, dst_rect.GetWidth(), dst_rect.GetHeight());
    // init game scaling transformation
    GameScaling.Init(native_size, gfxDriver->GetRenderDestination());
    return true;
}

bool graphics_mode_set_native_res(const GraphicResolution &native_res)
{
    if (!gfxDriver || !native_res.IsValid())
        return false;
    if (!gfxDriver->SetNativeResolution(native_res))
        return false;
    // if render frame translation was already set, then update it with new native size
    if (gfxDriver->IsRenderFrameValid())
        graphics_mode_update_render_frame();
    return true;
}

FrameScaleDef graphics_mode_get_render_frame()
{
    return CurFrameSetup;
}

bool graphics_mode_set_render_frame(const FrameScaleDef frame)
{
    if (frame < 0 || frame >= kNumFrameScaleDef)
        return false;
    CurFrameSetup = frame;
    if (gfxDriver->GetDisplayMode().IsWindowed())
        SavedWindowedSetting.Frame = frame;
    else
        SavedFullscreenSetting.Frame = frame;
    graphics_mode_update_render_frame();
    return true;
}

bool graphics_mode_set_filter(const String &filter_id)
{
    if (!GfxFactory)
        return false;

    String filter_error;
    PGfxFilter filter = GfxFactory->SetFilter(filter_id, filter_error);
    if (!filter)
    {
        Debug::Printf(kDbgMsg_Error, "Unable to set graphics filter '%s'. Error: %s", filter_id.GetCStr(), filter_error.GetCStr());
        return false;
    }
    Rect filter_rect  = filter->GetDestination();
    Debug::Printf("Graphics filter set: '%s', filter dest (%d, %d, %d, %d : %d x %d)", filter->GetInfo().Id.GetCStr(),
        filter_rect.Left, filter_rect.Top, filter_rect.Right, filter_rect.Bottom, filter_rect.GetWidth(), filter_rect.GetHeight());
    return true;
}

void graphics_mode_on_window_changed(const Size &sz)
{
    if (!gfxDriver)
        return; // nothing to update
    release_drawobj_rendertargets();
    gfxDriver->UpdateDeviceScreen(sz);
    graphics_mode_update_render_frame();
}

void graphics_mode_shutdown()
{
    if (GfxFactory)
        GfxFactory->Shutdown();
    GfxFactory = nullptr;
    gfxDriver = nullptr;
}
