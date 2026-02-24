// AGS Editor ImGui - Settings Pane
#pragma once

#include "ui/editor_ui.h"
#include <climits>

namespace AGSEditor
{

struct GameData;

class SettingsPane : public EditorPane
{
public:
    explicit SettingsPane(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Settings"; }

private:
    void DrawGeneralSection();
    void DrawVisualSection();
    void DrawDialogSection();
    void DrawSoundSection();
    void DrawSpeechSection();
    void DrawCharacterBehaviorSection();
    void DrawInventorySection();
    void DrawSavedGamesSection();
    void DrawCompilerSection();
    void DrawBackwardsCompatSection();

    // Property change handlers
    void HandleGameTitleChange(GameData* gd);
    void HandleColorDepthChange(GameData* gd);
    void HandleResolutionChange(GameData* gd);
    void ResizeAllGUIs(GameData* gd, int old_w, int old_h, int new_w, int new_h);

    EditorUI& editor_;

    // Local copies (applied on save)
    char game_title_[256] = "My Adventure Game";
    int resolution_width_ = 320;
    int resolution_height_ = 200;
    int color_depth_ = 0; // 0 = 32-bit, 1 = 16-bit
    bool debug_mode_ = true;
    bool anti_alias_fonts_ = true;
    int target_fps_ = 40;
    int starting_room_ = 1;
    char description_[512] = "";
    char developer_name_[256] = "";
    char developer_url_[256] = "";
    int max_score_ = 0;
    bool save_screenshots_ = false;
    bool pixel_perfect_speed_ = false;
    bool split_resources_ = false;
    int split_resource_threshold_ = 1024;
    int sprite_cache_size_ = 128;
    int dialog_options_gui_ = -1;
    int dialog_options_gap_ = 0;
    int dialog_bullet_sprite_ = 0;
    bool number_dialog_options_ = false;
    bool run_game_loops_in_dialog_ = false;
    bool play_sound_on_score_ = true;
    int score_sound_clip_ = -1;
    int crossfade_music_ = 0;
    int text_alignment_ = 0;
    bool use_speech_ = false;
    int speech_style_ = 0;
    bool enforce_object_scripting_ = false;
    bool enforce_new_strings_ = true;
    bool left_to_right_precedence_ = true;
    bool enforce_new_audio_ = true;

    // New settings
    int script_api_version_ = INT_MAX;
    int script_compat_level_ = INT_MAX;
    bool use_old_custom_dialog_api_ = false;
    bool use_old_keyboard_handling_ = false;
    int text_window_gui_ = -1;
    int thought_gui_ = -1;
    bool always_display_text_as_speech_ = false;
    bool turn_before_walking_ = true;
    bool turn_before_facing_ = true;
    bool inventory_cursors_ = true;
    bool handle_inv_clicks_in_script_ = true;
    bool display_multiple_inv_ = false;
    char save_game_extension_[32] = "sav";
    char save_game_folder_[256] = "";
    char game_file_name_[256] = "Game";
    int sprite_file_compression_ = 0;
    int default_room_mask_resolution_ = 1;
    int room_transition_ = 0;
    bool render_at_screen_resolution_ = false;
    int when_interface_disabled_ = 0;
    char dialog_say_function_[256] = "";
    char dialog_narrate_function_[256] = "";
    int skip_speech_style_ = 0;
    bool use_global_speech_anim_delay_ = false;
    int global_speech_anim_delay_ = 5;

    // State for property change confirmation popups
    bool show_color_depth_warning_ = false;
    bool show_resize_gui_offer_ = false;
    int pending_old_res_w_ = 0;
    int pending_old_res_h_ = 0;
    int pending_new_res_w_ = 0;
    int pending_new_res_h_ = 0;
};

} // namespace AGSEditor
