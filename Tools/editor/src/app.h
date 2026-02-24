// AGS Editor ImGui - Application header
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>

#include "core/preferences.h"
#include "core/undo_manager.h"
#include "core/shortcut_manager.h"
#include "core/clipboard_manager.h"
#include "core/audio_player.h"
#include "project/template_manager.h"
#include "project/texture_cache.h"

struct SDL_Window;
struct SDL_Renderer;
struct ImFont;

namespace AGSEditor { class EditorUI; }
namespace AGSEditor { class Project; }
namespace AGSEditor { extern bool g_icons_loaded; }

class AGSEditorApp
{
public:
    AGSEditorApp();
    ~AGSEditorApp();

    bool Init(int argc, char* argv[]);
    void Run();
    void Shutdown();

    // Headless CLI operations (no GUI)
    bool IsHeadless() const { return headless_mode_; }
    int GetExitCode() const { return exit_code_; }
    int RunHeadless();

    // Access
    SDL_Window* GetWindow() const { return window_; }
    SDL_Renderer* GetRenderer() const { return renderer_; }
    AGSEditor::Project* GetProject() const { return project_.get(); }
    float GetDpiScale() const { return dpi_scale_; }

    // Core managers
    AGSEditor::UndoManager& GetUndoManager() { return undo_manager_; }
    AGSEditor::ShortcutManager* GetShortcutManager() { return &shortcut_manager_; }
    AGSEditor::ClipboardManager& GetClipboard() { return clipboard_; }

    // Preferences
    const AGSEditor::EditorPreferences& GetPreferences() const { return preferences_; }
    AGSEditor::EditorPreferences& GetPreferences() { return preferences_; }
    void SetPreferences(const AGSEditor::EditorPreferences& prefs) { preferences_ = prefs; }

    // Templates
    AGSEditor::TemplateManager& GetTemplateManager() { return template_manager_; }
    const std::string& GetEditorDir() const { return editor_dir_; }

    // Texture cache for GPU sprite/bitmap rendering
    AGSEditor::TextureCache& GetTextureCache() { return texture_cache_; }

    // Audio player for editor preview
    AGSEditor::AudioPlayer& GetAudioPlayer() { return audio_player_; }

    // Check if a test game is currently running
    bool IsGameRunning() const;

    // Game font management (loaded from project TTFs)
    void LoadGameFonts();
    ImFont* GetGameFont(int font_id) const;

private:
    bool InitSDL();
    bool InitImGui();
    float DetectDpiScale();
    void RegisterShortcuts();
    void ProcessEvents(bool& done);
    void BeginFrame();
    void RenderFrame();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    std::unique_ptr<AGSEditor::EditorUI> ui_;
    std::unique_ptr<AGSEditor::Project> project_;

    // Core systems
    AGSEditor::UndoManager undo_manager_;
    AGSEditor::ShortcutManager shortcut_manager_;
    AGSEditor::ClipboardManager clipboard_;
    AGSEditor::EditorPreferences preferences_;
    AGSEditor::TemplateManager template_manager_;
    AGSEditor::TextureCache texture_cache_;
    AGSEditor::AudioPlayer audio_player_;
    std::string editor_dir_;
    std::map<int, ImFont*> game_fonts_;   // font_id -> ImFont* (loaded from project TTFs)

    std::string window_title_;
    float dpi_scale_ = 0.0f;     // overall desired content scale (0 = auto-detect)
    float fb_scale_ = 1.0f;      // framebuffer / window ratio (compositor scaling)
    bool show_quit_confirm_ = false; // shutdown guard during testing

    // Headless CLI mode
    bool headless_mode_ = false;
    int exit_code_ = 0;
    enum class CLIAction { None, Compile, Package, Run };
    CLIAction cli_action_ = CLIAction::None;
    std::string cli_project_path_;
};
