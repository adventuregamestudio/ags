// AGS Editor ImGui - Editor Preferences
// Persists editor settings to platform-appropriate config dir/preferences.ini
// Includes theme, font size, layout dimensions, and shortcut overrides.
#pragma once

#include <string>
#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>

namespace AGSEditor
{

// Theme options
enum class EditorTheme
{
    Dark = 0,
    Light,
    Classic,
    Blue,
    Nord,
    Count
};

inline const char* ThemeName(EditorTheme t)
{
    switch (t) {
        case EditorTheme::Dark:    return "Dark";
        case EditorTheme::Light:   return "Light";
        case EditorTheme::Classic: return "Classic";
        case EditorTheme::Blue:    return "Blue";
        case EditorTheme::Nord:    return "Nord";
        default: return "Dark";
    }
}

struct EditorPreferences
{
    // --- Appearance ---
    EditorTheme theme = EditorTheme::Dark;
    float font_size = 16.0f;        // Base font size before DPI scaling
    float dpi_override = 0.0f;      // 0 = auto-detect, >0 = manual override
    bool use_system_font = false;

    // --- Layout ---
    float left_panel_pct = 0.18f;
    float right_panel_pct = 0.22f;
    float bottom_panel_pct = 0.22f;

    // --- Behavior ---
    bool auto_save = false;
    int auto_save_interval_sec = 300;
    bool reopen_last_project = true;
    int max_recent_files = 10;
    bool show_line_numbers = true;
    int tab_size = 4;
    bool use_spaces = true;
    bool word_wrap = false;
    bool auto_indent = true;

    // --- Build ---
    bool auto_build_before_run = true;
    bool show_warnings = true;

    // --- Paths ---
    std::string default_import_path = ".";
    std::string new_game_path = ".";
    std::string paint_program_path;       // external paint program for sprites

    // --- Test Game ---
    int test_game_style = 0;              // 0=windowed, 1=fullscreen, 2=game settings

    // --- Startup ---
    int startup_pane = 0;                 // 0=start page, 1=last project, 2=nothing

    // --- Compile ---
    int message_on_compile = 0;           // 0=always, 1=only errors, 2=never

    // --- Views/Sprites ---
    bool always_show_view_preview = true;
    int sprite_import_transparency = 0;   // 0=auto, 1=top-left pixel, 2=bottom-left pixel, 3=none
    bool remap_palette_backgrounds = false;

    // --- Backup ---
    int backup_warning_interval_min = 30; // 0 = disabled

    // --- UI Behavior ---
    bool keep_help_on_top = false;
    bool prompt_multi_tab_close = true;
    bool script_reload_external = true;

    // --- Script Editor Font ---
    std::string script_font_family = ""; // empty = use default monospace
    float script_font_size = 14.0f;

    // --- Log Panel Font ---
    float log_font_size = 13.0f;

    // --- Android Build ---
    std::string java_home;
    std::string android_home;
    std::string keystore_path;
    std::string keystore_alias;

    // --- Sprite Manager ---
    float sprite_icon_size = 64.0f;
    bool sprite_show_properties = true;

    // --- ImGui ini path ---
    std::string imgui_ini_path;

    // ---- Persistence ----
    static std::string GetPrefsDir()
    {
        std::string dir;
        char* pref_path = SDL_GetPrefPath("AGS", "Editor");
        if (pref_path)
        {
            dir = pref_path;
            // Remove trailing separator
            if (!dir.empty() && (dir.back() == '/' || dir.back() == '\\'))
                dir.pop_back();
            SDL_free(pref_path);
        }
        else
        {
            dir = ".";
        }
        return dir;
    }

    static std::string GetPrefsPath()
    {
        return GetPrefsDir() + "/preferences.ini";
    }

    bool Load()
    {
        FILE* f = fopen(GetPrefsPath().c_str(), "r");
        if (!f) return false;

        char line[512];
        while (fgets(line, sizeof(line), f))
        {
            // Strip newline
            char* nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            char* cr = strchr(line, '\r');
            if (cr) *cr = '\0';

            // Skip comments and empty lines
            if (line[0] == '#' || line[0] == ';' || line[0] == '\0')
                continue;

            char* eq = strchr(line, '=');
            if (!eq) continue;

            std::string key(line, eq);
            std::string val(eq + 1);

            // Trim whitespace
            while (!key.empty() && key.back() == ' ') key.pop_back();
            while (!val.empty() && val.front() == ' ') val.erase(val.begin());

            if (key == "theme")              theme = (EditorTheme)std::atoi(val.c_str());
            else if (key == "font_size")     font_size = (float)std::atof(val.c_str());
            else if (key == "dpi_override")  dpi_override = (float)std::atof(val.c_str());
            else if (key == "left_panel")    left_panel_pct = (float)std::atof(val.c_str());
            else if (key == "right_panel")   right_panel_pct = (float)std::atof(val.c_str());
            else if (key == "bottom_panel")  bottom_panel_pct = (float)std::atof(val.c_str());
            else if (key == "auto_save")     auto_save = (val == "1");
            else if (key == "auto_save_interval") auto_save_interval_sec = std::atoi(val.c_str());
            else if (key == "reopen_last")   reopen_last_project = (val == "1");
            else if (key == "max_recent")    max_recent_files = std::atoi(val.c_str());
            else if (key == "tab_size")      tab_size = std::atoi(val.c_str());
            else if (key == "use_spaces")    use_spaces = (val == "1");
            else if (key == "word_wrap")     word_wrap = (val == "1");
            else if (key == "auto_indent")   auto_indent = (val == "1");
            else if (key == "show_warnings") show_warnings = (val == "1");
            else if (key == "auto_build_before_run") auto_build_before_run = (val == "1");
            else if (key == "default_import_path") default_import_path = val;
            else if (key == "new_game_path") new_game_path = val;
            else if (key == "paint_program_path") paint_program_path = val;
            else if (key == "test_game_style") test_game_style = std::atoi(val.c_str());
            else if (key == "startup_pane") startup_pane = std::atoi(val.c_str());
            else if (key == "message_on_compile") message_on_compile = std::atoi(val.c_str());
            else if (key == "always_show_view_preview") always_show_view_preview = (val == "1");
            else if (key == "sprite_import_transparency") sprite_import_transparency = std::atoi(val.c_str());
            else if (key == "remap_palette_backgrounds") remap_palette_backgrounds = (val == "1");
            else if (key == "backup_warning_interval") backup_warning_interval_min = std::atoi(val.c_str());
            else if (key == "keep_help_on_top") keep_help_on_top = (val == "1");
            else if (key == "prompt_multi_tab_close") prompt_multi_tab_close = (val == "1");
            else if (key == "script_reload_external") script_reload_external = (val == "1");
            else if (key == "script_font_family") script_font_family = val;
            else if (key == "script_font_size") script_font_size = (float)std::atof(val.c_str());
            else if (key == "log_font_size") log_font_size = (float)std::atof(val.c_str());
            else if (key == "java_home") java_home = val;
            else if (key == "android_home") android_home = val;
            else if (key == "keystore_path") keystore_path = val;
            else if (key == "keystore_alias") keystore_alias = val;
            else if (key == "sprite_icon_size") sprite_icon_size = (float)std::atof(val.c_str());
            else if (key == "sprite_show_properties") sprite_show_properties = (val == "1");
        }

        fclose(f);
        return true;
    }

    bool Save() const
    {
        // Ensure directory exists
        std::string dir = GetPrefsDir();
        std::filesystem::create_directories(dir);

        FILE* f = fopen(GetPrefsPath().c_str(), "w");
        if (!f) return false;

        fprintf(f, "# AGS Editor Preferences\n");
        fprintf(f, "theme=%d\n", (int)theme);
        fprintf(f, "font_size=%.1f\n", font_size);
        fprintf(f, "dpi_override=%.2f\n", dpi_override);
        fprintf(f, "left_panel=%.3f\n", left_panel_pct);
        fprintf(f, "right_panel=%.3f\n", right_panel_pct);
        fprintf(f, "bottom_panel=%.3f\n", bottom_panel_pct);
        fprintf(f, "auto_save=%d\n", auto_save ? 1 : 0);
        fprintf(f, "auto_save_interval=%d\n", auto_save_interval_sec);
        fprintf(f, "reopen_last=%d\n", reopen_last_project ? 1 : 0);
        fprintf(f, "max_recent=%d\n", max_recent_files);
        fprintf(f, "tab_size=%d\n", tab_size);
        fprintf(f, "use_spaces=%d\n", use_spaces ? 1 : 0);
        fprintf(f, "word_wrap=%d\n", word_wrap ? 1 : 0);
        fprintf(f, "auto_indent=%d\n", auto_indent ? 1 : 0);
        fprintf(f, "show_warnings=%d\n", show_warnings ? 1 : 0);
        fprintf(f, "auto_build_before_run=%d\n", auto_build_before_run ? 1 : 0);
        fprintf(f, "default_import_path=%s\n", default_import_path.c_str());
        fprintf(f, "new_game_path=%s\n", new_game_path.c_str());
        fprintf(f, "paint_program_path=%s\n", paint_program_path.c_str());
        fprintf(f, "test_game_style=%d\n", test_game_style);
        fprintf(f, "startup_pane=%d\n", startup_pane);
        fprintf(f, "message_on_compile=%d\n", message_on_compile);
        fprintf(f, "always_show_view_preview=%d\n", always_show_view_preview ? 1 : 0);
        fprintf(f, "sprite_import_transparency=%d\n", sprite_import_transparency);
        fprintf(f, "remap_palette_backgrounds=%d\n", remap_palette_backgrounds ? 1 : 0);
        fprintf(f, "backup_warning_interval=%d\n", backup_warning_interval_min);
        fprintf(f, "keep_help_on_top=%d\n", keep_help_on_top ? 1 : 0);
        fprintf(f, "prompt_multi_tab_close=%d\n", prompt_multi_tab_close ? 1 : 0);
        fprintf(f, "script_reload_external=%d\n", script_reload_external ? 1 : 0);
        fprintf(f, "script_font_family=%s\n", script_font_family.c_str());
        fprintf(f, "script_font_size=%.1f\n", script_font_size);
        fprintf(f, "log_font_size=%.1f\n", log_font_size);
        fprintf(f, "java_home=%s\n", java_home.c_str());
        fprintf(f, "android_home=%s\n", android_home.c_str());
        fprintf(f, "keystore_path=%s\n", keystore_path.c_str());
        fprintf(f, "keystore_alias=%s\n", keystore_alias.c_str());
        fprintf(f, "sprite_icon_size=%.1f\n", sprite_icon_size);
        fprintf(f, "sprite_show_properties=%d\n", sprite_show_properties ? 1 : 0);

        fclose(f);
        return true;
    }
};

} // namespace AGSEditor
