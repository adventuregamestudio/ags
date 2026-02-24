// AGS Editor ImGui - Build Pane implementation
// Provides build output UI, error list with clickable navigation,
// build settings panel, and progress display.

#include "build_pane.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "ui/file_dialog.h"
#include "ui/panes/script_editor.h"
#include "project/project.h"
#include "core/dpi_helper.h"
#include "app.h"

#include "imgui.h"
#include "../IconsLucide.h"
#include "TextEditor.h"

#include <cstring>
#include <cstdio>
#include <string>

namespace AGSEditor
{

BuildPane::BuildPane(EditorUI& editor)
    : editor_(editor)
{
    // Hook the build system log output to the editor log panel
    build_system_.SetLogCallback([this](const std::string& msg) {
        editor_.GetLogPanel().AddLog("%s", msg.c_str());
    });
}

// -------------------------------------------------------------------------
// Main Draw
// -------------------------------------------------------------------------
void BuildPane::Draw()
{
    // Initialize config from project on first draw
    if (!config_initialized_)
    {
        auto* project = editor_.GetProject();
        if (project && project->IsLoaded())
        {
            build_config_.SetDefaults(project->GetProjectDir());
            snprintf(output_dir_, sizeof(output_dir_), "%s",
                     build_config_.output_base_dir.c_str());
            snprintf(engine_linux_path_, sizeof(engine_linux_path_), "%s",
                     build_config_.engine_linux_path.c_str());
            snprintf(engine_windows_path_, sizeof(engine_windows_path_), "%s",
                     build_config_.engine_windows_path.c_str());
        }
        config_initialized_ = true;
    }

    DrawToolbar();

    // Progress bar (if building)
    if (build_system_.IsBuilding())
    {
        DrawProgress();
    }

    // Tab bar: Error List | Build Log | Build Config | Call Stack (debug only)
    if (ImGui::BeginTabBar("BuildTabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Error List"))
        {
            active_tab_ = 0;
            DrawErrorList();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Build Output"))
        {
            active_tab_ = 1;
            DrawBuildLog();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Build Settings"))
        {
            active_tab_ = 2;
            DrawBuildConfig();
            ImGui::EndTabItem();
        }

        // Call Stack tab — only shown during debug sessions
        if (build_system_.IsDebugRun() && build_system_.GetDebugController().IsActive())
        {
            if (ImGui::BeginTabItem("Call Stack"))
            {
                active_tab_ = 3;
                DrawCallStack();
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }

    // Sync breakpoints and handle debug state changes during debug sessions
    if (build_system_.IsDebugRun() && build_system_.GetDebugController().IsActive())
    {
        SyncBreakpointsToEngine();
        UpdateExecutionPoint();
    }
}

// -------------------------------------------------------------------------
// Toolbar
// -------------------------------------------------------------------------
void BuildPane::DrawToolbar()
{
    auto* project = editor_.GetProject();
    bool has_project = project && project->IsLoaded();
    bool is_building = build_system_.IsBuilding();
    bool is_running = build_system_.IsRunning();
    auto& dbg = build_system_.GetDebugController();
    bool is_debug = build_system_.IsDebugRun();
    bool is_paused = is_debug && dbg.IsPaused();

    // Build button
    ImGui::BeginDisabled(!has_project || is_building);
    {
        std::string lbl = std::string(g_icons_loaded ? ICON_LC_HAMMER " " : "") + "Build (F7)";
        if (ImGui::Button(lbl.c_str()))
            StartBuild();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Run button — when paused in debugger, this becomes Resume
    ImGui::BeginDisabled(!has_project || is_building);
    if (is_paused)
    {
        // Resume button (replaces Run when paused)
        std::string resume_lbl = std::string(g_icons_loaded ? ICON_LC_PLAY " " : "") + "Resume (F5)";
        if (ImGui::Button(resume_lbl.c_str()))
        {
            dbg.Resume();
            ClearExecutionPoints();
        }
    }
    else
    {
        std::string run_lbl = std::string(g_icons_loaded ? ICON_LC_PLAY " " : "") +
            (is_running ? "Restart (F5)" : "Run (F5)");
        if (ImGui::Button(run_lbl.c_str()))
        {
            if (is_running)
                build_system_.StopGame();
            StartRun(false);
        }
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Debug run button
    ImGui::BeginDisabled(!has_project || is_building || is_paused);
    {
        std::string dbg_lbl = std::string(g_icons_loaded ? ICON_LC_BUG " " : "") + "Debug (Ctrl+F5)";
        if (ImGui::Button(dbg_lbl.c_str()))
        {
            if (is_running)
                build_system_.StopGame();
            StartRun(true);
        }
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Step Into button — only enabled when paused in debugger
    ImGui::BeginDisabled(!is_paused);
    {
        std::string step_lbl = std::string(g_icons_loaded ? ICON_LC_ARROW_DOWN_TO_LINE " " : "") + "Step (F11)";
        if (ImGui::Button(step_lbl.c_str()))
        {
            dbg.StepInto();
            ClearExecutionPoints();
        }
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Stop button
    ImGui::BeginDisabled(!is_running);
    {
        std::string stop_lbl = std::string(g_icons_loaded ? ICON_LC_CIRCLE_STOP " " : "") + "Stop (Shift+F5)";
        if (ImGui::Button(stop_lbl.c_str()))
            StopRun();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Last build result
    const auto& last = build_system_.GetLastResult();
    if (last.elapsed_seconds > 0.0)
    {
        if (last.success)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
            ImGui::Text("Build succeeded (%.1fs)", last.elapsed_seconds);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::Text("Build failed: %d error(s) (%.1fs)",
                        last.ErrorCount(), last.elapsed_seconds);
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("| %d warning(s)", last.WarningCount());
    }

    // Debug status
    if (is_debug && is_running)
    {
        ImGui::SameLine();
        if (is_paused)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.2f, 1.0f));
            if (dbg.HasError())
                ImGui::Text("| PAUSED (error: %s)", dbg.GetLastError().c_str());
            else
                ImGui::Text("| PAUSED at %s:%d", dbg.GetCurrentScript().c_str(), dbg.GetCurrentLine());
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 1.0f, 1.0f));
            ImGui::Text("| Debugging...");
            ImGui::PopStyleColor();
        }
    }
    else if (is_running)
    {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 1.0f, 1.0f));
        ImGui::Text("| Game running (PID: %d)", build_system_.IsRunning() ? 1 : 0);
        ImGui::PopStyleColor();
    }

    ImGui::Separator();
}

// -------------------------------------------------------------------------
// Error List — clickable errors that navigate to script editor
// -------------------------------------------------------------------------
void BuildPane::DrawErrorList()
{
    const auto& result = build_system_.GetLastResult();

    // Filter buttons
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    int error_count = result.ErrorCount();
    char err_label[64];
    snprintf(err_label, sizeof(err_label), "Errors (%d)###Errors", error_count);
    if (ImGui::Selectable(err_label, show_errors_, 0, ImVec2(Dpi(100), 0)))
        show_errors_ = !show_errors_;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
    int warn_count = result.WarningCount();
    char warn_label[64];
    snprintf(warn_label, sizeof(warn_label), "Warnings (%d)###Warnings", warn_count);
    if (ImGui::Selectable(warn_label, show_warnings_, 0, ImVec2(Dpi(120), 0)))
        show_warnings_ = !show_warnings_;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    int info_count = (int)result.messages.size() - error_count - warn_count;
    char info_label[64];
    snprintf(info_label, sizeof(info_label), "Info (%d)###Info", info_count);
    if (ImGui::Selectable(info_label, show_info_, 0, ImVec2(Dpi(80), 0)))
        show_info_ = !show_info_;

    ImGui::Separator();

    // Error/Warning/Info table
    if (ImGui::BeginTable("ErrorList", 5,
        ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable |
        ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Dpi(24));         // Icon
        ImGui::TableSetupColumn("Module", ImGuiTableColumnFlags_WidthFixed, Dpi(120));
        ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthFixed, Dpi(200));
        ImGui::TableSetupColumn("Line", ImGuiTableColumnFlags_WidthFixed, Dpi(50));
        ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const auto& msg : result.messages)
        {
            // Filter
            if (msg.type == BuildMessageType::Error && !show_errors_) continue;
            if (msg.type == BuildMessageType::Warning && !show_warnings_) continue;
            if (msg.type == BuildMessageType::Info && !show_info_) continue;

            ImGui::TableNextRow();

            // Icon column
            ImGui::TableSetColumnIndex(0);
            switch (msg.type) {
                case BuildMessageType::Error:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                    ImGui::TextUnformatted(g_icons_loaded ? ICON_LC_CIRCLE_X : "X");
                    ImGui::PopStyleColor();
                    break;
                case BuildMessageType::Warning:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
                    ImGui::TextUnformatted(g_icons_loaded ? ICON_LC_TRIANGLE_ALERT : "!");
                    ImGui::PopStyleColor();
                    break;
                case BuildMessageType::Info:
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
                    ImGui::TextUnformatted(g_icons_loaded ? ICON_LC_INFO : "i");
                    ImGui::PopStyleColor();
                    break;
            }

            // Module column
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(msg.module_name.c_str());

            // File column
            ImGui::TableSetColumnIndex(2);
            if (!msg.file.empty() && msg.line > 0)
            {
                // Make clickable - navigate to error location
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
                char link_text[256];
                // Show just filename, not full path
                const char* filename = msg.file.c_str();
                const char* sep = strrchr(filename, '/');
                if (!sep) sep = strrchr(filename, '\\');
                if (sep) filename = sep + 1;
                snprintf(link_text, sizeof(link_text), "%s", filename);

                if (ImGui::Selectable(link_text, false,
                    ImGuiSelectableFlags_SpanAllColumns))
                {
                    if (navigate_cb_)
                        navigate_cb_(msg.file, msg.line);
                }
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::TextUnformatted(msg.file.c_str());
            }

            // Line column
            ImGui::TableSetColumnIndex(3);
            if (msg.line > 0)
                ImGui::Text("%d", msg.line);

            // Message column
            ImGui::TableSetColumnIndex(4);
            ImGui::TextWrapped("%s", msg.message.c_str());
        }

        ImGui::EndTable();
    }
}

// -------------------------------------------------------------------------
// Build Log — shows the build output (mirrors log panel)
// -------------------------------------------------------------------------
void BuildPane::DrawBuildLog()
{
    // Poll game output if a game is running
    build_system_.PollGameOutput();

    const auto& result = build_system_.GetLastResult();

    if (ImGui::Button("Clear"))
    {
        // Nothing to clear in the result — the log panel handles its own clearing
    }

    ImGui::SameLine();
    ImGui::TextDisabled("Build messages: %d", (int)result.messages.size());

    ImGui::Separator();

    if (ImGui::BeginChild("BuildLogScroll", ImVec2(0, 0), ImGuiChildFlags_None,
        ImGuiWindowFlags_HorizontalScrollbar))
    {
        for (const auto& msg : result.messages)
        {
            ImVec4 color(1.0f, 1.0f, 1.0f, 1.0f);
            switch (msg.type) {
                case BuildMessageType::Error:
                    color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                    break;
                case BuildMessageType::Warning:
                    color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
                    break;
                case BuildMessageType::Info:
                    color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
                    break;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            if (!msg.file.empty() && msg.line > 0)
            {
                ImGui::Text("[%s] %s(%d): %s",
                    msg.type == BuildMessageType::Error ? "Error" :
                    msg.type == BuildMessageType::Warning ? "Warning" : "Info",
                    msg.file.c_str(), msg.line, msg.message.c_str());
            }
            else
            {
                ImGui::Text("[%s] %s",
                    msg.type == BuildMessageType::Error ? "Error" :
                    msg.type == BuildMessageType::Warning ? "Warning" : "Info",
                    msg.message.c_str());
            }
            ImGui::PopStyleColor();
        }

        // Auto-scroll
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}

// -------------------------------------------------------------------------
// Build Configuration
// -------------------------------------------------------------------------
void BuildPane::DrawBuildConfig()
{
    ImGui::TextDisabled("Build Configuration");
    ImGui::Separator();

    bool changed = false;

    // --- General ---
    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        changed |= ImGui::Checkbox("Debug mode", &build_config_.debug_mode);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Compile scripts with DEBUG macro defined");

        changed |= ImGui::Checkbox("Line numbers", &build_config_.line_numbers);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Include source line numbers in compiled scripts (for error reporting)");

        ImGui::Spacing();
    }

    // --- Output Paths ---
    if (ImGui::CollapsingHeader("Output Paths", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Output directory:");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - Dpi(40));
        if (ImGui::InputText("##OutputDir", output_dir_, sizeof(output_dir_)))
        {
            build_config_.output_base_dir = output_dir_;
            changed = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Base directory for compiled output (default: <project>/Compiled)");
        ImGui::SameLine();
        if (ImGui::Button("...##BrowseOutputDir"))
        {
            FileDialog::Open(FileDialogType::SelectFolder, "Select Output Directory",
                "", output_dir_,
                [this](const std::string& path) {
                    snprintf(output_dir_, sizeof(output_dir_), "%s", path.c_str());
                    build_config_.output_base_dir = path;
                });
        }

        ImGui::Spacing();
    }

    // --- Target Platforms ---
    if (ImGui::CollapsingHeader("Target Platforms", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Data file is always enabled
        ImGui::BeginDisabled(true);
        bool data_target = true;
        ImGui::Checkbox("Data File (always)", &data_target);
        ImGui::EndDisabled();

        changed |= ImGui::Checkbox("Linux", &build_config_.targets[(int)BuildTarget::Linux]);
        changed |= ImGui::Checkbox("Windows", &build_config_.targets[(int)BuildTarget::Windows]);
        changed |= ImGui::Checkbox("macOS", &build_config_.targets[(int)BuildTarget::MacOS]);
        changed |= ImGui::Checkbox("Web (Emscripten)", &build_config_.targets[(int)BuildTarget::Web]);

        ImGui::Spacing();
    }

    // --- Engine Paths ---
    if (ImGui::CollapsingHeader("Engine Paths"))
    {
        ImGui::Text("Linux engine (ags):");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - Dpi(40));
        if (ImGui::InputText("##LinuxEngine", engine_linux_path_, sizeof(engine_linux_path_)))
        {
            build_config_.engine_linux_path = engine_linux_path_;
            changed = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Path to 'ags' binary for Linux builds. Leave empty to auto-detect.");
        ImGui::SameLine();
        if (ImGui::Button("...##BrowseLinuxEngine"))
        {
            FileDialog::Open(FileDialogType::OpenFile, "Select Linux Engine",
                ".*{All Files}", engine_linux_path_,
                [this](const std::string& path) {
                    snprintf(engine_linux_path_, sizeof(engine_linux_path_), "%s", path.c_str());
                    build_config_.engine_linux_path = path;
                });
        }

        ImGui::Text("Windows engine (acwin.exe):");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - Dpi(40));
        if (ImGui::InputText("##WinEngine", engine_windows_path_, sizeof(engine_windows_path_)))
        {
            build_config_.engine_windows_path = engine_windows_path_;
            changed = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Path to 'acwin.exe' for Windows builds.");
        ImGui::SameLine();
        if (ImGui::Button("...##BrowseWinEngine"))
        {
            FileDialog::Open(FileDialogType::OpenFile, "Select Windows Engine",
                ".exe{Executables},.*{All Files}", engine_windows_path_,
                [this](const std::string& path) {
                    snprintf(engine_windows_path_, sizeof(engine_windows_path_), "%s", path.c_str());
                    build_config_.engine_windows_path = path;
                });
        }

        ImGui::Spacing();
    }

    // --- Script API ---
    if (ImGui::CollapsingHeader("Script API"))
    {
        const char* api_versions[] = {
            "Highest", "v360", "v351", "v3507", "v350",
            "v341", "v340", "v335", "v334", "v330", "v321"
        };
        int api_count = 11;

        int current_api = 0;
        for (int i = 0; i < api_count; i++)
        {
            if (build_config_.script_api_version == api_versions[i])
            {
                current_api = i;
                break;
            }
        }
        ImGui::Text("Script API version:");
        if (ImGui::Combo("##ScriptAPI", &current_api, api_versions, api_count))
        {
            build_config_.script_api_version = api_versions[current_api];
            changed = true;
        }

        int current_compat = 0;
        for (int i = 0; i < api_count; i++)
        {
            if (build_config_.script_compat_level == api_versions[i])
            {
                current_compat = i;
                break;
            }
        }
        ImGui::Text("Script compatibility level:");
        if (ImGui::Combo("##ScriptCompat", &current_compat, api_versions, api_count))
        {
            build_config_.script_compat_level = api_versions[current_compat];
            changed = true;
        }

        ImGui::Spacing();
    }

    // --- Package Options ---
    if (ImGui::CollapsingHeader("Package Options"))
    {
        changed |= ImGui::InputInt("Split size (MB, 0=none)", &build_config_.split_size_mb);
        if (build_config_.split_size_mb < 0) build_config_.split_size_mb = 0;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Split game.ags into parts this size (for distribution). 0 = single file.");

        ImGui::Spacing();
    }

    // Apply button
    ImGui::Separator();
    if (ImGui::Button("Apply Settings"))
    {
        build_system_.SetConfig(build_config_);
        editor_.GetLogPanel().AddLog("[Info] Build settings updated.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset to Defaults"))
    {
        auto* project = editor_.GetProject();
        if (project && project->IsLoaded())
        {
            build_config_.SetDefaults(project->GetProjectDir());
            snprintf(output_dir_, sizeof(output_dir_), "%s",
                     build_config_.output_base_dir.c_str());
            snprintf(engine_linux_path_, sizeof(engine_linux_path_), "%s",
                     build_config_.engine_linux_path.c_str());
            snprintf(engine_windows_path_, sizeof(engine_windows_path_), "%s",
                     build_config_.engine_windows_path.c_str());
            build_system_.SetConfig(build_config_);
            editor_.GetLogPanel().AddLog("[Info] Build settings reset to defaults.");
        }
    }

    (void)changed;
}

// -------------------------------------------------------------------------
// Progress display
// -------------------------------------------------------------------------
void BuildPane::DrawProgress()
{
    const auto& progress = build_system_.GetProgress();

    ImGui::ProgressBar(progress.percentage,
        ImVec2(-1.0f, 0.0f),
        progress.step_description.c_str());

    ImGui::Text("Step %d/%d: %s",
        progress.current_step, progress.total_steps,
        progress.step_description.c_str());
}

// -------------------------------------------------------------------------
// Actions
// -------------------------------------------------------------------------
void BuildPane::StartBuild()
{
    auto* project = editor_.GetProject();
    if (!project || !project->IsLoaded())
    {
        editor_.GetLogPanel().AddLog("[Error] No project loaded.");
        return;
    }

    // Apply current config
    build_system_.SetConfig(build_config_);

    // Run build
    auto result = build_system_.BuildGame(*project, build_config_);

    // Log summary is already handled by the BuildSystem log callback
    // Switch to error list tab if there are errors
    if (!result.success)
        active_tab_ = 0;
}

void BuildPane::StartRun(bool debug)
{
    auto* project = editor_.GetProject();
    if (!project || !project->IsLoaded())
    {
        editor_.GetLogPanel().AddLog("[Error] No project loaded.");
        return;
    }

    // Check if we need to build first
    const auto& last = build_system_.GetLastResult();
    if (!last.success || last.elapsed_seconds < 0.001)
    {
        editor_.GetLogPanel().AddLog("[Build] Building before run...");

        // Enable debug target
        BuildConfig run_config = build_config_;
        if (debug)
            run_config.targets[(int)BuildTarget::Debug] = true;

        auto result = build_system_.BuildGame(*project, run_config);
        if (!result.success)
        {
            editor_.GetLogPanel().AddLog("[Error] Build failed. Cannot run game.");
            active_tab_ = 0;
            return;
        }
    }

    build_system_.RunGame(*project, build_config_, debug);

    // If this is a debug run, sync initial breakpoints to the debug controller
    if (debug && build_system_.IsDebugRun())
    {
        SyncBreakpointsToEngine();
    }
}

void BuildPane::StopRun()
{
    ClearExecutionPoints();
    build_system_.StopGame();
}

// -------------------------------------------------------------------------
// Call Stack display
// -------------------------------------------------------------------------
void BuildPane::DrawCallStack()
{
    auto& dbg = build_system_.GetDebugController();
    const auto& stack = dbg.GetCallStack();

    if (stack.empty())
    {
        ImGui::TextDisabled("(no call stack available)");
        return;
    }

    if (dbg.HasError())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextWrapped("Error: %s", dbg.GetLastError().c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    // Display call stack entries — clickable to navigate to source
    for (size_t i = 0; i < stack.size(); ++i)
    {
        const auto& entry = stack[i];
        char label[256];
        if (entry.line_number > 0)
            snprintf(label, sizeof(label), "%s%s : line %d",
                     i == 0 ? "> " : "  ",
                     entry.script_name.c_str(), entry.line_number);
        else
            snprintf(label, sizeof(label), "%s%s",
                     i == 0 ? "> " : "  ",
                     entry.script_name.c_str());

        if (i == 0)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.3f, 1.0f));

        if (ImGui::Selectable(label) && entry.line_number > 0)
        {
            // Navigate to the script and line
            editor_.OpenScriptFile(entry.script_name);
            // Find the ScriptEditor and go to line
            for (auto& pane : editor_.GetPanes())
            {
                auto* se = dynamic_cast<ScriptEditor*>(pane.get());
                if (se && se->GetFilename() == entry.script_name)
                {
                    se->GoToLine(entry.line_number);
                    break;
                }
            }
        }

        if (i == 0)
            ImGui::PopStyleColor();
    }
}

// -------------------------------------------------------------------------
// Breakpoint sync — collect from all open ScriptEditors, send to engine
// -------------------------------------------------------------------------
void BuildPane::SyncBreakpointsToEngine()
{
    auto& dbg = build_system_.GetDebugController();
    if (!dbg.IsActive())
        return;

    std::unordered_map<std::string, std::unordered_set<int>> all_breakpoints;

    // Iterate all open ScriptEditor panes and collect breakpoints
    for (auto& pane : editor_.GetPanes())
    {
        auto* se = dynamic_cast<ScriptEditor*>(pane.get());
        if (!se)
            continue;

        const auto& bps = se->GetBreakpoints();
        if (!bps.empty())
        {
            all_breakpoints[se->GetFilename()] = bps;
        }
    }

    dbg.SyncBreakpoints(all_breakpoints);
}

// -------------------------------------------------------------------------
// Execution point — set/clear the yellow arrow in the correct ScriptEditor
// -------------------------------------------------------------------------
void BuildPane::UpdateExecutionPoint()
{
    auto& dbg = build_system_.GetDebugController();
    if (!dbg.IsPaused())
        return;

    const std::string& script = dbg.GetCurrentScript();
    int line = dbg.GetCurrentLine();

    if (script.empty() || line <= 0)
        return;

    // Only update once per pause (avoid re-navigating every frame)
    if (script == last_exec_script_ && line == last_exec_line_)
        return;

    last_exec_script_ = script;
    last_exec_line_ = line;

    // Clear previous execution points from all editors
    ClearExecutionPoints();

    // Open/focus the script and set the execution point
    editor_.OpenScriptFile(script);
    for (auto& pane : editor_.GetPanes())
    {
        auto* se = dynamic_cast<ScriptEditor*>(pane.get());
        if (se && se->GetFilename() == script)
        {
            se->SetExecutionLine(line);
            se->GoToLine(line);

            // If there's a runtime error, show it inline at the error line
            if (dbg.HasError())
            {
                se->SetDebugError(line, dbg.GetLastError());
            }
            break;
        }
    }
}

void BuildPane::ClearExecutionPoints()
{
    last_exec_script_.clear();
    last_exec_line_ = 0;

    for (auto& pane : editor_.GetPanes())
    {
        auto* se = dynamic_cast<ScriptEditor*>(pane.get());
        if (se)
        {
            se->SetExecutionLine(0);
            se->ClearDebugError();
        }
    }
}

} // namespace AGSEditor
