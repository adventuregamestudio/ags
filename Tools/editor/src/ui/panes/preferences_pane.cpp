// AGS Editor ImGui - Preferences Dialog Pane implementation
#include "preferences_pane.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "core/shortcut_manager.h"
#include "core/dpi_helper.h"
#include "app.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

namespace AGSEditor
{

PreferencesPane::PreferencesPane(EditorUI& editor)
    : editor_(editor)
{
    // Load current preferences
    prefs_ = editor_.GetApp().GetPreferences();
}

void PreferencesPane::Draw()
{
    // Left side: section list
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float list_width = Dpi(180);

    ImGui::BeginChild("PrefsSections", ImVec2(list_width, avail.y), ImGuiChildFlags_Borders);
    {
        const char* sections[] = {
            "Appearance", "Editor", "Behavior", "Paths", "Android", "Shortcuts", "About"
        };
        for (int i = 0; i < 7; i++)
        {
            bool selected = (active_section_ == i);
            if (ImGui::Selectable(sections[i], selected))
                active_section_ = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right side: section content
    ImGui::BeginChild("PrefsContent", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
    {
        switch (active_section_)
        {
            case 0: DrawAppearanceSection(); break;
            case 1: DrawEditorSection(); break;
            case 2: DrawBehaviorSection(); break;
            case 3: DrawPathsSection(); break;
            case 4: DrawAndroidSection(); break;
            case 5: DrawShortcutsSection(); break;
            case 6: DrawAboutSection(); break;
        }
    }
    ImGui::EndChild();
}

void PreferencesPane::DrawAppearanceSection()
{
    ImGui::Text("Appearance Settings");
    ImGui::Separator();
    ImGui::Spacing();

    // Theme selector
    int theme = (int)prefs_.theme;
    if (ImGui::Combo("Theme", &theme, "Dark\0Light\0Classic\0Blue\0Nord\0"))
    {
        prefs_.theme = (EditorTheme)theme;
        ApplyTheme();
    }

    ImGui::Spacing();

    // Font size
    if (ImGui::SliderFloat("Font Size", &prefs_.font_size, 10.0f, 28.0f, "%.0f px"))
    {
        needs_font_reload_ = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Base font size before DPI scaling.\nRequires restart to take effect.");

    // DPI override
    ImGui::Spacing();
    float dpi = prefs_.dpi_override;
    if (ImGui::SliderFloat("DPI Scale Override", &dpi, 0.0f, 4.0f, dpi < 0.01f ? "Auto" : "%.2f"))
    {
        prefs_.dpi_override = dpi;
        needs_font_reload_ = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("0 = auto-detect from display.\nSet manually if auto-detect is wrong.\nRequires restart to take effect.");

    // Layout proportions
    ImGui::Spacing();
    ImGui::SeparatorText("Layout");
    ImGui::SliderFloat("Left Panel Width", &prefs_.left_panel_pct, 0.10f, 0.35f, "%.0f%%");
    ImGui::SliderFloat("Right Panel Width", &prefs_.right_panel_pct, 0.10f, 0.35f, "%.0f%%");
    ImGui::SliderFloat("Bottom Panel Height", &prefs_.bottom_panel_pct, 0.10f, 0.40f, "%.0f%%");

    ImGui::Spacing();
    ImGui::Spacing();

    if (needs_font_reload_)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::TextWrapped("Font size or DPI changes require restarting the editor to take effect.");
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    // Save / Reset buttons
    if (ImGui::Button("Save Preferences"))
    {
        editor_.GetApp().SetPreferences(prefs_);
        if (prefs_.Save())
            editor_.GetLogPanel().AddLog("[Info] Preferences saved.");
        else
            editor_.GetLogPanel().AddLog("[Error] Failed to save preferences.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset to Defaults"))
    {
        prefs_ = EditorPreferences();
        ApplyTheme();
        editor_.GetLogPanel().AddLog("[Info] Preferences reset to defaults.");
    }
}

void PreferencesPane::DrawEditorSection()
{
    ImGui::Text("Editor Settings");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Show Line Numbers", &prefs_.show_line_numbers);
    ImGui::SliderInt("Tab Size", &prefs_.tab_size, 2, 8);
    ImGui::Checkbox("Use Spaces for Tabs", &prefs_.use_spaces);
    ImGui::Checkbox("Word Wrap", &prefs_.word_wrap);
    ImGui::Checkbox("Auto-Indent", &prefs_.auto_indent);
    ImGui::Checkbox("Reload Scripts on External Change", &prefs_.script_reload_external);

    ImGui::Spacing();
    ImGui::SeparatorText("Script Editor Font");
    ImGui::SetNextItemWidth(Dpi(200));
    ImGui::InputText("Font Family", &prefs_.script_font_family);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Leave empty for default monospace font.\nExamples: Consolas, Source Code Pro, Fira Code");
    ImGui::SetNextItemWidth(Dpi(100));
    ImGui::SliderFloat("Script Font Size", &prefs_.script_font_size, 8.0f, 32.0f, "%.0f px");

    ImGui::Spacing();
    ImGui::SeparatorText("Log Panel Font");
    ImGui::SetNextItemWidth(Dpi(100));
    ImGui::SliderFloat("Log Font Size", &prefs_.log_font_size, 8.0f, 24.0f, "%.0f px");

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Save Preferences"))
    {
        editor_.GetApp().SetPreferences(prefs_);
        prefs_.Save();
        editor_.GetLogPanel().AddLog("[Info] Editor preferences saved.");
    }
}

void PreferencesPane::DrawBehaviorSection()
{
    ImGui::Text("Behavior Settings");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Auto-Save", &prefs_.auto_save);
    if (prefs_.auto_save)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(Dpi(100));
        int interval = prefs_.auto_save_interval_sec;
        if (ImGui::InputInt("Interval (sec)", &interval))
        {
            if (interval < 30) interval = 30;
            if (interval > 3600) interval = 3600;
            prefs_.auto_save_interval_sec = interval;
        }
    }

    ImGui::Checkbox("Reopen Last Project on Startup", &prefs_.reopen_last_project);
    ImGui::SliderInt("Max Recent Files", &prefs_.max_recent_files, 5, 30);

    ImGui::Spacing();
    ImGui::SeparatorText("Startup");
    ImGui::SetNextItemWidth(Dpi(180));
    ImGui::Combo("Startup Pane", &prefs_.startup_pane, "Start Page\0Last Project\0Nothing\0");

    ImGui::Spacing();
    ImGui::SeparatorText("Build");
    ImGui::Checkbox("Auto-Build Before Run", &prefs_.auto_build_before_run);
    ImGui::Checkbox("Show Compiler Warnings", &prefs_.show_warnings);
    ImGui::SetNextItemWidth(Dpi(180));
    ImGui::Combo("Message On Compile", &prefs_.message_on_compile, "Always\0Only Errors\0Never\0");

    ImGui::Spacing();
    ImGui::SeparatorText("Test Game");
    ImGui::SetNextItemWidth(Dpi(180));
    ImGui::Combo("Test Game Window", &prefs_.test_game_style, "Windowed\0Fullscreen\0Use Game Settings\0");

    ImGui::Spacing();
    ImGui::SeparatorText("Views & Sprites");
    ImGui::Checkbox("Always Show View Preview", &prefs_.always_show_view_preview);
    ImGui::SetNextItemWidth(Dpi(180));
    ImGui::Combo("Sprite Import Transparency", &prefs_.sprite_import_transparency,
        "Auto-Detect\0Top-Left Pixel\0Bottom-Left Pixel\0No Transparency\0");
    ImGui::Checkbox("Remap Palettized Backgrounds", &prefs_.remap_palette_backgrounds);

    ImGui::Spacing();
    ImGui::SeparatorText("Backup & UI");
    ImGui::SetNextItemWidth(Dpi(100));
    ImGui::InputInt("Backup Warning (min)", &prefs_.backup_warning_interval_min);
    if (prefs_.backup_warning_interval_min < 0) prefs_.backup_warning_interval_min = 0;
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Show a reminder to save. 0 = disabled.");
    ImGui::Checkbox("Keep Help Window On Top", &prefs_.keep_help_on_top);
    ImGui::Checkbox("Prompt When Closing Multiple Tabs", &prefs_.prompt_multi_tab_close);

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Save Preferences"))
    {
        editor_.GetApp().SetPreferences(prefs_);
        prefs_.Save();
        editor_.GetLogPanel().AddLog("[Info] Behavior preferences saved.");
    }
}

void PreferencesPane::DrawPathsSection()
{
    ImGui::Text("Path Settings");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetNextItemWidth(Dpi(300));
    ImGui::InputText("Default Import Path", &prefs_.default_import_path);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Default directory when importing sprites, sounds, etc.");

    ImGui::SetNextItemWidth(Dpi(300));
    ImGui::InputText("New Game Default Path", &prefs_.new_game_path);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Default directory for creating new game projects.");

    ImGui::Spacing();
    ImGui::SeparatorText("External Programs");

    ImGui::SetNextItemWidth(Dpi(300));
    ImGui::InputText("Paint Program", &prefs_.paint_program_path);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Path to external paint program for editing sprites.\nUsed by 'Edit in Paint Program' feature.");

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Save Preferences"))
    {
        editor_.GetApp().SetPreferences(prefs_);
        prefs_.Save();
        editor_.GetLogPanel().AddLog("[Info] Path preferences saved.");
    }
}

void PreferencesPane::DrawAndroidSection()
{
    ImGui::Text("Android Build Settings");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetNextItemWidth(Dpi(300));
    ImGui::InputText("JAVA_HOME", &prefs_.java_home);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Path to Java Development Kit (JDK).\nExample: /usr/lib/jvm/java-17-openjdk");

    ImGui::SetNextItemWidth(Dpi(300));
    ImGui::InputText("ANDROID_HOME", &prefs_.android_home);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Path to Android SDK.\nExample: ~/Android/Sdk");

    ImGui::Spacing();
    ImGui::SeparatorText("Keystore");

    ImGui::SetNextItemWidth(Dpi(300));
    ImGui::InputText("Keystore Path", &prefs_.keystore_path);

    ImGui::SetNextItemWidth(Dpi(200));
    ImGui::InputText("Key Alias", &prefs_.keystore_alias);

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Save Preferences"))
    {
        editor_.GetApp().SetPreferences(prefs_);
        prefs_.Save();
        editor_.GetLogPanel().AddLog("[Info] Android build preferences saved.");
    }
}

void PreferencesPane::DrawShortcutsSection()
{
    ImGui::Text("Keyboard Shortcuts");
    ImGui::Separator();
    ImGui::Spacing();

    auto* sm = editor_.GetApp().GetShortcutManager();
    if (!sm)
    {
        ImGui::TextDisabled("Shortcut manager not available.");
        return;
    }

    const auto& shortcuts = sm->GetAll();
    if (shortcuts.empty())
    {
        ImGui::TextDisabled("No shortcuts registered.");
        return;
    }

    // Group by category
    std::string last_category;
    if (ImGui::BeginTable("ShortcutsTable", 3,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Shortcut", ImGuiTableColumnFlags_WidthFixed, Dpi(150));
        ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed, Dpi(60));
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < shortcuts.size(); i++)
        {
            const auto& sc = shortcuts[i];
            if (sc.category != last_category)
            {
                last_category = sc.category;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "%s", sc.category.c_str());
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("  %s", sc.id.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", sc.label.c_str());

            ImGui::TableSetColumnIndex(2);
            bool enabled = sc.enabled;
            ImGui::PushID((int)i);
            if (ImGui::Checkbox("##en", &enabled))
            {
                // Toggle in the manager
                sm->SetEnabled(sc.id, enabled);
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

void PreferencesPane::DrawAboutSection()
{
    ImGui::Text("About AGS Editor (ImGui)");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Adventure Game Studio - Cross-Platform Editor");
    ImGui::Text("Version: 3.6.3 (ImGui Edition)");
    ImGui::Spacing();
    ImGui::Text("Built with:");
    ImGui::BulletText("Dear ImGui %s", ImGui::GetVersion());
    ImGui::BulletText("SDL2 (SDL_Renderer backend)");
    ImGui::BulletText("AGS Common & Compiler Libraries");
    ImGui::Spacing();
    ImGui::TextWrapped("AGS is open source under the Artistic License 2.0.");
    ImGui::Spacing();

    ImGui::SeparatorText("System Information");
    ImGui::Text("DPI Scale: %.2f", editor_.GetApp().GetDpiScale());
    ImGui::Text("Display FPS: %.1f", ImGui::GetIO().Framerate);
}

void PreferencesPane::ApplyTheme()
{
    switch (prefs_.theme)
    {
        case EditorTheme::Dark:
            ImGui::StyleColorsDark();
            break;
        case EditorTheme::Light:
            ImGui::StyleColorsLight();
            break;
        case EditorTheme::Classic:
            ImGui::StyleColorsClassic();
            break;
        case EditorTheme::Blue:
        case EditorTheme::Nord:
            ImGui::StyleColorsDark(); // Start from dark as base
            break;
        default:
            break;
    }

    // Re-apply our custom color tweaks for dark theme
    if (prefs_.theme == EditorTheme::Dark)
    {
        ImGuiStyle& style = ImGui::GetStyle();
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
    }

    // Blue theme: professional blue-tinted dark theme
    if (prefs_.theme == EditorTheme::Blue)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* c = style.Colors;
        c[ImGuiCol_WindowBg]        = ImVec4(0.10f, 0.12f, 0.18f, 1.00f);
        c[ImGuiCol_ChildBg]         = ImVec4(0.10f, 0.12f, 0.18f, 1.00f);
        c[ImGuiCol_PopupBg]         = ImVec4(0.12f, 0.14f, 0.20f, 0.95f);
        c[ImGuiCol_Border]          = ImVec4(0.20f, 0.25f, 0.35f, 0.50f);
        c[ImGuiCol_FrameBg]         = ImVec4(0.14f, 0.17f, 0.25f, 1.00f);
        c[ImGuiCol_FrameBgHovered]  = ImVec4(0.18f, 0.22f, 0.33f, 1.00f);
        c[ImGuiCol_FrameBgActive]   = ImVec4(0.22f, 0.28f, 0.42f, 1.00f);
        c[ImGuiCol_TitleBg]         = ImVec4(0.08f, 0.10f, 0.16f, 1.00f);
        c[ImGuiCol_TitleBgActive]   = ImVec4(0.12f, 0.16f, 0.28f, 1.00f);
        c[ImGuiCol_MenuBarBg]       = ImVec4(0.10f, 0.12f, 0.20f, 1.00f);
        c[ImGuiCol_Header]          = ImVec4(0.16f, 0.22f, 0.36f, 0.80f);
        c[ImGuiCol_HeaderHovered]   = ImVec4(0.20f, 0.28f, 0.45f, 0.80f);
        c[ImGuiCol_HeaderActive]    = ImVec4(0.24f, 0.33f, 0.52f, 1.00f);
        c[ImGuiCol_Tab]             = ImVec4(0.12f, 0.16f, 0.26f, 1.00f);
        c[ImGuiCol_TabSelected]     = ImVec4(0.20f, 0.28f, 0.45f, 1.00f);
        c[ImGuiCol_TabHovered]      = ImVec4(0.24f, 0.33f, 0.52f, 1.00f);
        c[ImGuiCol_Button]          = ImVec4(0.16f, 0.22f, 0.36f, 1.00f);
        c[ImGuiCol_ButtonHovered]   = ImVec4(0.22f, 0.30f, 0.48f, 1.00f);
        c[ImGuiCol_ButtonActive]    = ImVec4(0.28f, 0.38f, 0.58f, 1.00f);
        c[ImGuiCol_ScrollbarBg]     = ImVec4(0.08f, 0.10f, 0.16f, 1.00f);
        c[ImGuiCol_ScrollbarGrab]   = ImVec4(0.20f, 0.25f, 0.38f, 1.00f);
        c[ImGuiCol_CheckMark]       = ImVec4(0.40f, 0.55f, 0.85f, 1.00f);
        c[ImGuiCol_SliderGrab]      = ImVec4(0.30f, 0.42f, 0.65f, 1.00f);
        c[ImGuiCol_TextSelectedBg]  = ImVec4(0.20f, 0.30f, 0.50f, 0.50f);
    }

    // Nord theme: based on Nord color palette (polar night, snow storm, frost)
    if (prefs_.theme == EditorTheme::Nord)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* c = style.Colors;
        // Nord Polar Night
        c[ImGuiCol_WindowBg]        = ImVec4(0.18f, 0.20f, 0.25f, 1.00f); // nord0
        c[ImGuiCol_ChildBg]         = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
        c[ImGuiCol_PopupBg]         = ImVec4(0.20f, 0.22f, 0.27f, 0.95f); // nord1
        c[ImGuiCol_Border]          = ImVec4(0.26f, 0.30f, 0.37f, 0.50f); // nord3
        c[ImGuiCol_FrameBg]         = ImVec4(0.22f, 0.25f, 0.30f, 1.00f); // nord1
        c[ImGuiCol_FrameBgHovered]  = ImVec4(0.26f, 0.30f, 0.37f, 1.00f); // nord3
        c[ImGuiCol_FrameBgActive]   = ImVec4(0.30f, 0.34f, 0.42f, 1.00f);
        c[ImGuiCol_TitleBg]         = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
        c[ImGuiCol_TitleBgActive]   = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        c[ImGuiCol_MenuBarBg]       = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        c[ImGuiCol_Header]          = ImVec4(0.26f, 0.30f, 0.37f, 0.80f); // nord3
        c[ImGuiCol_HeaderHovered]   = ImVec4(0.33f, 0.54f, 0.63f, 0.80f); // nord8
        c[ImGuiCol_HeaderActive]    = ImVec4(0.36f, 0.59f, 0.67f, 1.00f); // nord8
        c[ImGuiCol_Tab]             = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        c[ImGuiCol_TabSelected]     = ImVec4(0.33f, 0.54f, 0.63f, 1.00f); // nord8
        c[ImGuiCol_TabHovered]      = ImVec4(0.36f, 0.59f, 0.67f, 1.00f);
        c[ImGuiCol_Button]          = ImVec4(0.33f, 0.54f, 0.63f, 0.80f); // nord8
        c[ImGuiCol_ButtonHovered]   = ImVec4(0.36f, 0.59f, 0.67f, 1.00f);
        c[ImGuiCol_ButtonActive]    = ImVec4(0.40f, 0.64f, 0.73f, 1.00f);
        c[ImGuiCol_ScrollbarBg]     = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
        c[ImGuiCol_ScrollbarGrab]   = ImVec4(0.26f, 0.30f, 0.37f, 1.00f);
        c[ImGuiCol_CheckMark]       = ImVec4(0.53f, 0.75f, 0.56f, 1.00f); // nord14
        c[ImGuiCol_SliderGrab]      = ImVec4(0.33f, 0.54f, 0.63f, 1.00f); // nord8
        c[ImGuiCol_TextSelectedBg]  = ImVec4(0.33f, 0.54f, 0.63f, 0.40f);
        // Nord Snow Storm for text
        c[ImGuiCol_Text]            = ImVec4(0.85f, 0.87f, 0.91f, 1.00f); // nord6
        c[ImGuiCol_TextDisabled]    = ImVec4(0.55f, 0.58f, 0.65f, 1.00f); // nord3-ish
    }
}

} // namespace AGSEditor
