// AGS Editor ImGui - Application implementation
#include "app.h"
#include "ui/editor_ui.h"
#include "ui/panes/build_pane.h"
#include "pipeline/build_system.h"
#include "project/project.h"
#include "project/game_data.h"
#include "core/dpi_helper.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <SDL.h>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <memory>
#include <filesystem>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "ui/IconsLucide.h"
#include "core/app_icon.h"

namespace AGSEditor { bool g_icons_loaded = false; }

AGSEditorApp::AGSEditorApp()
    : window_title_("AGS Editor (ImGui)")
{
}

AGSEditorApp::~AGSEditorApp() = default;

bool AGSEditorApp::Init(int argc, char* argv[])
{
    // Parse command-line arguments
    std::string auto_open_game;
    int auto_open_room = -1;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--open" && i + 1 < argc)
            auto_open_game = argv[++i];
        else if (arg == "--room" && i + 1 < argc)
            auto_open_room = std::atoi(argv[++i]);
        else if (arg == "--compile")
            cli_action_ = CLIAction::Compile;
        else if (arg == "--package")
            cli_action_ = CLIAction::Package;
        else if (arg == "--run")
            cli_action_ = CLIAction::Run;
    }

    // If a CLI action is requested, we run headless (no GUI)
    if (cli_action_ != CLIAction::None)
    {
        headless_mode_ = true;
        cli_project_path_ = auto_open_game;

        if (cli_project_path_.empty())
        {
            fprintf(stderr, "[Error] --compile/--package/--run requires --open <project_path>\n");
            fprintf(stderr, "Usage: ags_editor_imgui --open Game.agf --compile\n");
            fprintf(stderr, "       ags_editor_imgui --open Game.agf --package\n");
            fprintf(stderr, "       ags_editor_imgui --open Game.agf --run\n");
            exit_code_ = 1;
            return true; // return true so RunHeadless() can report the error
        }

        // Minimal init: just the project, no SDL/ImGui
        project_ = std::make_unique<AGSEditor::Project>();
        return true;
    }

    // Load editor preferences before anything else
    preferences_.Load();

    // Apply DPI override from preferences
    if (preferences_.dpi_override > 0.1f)
        dpi_scale_ = preferences_.dpi_override;

    if (!InitSDL())
        return false;

    // Compute the "content scale" that our Dpi() helpers should use.
    // On compositor-scaled displays (Wayland), the compositor already handles
    // the upscaling, so ImGui works in logical coordinates and we set
    // g_dpi_scale = 1.0.  On non-compositor HiDPI (X11), we must scale
    // widget sizes ourselves, so g_dpi_scale = dpi_scale_.
    //
    // fb_scale_ records whether the compositor is doing scaling for us.
    float ui_scale = dpi_scale_ / fb_scale_;
    AGSEditor::g_dpi_scale = ui_scale;
    fprintf(stderr, "[Info] DPI scale: %.2f, fb_scale: %.2f, UI scale (g_dpi_scale): %.2f\n",
            dpi_scale_, fb_scale_, ui_scale);

    if (!InitImGui())
        return false;

    project_ = std::make_unique<AGSEditor::Project>();

    // Initialize texture cache with renderer
    texture_cache_.SetRenderer(renderer_);

    // Discover game templates
    if (!editor_dir_.empty())
        template_manager_.DiscoverTemplates(editor_dir_);
    else
        template_manager_.DiscoverTemplates(".");

    ui_ = std::make_unique<AGSEditor::EditorUI>(*this);

    RegisterShortcuts();

    // Auto-open game and room from command line
    if (!auto_open_game.empty())
    {
        fprintf(stderr, "[App] Auto-opening game: %s\n", auto_open_game.c_str());
        if (project_->OpenProject(auto_open_game))
        {
            fprintf(stderr, "[App] Game opened successfully.\n");
            ui_->NotifyProjectLoaded();
            if (auto_open_room >= 0)
            {
                fprintf(stderr, "[App] Auto-opening room %d...\n", auto_open_room);
                ui_->OpenRoomEditor(auto_open_room);
                fprintf(stderr, "[App] Room %d editor opened.\n", auto_open_room);
            }
        }
        else
        {
            fprintf(stderr, "[App] Failed to open game.\n");
        }
    }

    return true;
}

bool AGSEditorApp::InitSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        fprintf(stderr, "Error: SDL_Init(): %s\n", SDL_GetError());
        return false;
    }

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // --- Create window first, then detect DPI robustly ---
    // We need the window + renderer to exist before we can measure the real
    // content scale (renderer output size vs. window size).  Pre-window DPI
    // helpers like ImGui_ImplSDL2_GetContentScaleForDisplay() return 1.0 on
    // Wayland and are unreliable on X11, so we defer DPI detection.

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MAXIMIZED);

    window_ = SDL_CreateWindow(
        window_title_.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1600,
        900,
        window_flags);

    if (!window_)
    {
        fprintf(stderr, "Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return false;
    }

    // Set window icon
    SDL_Surface* icon = CreateAGSEditorIcon();
    if (icon)
    {
        SDL_SetWindowIcon(window_, icon);
        SDL_FreeSurface(icon);
    }

    renderer_ = SDL_CreateRenderer(window_, -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    if (!renderer_)
    {
        fprintf(stderr, "Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return false;
    }

    // --- Measure framebuffer ratio (compositor scaling) ---
    {
        int win_w = 0, win_h = 0, out_w = 0, out_h = 0;
        SDL_GetWindowSize(window_, &win_w, &win_h);
        SDL_GetRendererOutputSize(renderer_, &out_w, &out_h);
        if (win_w > 0 && out_w > 0)
            fb_scale_ = (float)out_w / (float)win_w;
        else
            fb_scale_ = 1.0f;
        fprintf(stderr, "[DPI] Framebuffer ratio (output/window): %.2f (%dx%d / %dx%d)\n",
                fb_scale_, out_w, out_h, win_w, win_h);
    }

    // --- Detect overall desired content scale ---
    if (dpi_scale_ < 0.5f)
    {
        dpi_scale_ = DetectDpiScale();
    }

    // --- Resize window to DPI-appropriate dimensions ---
    // The window was created at a small default size so we could measure
    // fb_scale_.  Now that we know the real DPI, resize to fill the screen
    // properly.  SDL_WINDOW_MAXIMIZED should handle this, but on some X11
    // WMs the initial size matters, so we set it explicitly.
    if (dpi_scale_ > 1.01f && fb_scale_ < 1.01f)
    {
        // Non-compositor HiDPI (X11): the WM works in raw pixels, so we
        // need a larger initial window size.
        int new_w = (int)(1600 * dpi_scale_);
        int new_h = (int)(900 * dpi_scale_);
        SDL_SetWindowSize(window_, new_w, new_h);
        SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        fprintf(stderr, "[DPI] Resized window to %dx%d for non-compositor HiDPI\n", new_w, new_h);
    }

    return true;
}

float AGSEditorApp::DetectDpiScale()
{
    // The "content scale" is the overall factor by which everything should
    // appear larger.  We probe multiple sources and pick the best one.
    // fb_scale_ is already measured at this point.
    //
    // Diagnosis is printed for EVERY method so we can debug HiDPI issues.

    int display = SDL_GetWindowDisplayIndex(window_);
    if (display < 0) display = 0;

    // Method 1: Framebuffer ratio (Wayland with SDL_WINDOW_ALLOW_HIGHDPI).
    fprintf(stderr, "[DPI] Method 1 - framebuffer ratio: %.2f\n", fb_scale_);
    if (fb_scale_ > 1.01f)
    {
        fprintf(stderr, "[DPI] → Using framebuffer ratio: %.2f\n", fb_scale_);
        return fb_scale_;
    }

    // Below this point fb_scale_ ≈ 1.0, so the compositor is NOT doing
    // HiDPI scaling for us.  We need to detect the desired scale ourselves.

    // Method 2: SDL_GetDisplayDPI
    {
        float ddpi = 0.0f, hdpi = 0.0f, vdpi = 0.0f;
        int rc = SDL_GetDisplayDPI(display, &ddpi, &hdpi, &vdpi);
        fprintf(stderr, "[DPI] Method 2 - SDL_GetDisplayDPI: rc=%d, ddpi=%.1f, hdpi=%.1f, vdpi=%.1f\n",
                rc, ddpi, hdpi, vdpi);
        if (rc == 0 && ddpi > 0.0f)
        {
            float scale = ddpi / 96.0f;
            if (scale >= 1.2f)
            {
                fprintf(stderr, "[DPI] → Using SDL DPI: %.2f\n", scale);
                return scale;
            }
        }
    }

    // Method 3: Xft.dpi from X11 resource database (most reliable on X11).
    // GNOME, KDE, XFCE all set this to reflect the desired DPI.
    // e.g. Xft.dpi: 192 means 2× scaling.
    {
        float xft_dpi = 0.0f;
        FILE* pipe = popen("xrdb -query 2>/dev/null", "r");
        if (pipe)
        {
            char line[256];
            while (fgets(line, sizeof(line), pipe))
            {
                if (strncmp(line, "Xft.dpi:", 8) == 0 || strncmp(line, "Xft.dpi\t", 8) == 0)
                {
                    // Parse the value after "Xft.dpi:" or "Xft.dpi\t..."
                    const char* p = line + 8;
                    while (*p == ' ' || *p == '\t') p++;
                    xft_dpi = (float)atof(p);
                    break;
                }
            }
            pclose(pipe);
        }
        fprintf(stderr, "[DPI] Method 3 - Xft.dpi: %.1f\n", xft_dpi);
        if (xft_dpi > 0.0f)
        {
            float scale = xft_dpi / 96.0f;
            if (scale >= 1.1f)
            {
                fprintf(stderr, "[DPI] → Using Xft.dpi: %.2f\n", scale);
                return scale;
            }
        }
    }

    // Method 4: Environment variables set by desktop environments.
    // Check both integer and fractional scale variables.
    {
        const char* env_vars[] = {
            "GDK_SCALE",            // GTK3 integer scaling
            "GDK_DPI_SCALE",        // GTK3 fractional DPI multiplier
            "QT_SCALE_FACTOR",      // Qt5+ global scale
            "QT_SCREEN_SCALE_FACTORS",  // Qt5+ per-screen
            "ELM_SCALE",            // EFL/Enlightenment
            nullptr
        };
        for (const char** ev = env_vars; *ev; ++ev)
        {
            const char* val = SDL_getenv(*ev);
            if (val && val[0])
            {
                float scale = (float)atof(val);
                fprintf(stderr, "[DPI] Method 4 - env $%s=%s (parsed: %.2f)\n", *ev, val, scale);
                if (scale >= 1.1f)
                {
                    fprintf(stderr, "[DPI] → Using env $%s: %.2f\n", *ev, scale);
                    return scale;
                }
            }
        }
        // Also check QT_FONT_DPI (KDE Plasma sets this)
        const char* qt_font_dpi = SDL_getenv("QT_FONT_DPI");
        if (qt_font_dpi && qt_font_dpi[0])
        {
            float dpi_val = (float)atof(qt_font_dpi);
            fprintf(stderr, "[DPI] Method 4 - env $QT_FONT_DPI=%s (parsed: %.1f)\n", qt_font_dpi, dpi_val);
            if (dpi_val > 0.0f)
            {
                float scale = dpi_val / 96.0f;
                if (scale >= 1.1f)
                {
                    fprintf(stderr, "[DPI] → Using QT_FONT_DPI: %.2f\n", scale);
                    return scale;
                }
            }
        }
    }

    // Method 5: Display resolution heuristic.
    // If the display mode resolution is very high but SDL couldn't detect DPI,
    // infer a reasonable scale from the display mode.
    {
        SDL_DisplayMode mode;
        if (SDL_GetCurrentDisplayMode(display, &mode) == 0)
        {
            fprintf(stderr, "[DPI] Method 5 - Display mode: %dx%d\n", mode.w, mode.h);
            // Common HiDPI thresholds (these are native pixel resolutions):
            // ≥3840 wide = 4K → likely 2×
            // ≥3200 wide = QHD+ → likely 2×
            // ≥2560 wide = QHD → likely 1.25-1.5×
            // This is a rough heuristic; it can't know the physical monitor size.
            if (mode.w >= 3840 || mode.h >= 2160)
            {
                fprintf(stderr, "[DPI] → 4K+ display detected, using 2.0\n");
                return 2.0f;
            }
            else if (mode.w >= 3200 || mode.h >= 1800)
            {
                fprintf(stderr, "[DPI] → QHD+ display detected, using 1.75\n");
                return 1.75f;
            }
            else if (mode.w >= 2560 || mode.h >= 1440)
            {
                fprintf(stderr, "[DPI] → QHD display detected, using 1.25\n");
                return 1.25f;
            }
        }
    }

    // Method 6: ImGui's SDL2 helper (last resort).
    {
        float scale = ImGui_ImplSDL2_GetContentScaleForWindow(window_);
        fprintf(stderr, "[DPI] Method 6 - ImGui SDL2 helper: %.2f\n", scale);
        if (scale >= 1.1f)
        {
            fprintf(stderr, "[DPI] → Using ImGui helper: %.2f\n", scale);
            return scale;
        }
    }

    fprintf(stderr, "[DPI] No HiDPI detected by any method, using 1.0\n");
    return 1.0f;
}

bool AGSEditorApp::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // --- HiDPI font strategy ---
    //
    // dpi_scale_  = overall content scale (e.g. 2.0 on a 2× HiDPI display)
    // fb_scale_   = framebuffer/window ratio (>1 when compositor handles scaling)
    // ui_scale    = dpi_scale_ / fb_scale_  (the part WE must handle in layout)
    //
    // Compositor-scaled (Wayland):  dpi=2, fb=2, ui=1
    //   → Font loaded at 16*2=32px for crisp glyphs,
    //     FontGlobalScale=1/2 so it occupies 16 logical px → 32 physical px ✓
    //   → Layout helpers (Dpi()) return 1× (compositor upscales already)
    //
    // Non-compositor (X11 HiDPI):  dpi=2, fb=1, ui=2
    //   → Font loaded at 16*2=32px, FontGlobalScale=1 → 32 physical px ✓
    //   → Layout helpers return 2× (we scale widgets ourselves)
    //
    // Normal display:  dpi=1, fb=1, ui=1
    //   → Font at 16px, FontGlobalScale=1 → 16px ✓

    float base_font_size = preferences_.font_size;
    float font_pixel_size = std::floor(base_font_size * dpi_scale_);
    float font_global_scale = 1.0f / fb_scale_;
    float ui_scale = dpi_scale_ / fb_scale_;

    // Try fonts in order of preference (shipped with Dear ImGui)
    const char* font_paths[] = {
        "imgui/misc/fonts/Roboto-Medium.ttf",
        "imgui/misc/fonts/DroidSans.ttf",
        "imgui/misc/fonts/Karla-Regular.ttf",
        nullptr
    };

    // Compute paths relative to the executable/source dir
    std::string editor_dir;
    {
        const char* base_dirs[] = {
            SOURCE_DIR,  // CMake-defined source dir
            ".",
            "..",
            nullptr
        };
        for (const char** bd = base_dirs; *bd; ++bd)
        {
            std::string test = std::string(*bd) + "/imgui/misc/fonts/Roboto-Medium.ttf";
            FILE* f = fopen(test.c_str(), "rb");
            if (f) { fclose(f); editor_dir = *bd; break; }
        }
    }
    editor_dir_ = editor_dir;

    bool font_loaded = false;
    if (!editor_dir.empty())
    {
        for (const char** fp = font_paths; *fp; ++fp)
        {
            std::string full_path = editor_dir + "/" + *fp;
            ImFont* font = io.Fonts->AddFontFromFileTTF(full_path.c_str(), font_pixel_size);
            if (font)
            {
                font_loaded = true;
                fprintf(stderr, "[Info] Loaded font: %s at %.0fpx (dpi=%.2f, fb=%.2f, FontGlobalScale=%.2f)\n",
                    full_path.c_str(), font_pixel_size, dpi_scale_, fb_scale_, font_global_scale);
                break;
            }
        }
    }

    if (font_loaded)
    {
        // Compensate: the font atlas has oversized glyphs (for crispness),
        // but ImGui should lay them out at the correct logical size.
        io.FontGlobalScale = font_global_scale;
    }
    else
    {
        fprintf(stderr, "[Warning] Could not load TTF font, using default bitmap font.\n");
        // The built-in 13px bitmap font needs to be upscaled to match.
        io.FontGlobalScale = dpi_scale_;
    }

    // Try to load icon font (Lucide) merged with main font
    if (font_loaded && !editor_dir.empty())
    {
        const char* icon_font_paths[] = {
            "imgui/misc/fonts/lucide.ttf",
            nullptr
        };
        for (const char** ifp = icon_font_paths; *ifp; ++ifp)
        {
            std::string icon_path = editor_dir + "/" + *ifp;
            FILE* f = fopen(icon_path.c_str(), "rb");
            if (f)
            {
                fclose(f);
                ImFontConfig icon_cfg;
                icon_cfg.MergeMode = true;
                icon_cfg.PixelSnapH = true;
                icon_cfg.GlyphMinAdvanceX = font_pixel_size;
                static const ImWchar icon_ranges[] = { ICON_MIN_LC, ICON_MAX_LC, 0 };
                ImFont* icon_font = io.Fonts->AddFontFromFileTTF(
                    icon_path.c_str(), font_pixel_size, &icon_cfg, icon_ranges);
                if (icon_font)
                {
                    AGSEditor::g_icons_loaded = true;
                    fprintf(stderr, "[Info] Loaded icon font: %s\n", icon_path.c_str());
                }
                break;
            }
        }
        if (!AGSEditor::g_icons_loaded)
        {
            fprintf(stderr, "[Info] No icon font found. Place lucide.ttf in imgui/misc/fonts/ for icons.\n");
        }
    }

    // Use a dark editor theme
    ImGui::StyleColorsDark();

    // Scale UI widget sizes by the part the compositor doesn't handle.
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(ui_scale);

    if (!font_loaded)
        style.FontScaleDpi = ui_scale;

    // Customize editor colors for a professional look
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.35f, 0.80f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.30f, 0.45f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.25f, 0.28f, 0.38f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.33f, 0.45f, 1.00f);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer2_Init(renderer_);

    return true;
}

void AGSEditorApp::Run()
{
    bool done = false;
    const ImVec4 clear_color(0.10f, 0.10f, 0.12f, 1.00f);

    while (!done)
    {
        ProcessEvents(done);

        if (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        BeginFrame();

        // Draw editor UI
        ui_->Draw();

        // Shutdown guard confirmation dialog
        if (show_quit_confirm_)
        {
            ImGui::OpenPopup("Quit While Testing?");
            show_quit_confirm_ = false;
        }
        if (ImGui::BeginPopupModal("Quit While Testing?", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("A test game is still running.");
            ImGui::Text("Do you want to stop the game and quit the editor?");
            ImGui::Spacing();
            if (ImGui::Button("Stop and Quit", ImVec2(AGSEditor::Dpi(140), 0)))
            {
                auto* build_pane = ui_->FindPane<AGSEditor::BuildPane>();
                if (build_pane) build_pane->GetBuildSystem().StopGame();
                done = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(AGSEditor::Dpi(100), 0)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // Render
        ImGui::Render();
        ImGuiIO& io = ImGui::GetIO();
        SDL_RenderSetScale(renderer_, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer_,
            (Uint8)(clear_color.x * 255),
            (Uint8)(clear_color.y * 255),
            (Uint8)(clear_color.z * 255),
            (Uint8)(clear_color.w * 255));
        SDL_RenderClear(renderer_);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
        SDL_RenderPresent(renderer_);
    }
}

bool AGSEditorApp::IsGameRunning() const
{
    if (!ui_) return false;
    auto* build_pane = ui_->FindPane<AGSEditor::BuildPane>();
    return build_pane && build_pane->GetBuildSystem().IsRunning();
}

void AGSEditorApp::ProcessEvents(bool& done)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        bool quit_requested = false;
        if (event.type == SDL_QUIT)
            quit_requested = true;
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(window_))
            quit_requested = true;

        if (quit_requested)
        {
            if (IsGameRunning())
                show_quit_confirm_ = true; // show confirmation instead
            else
                done = true;
        }

        // Handle file drops (drag external files onto the window)
        if (event.type == SDL_DROPFILE && event.drop.file)
        {
            std::string dropped_file = event.drop.file;
            SDL_free(event.drop.file);

            // Check if it's an AGS project file
            if (dropped_file.size() > 4)
            {
                std::string ext = dropped_file.substr(dropped_file.size() - 4);
                if (ext == ".agf" || ext == ".ags" || ext == ".dta")
                {
                    if (project_->OpenProject(dropped_file))
                    {
                        LoadGameFonts();
                        if (ui_)
                            ui_->NotifyProjectLoaded();
                    }
                }
            }
        }
    }
}

void AGSEditorApp::BeginFrame()
{
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Process keyboard shortcuts each frame
    shortcut_manager_.ProcessShortcuts();
}

void AGSEditorApp::RegisterShortcuts()
{
    // Shortcuts are registered here but some callbacks need the UI,
    // which is created after Init(). The callbacks capture by reference
    // so they'll work once the UI is available.

    // File shortcuts
    shortcut_manager_.Register("file.save", "Ctrl+S", ImGuiKey_S, AGSEditor::kModCtrl,
        "File", [this]() {
            if (project_ && project_->IsLoaded())
                project_->SaveProject();
        });

    // Edit shortcuts
    shortcut_manager_.Register("edit.undo", "Ctrl+Z", ImGuiKey_Z, AGSEditor::kModCtrl,
        "Edit", [this]() { undo_manager_.Undo(); });

    shortcut_manager_.Register("edit.redo", "Ctrl+Y", ImGuiKey_Y, AGSEditor::kModCtrl,
        "Edit", [this]() { undo_manager_.Redo(); });
}

void AGSEditorApp::Shutdown()
{
    ui_.reset();
    project_.reset();

    if (!headless_mode_)
    {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        if (renderer_)
            SDL_DestroyRenderer(renderer_);
        if (window_)
            SDL_DestroyWindow(window_);
        SDL_Quit();
    }
}

// =========================================================================
// Headless CLI operations (--compile, --package, --run)
// =========================================================================
int AGSEditorApp::RunHeadless()
{
    if (exit_code_ != 0)
        return exit_code_;

    const char* action_name = "unknown";
    switch (cli_action_)
    {
        case CLIAction::Compile: action_name = "compile"; break;
        case CLIAction::Package: action_name = "package"; break;
        case CLIAction::Run:     action_name = "run"; break;
        default: break;
    }

    fprintf(stderr, "[CLI] Action: %s\n", action_name);
    fprintf(stderr, "[CLI] Project: %s\n", cli_project_path_.c_str());

    // Open the project
    if (!project_->OpenProject(cli_project_path_))
    {
        fprintf(stderr, "[Error] Failed to open project: %s\n", cli_project_path_.c_str());
        return 1;
    }
    fprintf(stderr, "[CLI] Project opened successfully.\n");

    // Set up build system
    AGSEditor::BuildSystem build_system;
    build_system.SetLogCallback([](const std::string& msg) {
        fprintf(stderr, "%s\n", msg.c_str());
    });

    AGSEditor::BuildConfig config;
    config.SetDefaults(project_->GetProjectDir());
    config.debug_mode = false;

    // For --compile only, we just compile scripts (no packaging)
    // For --package, we do a full build (compile + package)
    // For --run, we do a full build then launch the game
    if (cli_action_ == CLIAction::Compile)
    {
        // Only compile: enable DataFile target (scripts + game data)
        config.targets[(int)AGSEditor::BuildTarget::Linux] = false;
        config.targets[(int)AGSEditor::BuildTarget::Windows] = false;
        config.targets[(int)AGSEditor::BuildTarget::MacOS] = false;
        config.targets[(int)AGSEditor::BuildTarget::Web] = false;
        config.targets[(int)AGSEditor::BuildTarget::Debug] = false;
    }
    else
    {
        // Package/Run: enable DataFile + platform targets
#ifdef _WIN32
        config.targets[(int)AGSEditor::BuildTarget::Windows] = true;
#else
        config.targets[(int)AGSEditor::BuildTarget::Linux] = true;
#endif
        if (cli_action_ == CLIAction::Run)
            config.targets[(int)AGSEditor::BuildTarget::Debug] = true;
    }

    // Build
    AGSEditor::BuildResult result = build_system.BuildGame(*project_, config);

    int errors = result.ErrorCount();
    int warnings = result.WarningCount();
    fprintf(stderr, "[CLI] Build %s: %d error(s), %d warning(s) in %.2fs\n",
            result.success ? "succeeded" : "FAILED",
            errors, warnings, result.elapsed_seconds);

    if (!result.success)
    {
        // Print all error messages
        for (const auto& msg : result.messages)
        {
            if (msg.type == AGSEditor::BuildMessageType::Error)
            {
                if (!msg.file.empty())
                    fprintf(stderr, "  %s(%d): %s\n", msg.file.c_str(), msg.line, msg.message.c_str());
                else
                    fprintf(stderr, "  %s\n", msg.message.c_str());
            }
        }
        return 1;
    }

    // For --run, launch the game and wait for it to exit
    if (cli_action_ == CLIAction::Run)
    {
        if (!build_system.RunGame(*project_, config, false))
        {
            fprintf(stderr, "[Error] Failed to launch game.\n");
            return 1;
        }

        fprintf(stderr, "[CLI] Game launched, waiting for exit...\n");

        // Wait for the game process to finish
#ifndef _WIN32
        // Poll for game output while waiting
        while (build_system.IsRunning())
        {
            build_system.PollGameOutput();
            usleep(100000); // 100ms
        }
        fprintf(stderr, "[CLI] Game exited.\n");
#else
        // On Windows, just wait a bit and check
        while (build_system.IsRunning())
        {
            build_system.PollGameOutput();
            Sleep(100);
        }
        fprintf(stderr, "[CLI] Game exited.\n");
#endif
    }

    return 0;
}

void AGSEditorApp::LoadGameFonts()
{
    namespace fs = std::filesystem;

    game_fonts_.clear();

    if (!project_ || !project_->IsLoaded()) return;
    auto* gd = project_->GetGameData();
    if (!gd) return;

    std::string proj_dir = project_->GetProjectDir();
    ImGuiIO& io = ImGui::GetIO();
    bool any_loaded = false;

    for (const auto& fnt : gd->fonts)
    {
        // AGS convention: agsfntN.ttf
        char ttf_name[64];
        snprintf(ttf_name, sizeof(ttf_name), "agsfnt%d.ttf", fnt.id);
        fs::path ttf_path = fs::path(proj_dir) / ttf_name;

        if (!fs::exists(ttf_path)) continue;

        float font_size = (float)fnt.size * (float)fnt.size_multiplier * AGSEditor::g_dpi_scale;
        if (font_size < 8.0f) font_size = 8.0f;
        if (font_size > 120.0f) font_size = 120.0f;

        ImFontConfig cfg;
        cfg.MergeMode = false;
        snprintf(cfg.Name, sizeof(cfg.Name), "AGS Font %d (%s)", fnt.id, fnt.name.c_str());

        ImFont* loaded = io.Fonts->AddFontFromFileTTF(ttf_path.string().c_str(), font_size, &cfg);
        if (loaded)
        {
            game_fonts_[fnt.id] = loaded;
            any_loaded = true;
            fprintf(stderr, "[Info] Loaded game font %d: %s at %.0fpx\n",
                    fnt.id, ttf_path.string().c_str(), font_size);
        }
    }

    if (any_loaded)
    {
        // Rebuild font atlas — the SDL renderer backend handles texture updates automatically
        io.Fonts->Build();
        fprintf(stderr, "[Info] Game fonts loaded: %d TTF font(s)\n", (int)game_fonts_.size());
    }
}

ImFont* AGSEditorApp::GetGameFont(int font_id) const
{
    auto it = game_fonts_.find(font_id);
    return (it != game_fonts_.end()) ? it->second : nullptr;
}
