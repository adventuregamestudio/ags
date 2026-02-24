// AGS Editor ImGui - Build Pane
// Provides build output, error list with clickable navigation,
// build configuration, progress display, and debugger controls.
#pragma once

#include "ui/editor_ui.h"
#include "pipeline/build_system.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace AGSEditor
{

class BuildPane : public EditorPane
{
public:
    explicit BuildPane(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Build"; }

    // Navigate to a script error (opens script editor at line)
    using NavigateCallback = std::function<void(const std::string& file, int line)>;
    void SetNavigateCallback(NavigateCallback cb) { navigate_cb_ = std::move(cb); }

    // Access build system
    BuildSystem& GetBuildSystem() { return build_system_; }

    // Trigger actions
    void StartBuild();
    void StartRun(bool debug = false);
    void StopRun();

private:
    void DrawToolbar();
    void DrawErrorList();
    void DrawBuildLog();
    void DrawBuildConfig();
    void DrawProgress();
    void DrawCallStack();

    // Debug session helpers
    void SyncBreakpointsToEngine();
    void UpdateExecutionPoint();
    void ClearExecutionPoints();

    EditorUI& editor_;
    BuildSystem build_system_;
    NavigateCallback navigate_cb_;

    // UI state
    int active_tab_ = 0;           // 0=Error List, 1=Build Config
    bool show_errors_ = true;
    bool show_warnings_ = true;
    bool show_info_ = false;

    // Error list sorting
    int sort_column_ = -1;
    bool sort_ascending_ = true;

    // Build config local copy
    BuildConfig build_config_;
    bool config_initialized_ = false;

    // Engine path input buffer
    char engine_linux_path_[512] = {};
    char engine_windows_path_[512] = {};
    char output_dir_[512] = {};

    // Debug execution tracking
    std::string last_exec_script_;
    int last_exec_line_ = 0;
};

} // namespace AGSEditor
