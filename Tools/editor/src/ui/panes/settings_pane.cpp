// AGS Editor ImGui - Settings Pane implementation
#include "settings_pane.h"
#include "ui/log_panel.h"
#include "project/project.h"
#include "project/game_data.h"
#include "app.h"
#include "imgui.h"
#include <climits>

namespace AGSEditor
{

SettingsPane::SettingsPane(EditorUI& editor)
    : editor_(editor)
{
    // Load initial values from GameData if a project is loaded
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (gd)
    {
        snprintf(game_title_, sizeof(game_title_), "%s", gd->game_title.c_str());
        resolution_width_ = gd->resolution_width;
        resolution_height_ = gd->resolution_height;
        color_depth_ = (gd->color_depth == 16) ? 1 : 0;
        debug_mode_ = gd->debug_mode;
        target_fps_ = gd->target_fps;
        anti_alias_fonts_ = gd->anti_alias_fonts;
        starting_room_ = gd->starting_room;
        snprintf(description_, sizeof(description_), "%s", gd->description.c_str());
        snprintf(developer_name_, sizeof(developer_name_), "%s", gd->developer_name.c_str());
        snprintf(developer_url_, sizeof(developer_url_), "%s", gd->developer_url.c_str());
        max_score_ = gd->max_score;
        save_screenshots_ = gd->save_screenshots;
        pixel_perfect_speed_ = gd->pixel_perfect_speed;
        split_resources_ = gd->split_resources;
        split_resource_threshold_ = gd->split_resource_threshold;
        sprite_cache_size_ = gd->sprite_cache_size;
        dialog_options_gui_ = gd->dialog_options_gui;
        dialog_options_gap_ = gd->dialog_options_gap;
        dialog_bullet_sprite_ = gd->dialog_bullet_sprite;
        number_dialog_options_ = gd->number_dialog_options;
        run_game_loops_in_dialog_ = gd->run_game_loops_in_dialog;
        play_sound_on_score_ = gd->play_sound_on_score;
        score_sound_clip_ = gd->score_sound_clip;
        crossfade_music_ = gd->crossfade_music;
        text_alignment_ = gd->text_alignment;
        use_speech_ = gd->use_speech;
        speech_style_ = gd->speech_style;
        enforce_object_scripting_ = gd->enforce_object_scripting;
        enforce_new_strings_ = gd->enforce_new_strings;
        left_to_right_precedence_ = gd->left_to_right_precedence;
        enforce_new_audio_ = gd->enforce_new_audio;

        // New settings
        script_api_version_ = gd->script_api_version;
        script_compat_level_ = gd->script_compat_level;
        use_old_custom_dialog_api_ = gd->use_old_custom_dialog_api;
        use_old_keyboard_handling_ = gd->use_old_keyboard_handling;
        text_window_gui_ = gd->text_window_gui;
        thought_gui_ = gd->thought_gui;
        always_display_text_as_speech_ = gd->always_display_text_as_speech;
        turn_before_walking_ = gd->turn_before_walking;
        turn_before_facing_ = gd->turn_before_facing;
        inventory_cursors_ = gd->inventory_cursors;
        handle_inv_clicks_in_script_ = gd->handle_inv_clicks_in_script;
        display_multiple_inv_ = gd->display_multiple_inv;
        snprintf(save_game_extension_, sizeof(save_game_extension_), "%s", gd->save_game_extension.c_str());
        snprintf(save_game_folder_, sizeof(save_game_folder_), "%s", gd->save_game_folder.c_str());
        snprintf(game_file_name_, sizeof(game_file_name_), "%s", gd->game_file_name.c_str());
        sprite_file_compression_ = gd->sprite_file_compression;
        default_room_mask_resolution_ = gd->default_room_mask_resolution;
        room_transition_ = gd->room_transition;
        render_at_screen_resolution_ = gd->render_at_screen_resolution;
        when_interface_disabled_ = gd->when_interface_disabled;
        snprintf(dialog_say_function_, sizeof(dialog_say_function_), "%s", gd->dialog_say_function.c_str());
        snprintf(dialog_narrate_function_, sizeof(dialog_narrate_function_), "%s", gd->dialog_narrate_function.c_str());
        skip_speech_style_ = gd->skip_speech_style;
        use_global_speech_anim_delay_ = gd->use_global_speech_anim_delay;
        global_speech_anim_delay_ = gd->global_speech_anim_delay;
    }
}

void SettingsPane::Draw()
{
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Game Settings");
    ImGui::Separator();
    ImGui::Spacing();

    DrawGeneralSection();
    ImGui::Spacing();
    DrawVisualSection();
    ImGui::Spacing();
    DrawDialogSection();
    ImGui::Spacing();
    DrawSoundSection();
    ImGui::Spacing();
    DrawSpeechSection();
    ImGui::Spacing();
    DrawCharacterBehaviorSection();
    ImGui::Spacing();
    DrawInventorySection();
    ImGui::Spacing();
    DrawSavedGamesSection();
    ImGui::Spacing();
    DrawCompilerSection();
    ImGui::Spacing();
    DrawBackwardsCompatSection();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Apply Settings"))
    {
        auto* project = editor_.GetApp().GetProject();
        auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
        if (gd)
        {
            // Run property change handlers before applying
            HandleGameTitleChange(gd);
            HandleColorDepthChange(gd);
            HandleResolutionChange(gd);

            gd->game_title = game_title_;
            gd->resolution_width = resolution_width_;
            gd->resolution_height = resolution_height_;
            gd->color_depth = (color_depth_ == 1) ? 16 : 32;
            gd->debug_mode = debug_mode_;
            gd->target_fps = target_fps_;
            gd->anti_alias_fonts = anti_alias_fonts_;
            gd->starting_room = starting_room_;
            gd->description = description_;
            gd->developer_name = developer_name_;
            gd->developer_url = developer_url_;
            gd->max_score = max_score_;
            gd->save_screenshots = save_screenshots_;
            gd->pixel_perfect_speed = pixel_perfect_speed_;
            gd->split_resources = split_resources_;
            gd->split_resource_threshold = split_resource_threshold_;
            gd->sprite_cache_size = sprite_cache_size_;
            gd->dialog_options_gui = dialog_options_gui_;
            gd->dialog_options_gap = dialog_options_gap_;
            gd->dialog_bullet_sprite = dialog_bullet_sprite_;
            gd->number_dialog_options = number_dialog_options_;
            gd->run_game_loops_in_dialog = run_game_loops_in_dialog_;
            gd->play_sound_on_score = play_sound_on_score_;
            gd->score_sound_clip = score_sound_clip_;
            gd->crossfade_music = crossfade_music_;
            gd->text_alignment = text_alignment_;
            gd->use_speech = use_speech_;
            gd->speech_style = speech_style_;
            gd->enforce_object_scripting = enforce_object_scripting_;
            gd->enforce_new_strings = enforce_new_strings_;
            gd->left_to_right_precedence = left_to_right_precedence_;
            gd->enforce_new_audio = enforce_new_audio_;

            // New settings
            gd->script_api_version = script_api_version_;
            gd->script_compat_level = script_compat_level_;
            gd->use_old_custom_dialog_api = use_old_custom_dialog_api_;
            gd->use_old_keyboard_handling = use_old_keyboard_handling_;
            gd->text_window_gui = text_window_gui_;
            gd->thought_gui = thought_gui_;
            gd->always_display_text_as_speech = always_display_text_as_speech_;
            gd->turn_before_walking = turn_before_walking_;
            gd->turn_before_facing = turn_before_facing_;
            gd->inventory_cursors = inventory_cursors_;
            gd->handle_inv_clicks_in_script = handle_inv_clicks_in_script_;
            gd->display_multiple_inv = display_multiple_inv_;
            gd->save_game_extension = save_game_extension_;
            gd->save_game_folder = save_game_folder_;
            gd->game_file_name = game_file_name_;
            gd->sprite_file_compression = sprite_file_compression_;
            gd->default_room_mask_resolution = default_room_mask_resolution_;
            gd->room_transition = room_transition_;
            gd->render_at_screen_resolution = render_at_screen_resolution_;
            gd->when_interface_disabled = when_interface_disabled_;
            gd->dialog_say_function = dialog_say_function_;
            gd->dialog_narrate_function = dialog_narrate_function_;
            gd->skip_speech_style = skip_speech_style_;
            gd->use_global_speech_anim_delay = use_global_speech_anim_delay_;
            gd->global_speech_anim_delay = global_speech_anim_delay_;

            project->SetGameTitle(game_title_);
            editor_.GetLogPanel().AddLog("[Info] Settings applied to project.");
        }
        else
        {
            editor_.GetLogPanel().AddLog("[Warning] No project loaded — settings not saved.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset to Defaults"))
    {
        snprintf(game_title_, sizeof(game_title_), "My Adventure Game");
        resolution_width_ = 320;
        resolution_height_ = 200;
        color_depth_ = 0;
        debug_mode_ = true;
        anti_alias_fonts_ = true;
        target_fps_ = 40;
        starting_room_ = 1;
        description_[0] = '\0';
        developer_name_[0] = '\0';
        developer_url_[0] = '\0';
        max_score_ = 0;
        save_screenshots_ = false;
        pixel_perfect_speed_ = false;
        split_resources_ = false;
        split_resource_threshold_ = 1024;
        sprite_cache_size_ = 128;
        dialog_options_gui_ = -1;
        dialog_options_gap_ = 0;
        dialog_bullet_sprite_ = 0;
        number_dialog_options_ = false;
        run_game_loops_in_dialog_ = false;
        play_sound_on_score_ = true;
        score_sound_clip_ = -1;
        crossfade_music_ = 0;
        text_alignment_ = 0;
        use_speech_ = false;
        speech_style_ = 0;
        enforce_object_scripting_ = false;
        enforce_new_strings_ = true;
        left_to_right_precedence_ = true;
        enforce_new_audio_ = true;
        script_api_version_ = INT_MAX;
        script_compat_level_ = INT_MAX;
        use_old_custom_dialog_api_ = false;
        use_old_keyboard_handling_ = false;
        text_window_gui_ = -1;
        thought_gui_ = -1;
        always_display_text_as_speech_ = false;
        turn_before_walking_ = true;
        turn_before_facing_ = true;
        inventory_cursors_ = true;
        handle_inv_clicks_in_script_ = true;
        display_multiple_inv_ = false;
        snprintf(save_game_extension_, sizeof(save_game_extension_), "sav");
        save_game_folder_[0] = '\0';
        snprintf(game_file_name_, sizeof(game_file_name_), "Game");
        sprite_file_compression_ = 0;
        default_room_mask_resolution_ = 1;
        room_transition_ = 0;
        render_at_screen_resolution_ = false;
        when_interface_disabled_ = 0;
        dialog_say_function_[0] = '\0';
        dialog_narrate_function_[0] = '\0';
        skip_speech_style_ = 0;
        use_global_speech_anim_delay_ = false;
        global_speech_anim_delay_ = 5;
        editor_.GetLogPanel().AddLog("[Info] Settings reset to defaults.");
    }

    // Color depth change warning popup
    if (show_color_depth_warning_)
    {
        ImGui::OpenPopup("Color Depth Warning");
        show_color_depth_warning_ = false;
    }
    if (ImGui::BeginPopupModal("Color Depth Warning", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Changing the game color depth can invalidate your\n"
                     "existing sprites and room backgrounds.\n\n"
                     "Are you sure you want to continue?");
        ImGui::Spacing();
        if (ImGui::Button("Yes", ImVec2(120, 0)))
        {
            // Change accepted — already applied
            editor_.GetLogPanel().AddLog("[Info] Color depth changed.");
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(120, 0)))
        {
            // Revert color depth
            auto* project = editor_.GetApp().GetProject();
            auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
            if (gd)
            {
                color_depth_ = (gd->color_depth == 16) ? 1 : 0;
                editor_.GetLogPanel().AddLog("[Info] Color depth change reverted.");
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // GUI resize offer popup
    if (show_resize_gui_offer_)
    {
        ImGui::OpenPopup("Resolution Changed");
        show_resize_gui_offer_ = false;
    }
    if (ImGui::BeginPopupModal("Resolution Changed", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("You've changed the game resolution from %dx%d to %dx%d.\n\n"
                     "You will need to import new room backgrounds of the\n"
                     "correct size for any existing rooms.\n\n"
                     "Would you like AGS to automatically resize all your GUIs\n"
                     "to the new resolution?",
                     pending_old_res_w_, pending_old_res_h_,
                     pending_new_res_w_, pending_new_res_h_);
        ImGui::Spacing();
        if (ImGui::Button("Yes", ImVec2(120, 0)))
        {
            auto* project = editor_.GetApp().GetProject();
            auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
            if (gd)
            {
                ResizeAllGUIs(gd, pending_old_res_w_, pending_old_res_h_,
                              pending_new_res_w_, pending_new_res_h_);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void SettingsPane::DrawGeneralSection()
{
    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::InputText("Game Title", game_title_, sizeof(game_title_));
        ImGui::InputTextMultiline("Description", description_, sizeof(description_),
                                   ImVec2(-1, ImGui::GetTextLineHeight() * 3));
        ImGui::InputText("Developer Name", developer_name_, sizeof(developer_name_));
        ImGui::InputText("Developer URL", developer_url_, sizeof(developer_url_));

        const char* resolutions[] = { "320x200", "320x240", "640x400", "640x480",
                                       "800x600", "1024x768", "1280x720", "1920x1080" };
        static int res_idx = 0;
        if (ImGui::Combo("Resolution Preset", &res_idx, resolutions, IM_ARRAYSIZE(resolutions)))
            sscanf(resolutions[res_idx], "%dx%d", &resolution_width_, &resolution_height_);

        ImGui::InputInt("Width", &resolution_width_);
        ImGui::InputInt("Height", &resolution_height_);

        const char* depths[] = { "32-bit (True Color)", "16-bit (High Color)" };
        ImGui::Combo("Color Depth", &color_depth_, depths, IM_ARRAYSIZE(depths));

        ImGui::InputInt("Max Score", &max_score_);
        ImGui::Checkbox("Save Screenshots in Save Games", &save_screenshots_);
    }
}

void SettingsPane::DrawVisualSection()
{
    if (ImGui::CollapsingHeader("Visual", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Anti-alias Fonts", &anti_alias_fonts_);
        ImGui::Checkbox("Pixel-perfect Character Speed", &pixel_perfect_speed_);
        ImGui::SliderInt("Game Speed (FPS)", &target_fps_, 10, 120);
        ImGui::InputInt("Sprite Cache Size (MB)", &sprite_cache_size_);

        ImGui::Checkbox("Split Resources", &split_resources_);
        if (split_resources_)
        {
            ImGui::InputInt("Split Threshold (KB)", &split_resource_threshold_);
        }

        const char* transition_options[] = { "Cut", "Fade Out/In", "Dissolve", "Box Out",
                                              "Crossfade" };
        ImGui::Combo("Default Room Transition", &room_transition_, transition_options, 5);

        ImGui::Checkbox("Render Sprites at Screen Resolution", &render_at_screen_resolution_);

        const char* iface_disabled_options[] = { "Grey Out Controls", "Hide All GUIs",
                                                  "Show As Normal", "Don't change" };
        ImGui::Combo("When Interface Disabled", &when_interface_disabled_, iface_disabled_options, 4);
    }
}

void SettingsPane::DrawDialogSection()
{
    if (ImGui::CollapsingHeader("Dialog"))
    {
        ImGui::InputInt("Dialog Options GUI (-1 = default)", &dialog_options_gui_);
        ImGui::InputInt("Dialog Options Gap", &dialog_options_gap_);
        ImGui::InputInt("Dialog Bullet Sprite", &dialog_bullet_sprite_);
        ImGui::Checkbox("Number Dialog Options", &number_dialog_options_);
        ImGui::Checkbox("Run Game Loops While Dialog Active", &run_game_loops_in_dialog_);
        ImGui::InputText("Custom Say Function", dialog_say_function_, sizeof(dialog_say_function_));
        ImGui::SameLine(); ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Custom function to call for Say in dialog scripts.\nLeave blank for default.");
        ImGui::InputText("Custom Narrate Function", dialog_narrate_function_, sizeof(dialog_narrate_function_));
    }
}

void SettingsPane::DrawSoundSection()
{
    if (ImGui::CollapsingHeader("Sound"))
    {
        ImGui::Checkbox("Play Sound on Score", &play_sound_on_score_);
        if (play_sound_on_score_)
            ImGui::InputInt("Score Sound Clip ID", &score_sound_clip_);

        const char* crossfade_options[] = { "No", "Auto" };
        ImGui::Combo("Crossfade Music", &crossfade_music_, crossfade_options, 2);
    }
}

void SettingsPane::DrawSpeechSection()
{
    if (ImGui::CollapsingHeader("Text & Speech"))
    {
        const char* align_options[] = { "Left", "Center", "Right" };
        ImGui::Combo("Text Alignment", &text_alignment_, align_options, 3);

        ImGui::Checkbox("Use Speech", &use_speech_);
        if (use_speech_)
        {
            const char* speech_options[] = { "LucasArts", "Sierra", "Sierra (Background)", "Full Screen" };
            ImGui::Combo("Speech Style", &speech_style_, speech_options, 4);
        }

        const char* skip_options[] = { "Any key or mouse", "Any key only", "Mouse only", "Timer only" };
        ImGui::Combo("Skip Speech", &skip_speech_style_, skip_options, 4);

        ImGui::InputInt("Text Window GUI (-1 = default)", &text_window_gui_);
        ImGui::InputInt("Thought GUI (-1 = default)", &thought_gui_);
        ImGui::Checkbox("Always Display Text as Speech", &always_display_text_as_speech_);

        ImGui::Checkbox("Use Global Speech Animation Delay", &use_global_speech_anim_delay_);
        if (use_global_speech_anim_delay_)
            ImGui::InputInt("Speech Animation Delay", &global_speech_anim_delay_);
    }
}

void SettingsPane::DrawCharacterBehaviorSection()
{
    if (ImGui::CollapsingHeader("Character Behavior"))
    {
        ImGui::Checkbox("Characters Turn Before Walking", &turn_before_walking_);
        ImGui::Checkbox("Characters Turn to Face Direction", &turn_before_facing_);
    }
}

void SettingsPane::DrawInventorySection()
{
    if (ImGui::CollapsingHeader("Inventory"))
    {
        ImGui::Checkbox("Use Inventory Item as Cursor", &inventory_cursors_);
        ImGui::Checkbox("Handle Inventory Clicks in Script", &handle_inv_clicks_in_script_);
        ImGui::Checkbox("Display Multiple Icons for Multiple Items", &display_multiple_inv_);
    }
}

void SettingsPane::DrawSavedGamesSection()
{
    if (ImGui::CollapsingHeader("Saved Games"))
    {
        ImGui::InputText("Save File Extension", save_game_extension_, sizeof(save_game_extension_));
        ImGui::InputText("Save Folder Name", save_game_folder_, sizeof(save_game_folder_));
    }
}

void SettingsPane::DrawCompilerSection()
{
    if (ImGui::CollapsingHeader("Compiler", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Debug Mode", &debug_mode_);
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Enable debug mode for development.\nDisable for release builds.");

        ImGui::InputInt("Starting Room", &starting_room_);

        ImGui::InputText("Game File Name", game_file_name_, sizeof(game_file_name_));

        const char* compress_options[] = { "None", "RLE", "LZW", "Deflate" };
        ImGui::Combo("Sprite File Compression", &sprite_file_compression_, compress_options, 4);

        ImGui::InputInt("Default Room Mask Resolution (1:N)", &default_room_mask_resolution_);
        if (default_room_mask_resolution_ < 1) default_room_mask_resolution_ = 1;
    }
}

void SettingsPane::DrawBackwardsCompatSection()
{
    if (ImGui::CollapsingHeader("Backwards Compatibility"))
    {
        ImGui::Checkbox("Enforce object-based scripting", &enforce_object_scripting_);
        ImGui::Checkbox("Enforce new-style strings", &enforce_new_strings_);
        ImGui::Checkbox("Left-to-right operator precedence", &left_to_right_precedence_);
        ImGui::Checkbox("Enforce new-style audio", &enforce_new_audio_);

        // Map between combo indices and actual kScriptAPI_* enum values
        struct APIChoice { const char* display; int value; };
        static const APIChoice choices[] = {
            {"Highest",  INT_MAX},
            {"3.2.1",    0},        // kScriptAPI_v321
            {"3.3.0",    1},        // kScriptAPI_v330
            {"3.3.5",    3},        // kScriptAPI_v335
            {"3.4.0",    4},        // kScriptAPI_v340
            {"3.4.1",    5},        // kScriptAPI_v341
            {"3.5.0",    6},        // kScriptAPI_v350
            {"3.5.1",    8},        // kScriptAPI_v351
            {"3.6.0",    3060000},  // kScriptAPI_v360
            {"3.6.1",    3060100},  // kScriptAPI_v361
        };
        static const int choice_count = IM_ARRAYSIZE(choices);
        const char* choice_names[choice_count];
        for (int i = 0; i < choice_count; i++) choice_names[i] = choices[i].display;

        // Find current combo index from stored value
        int api_combo = 0;
        for (int i = 0; i < choice_count; i++) {
            if (choices[i].value == script_api_version_) { api_combo = i; break; }
        }
        if (ImGui::Combo("Script API Version", &api_combo, choice_names, choice_count))
            script_api_version_ = choices[api_combo].value;

        int compat_combo = 0;
        for (int i = 0; i < choice_count; i++) {
            if (choices[i].value == script_compat_level_) { compat_combo = i; break; }
        }
        if (ImGui::Combo("Script Compat Level", &compat_combo, choice_names, choice_count))
            script_compat_level_ = choices[compat_combo].value;

        ImGui::Checkbox("Use old custom dialog options API", &use_old_custom_dialog_api_);
        ImGui::Checkbox("Use old-style keyboard handling", &use_old_keyboard_handling_);
    }
}

void SettingsPane::HandleGameTitleChange(GameData* gd)
{
    std::string new_title = game_title_;
    std::string old_title = gd->game_title;

    if (new_title != old_title)
    {
        // If save folder was empty or matched the old game title, update it
        std::string cur_folder = save_game_folder_;
        if (cur_folder.empty() || cur_folder == old_title)
        {
            snprintf(save_game_folder_, sizeof(save_game_folder_),
                     "%s", new_title.c_str());
            editor_.GetLogPanel().AddLog(
                "[Info] Save game folder updated to match new game title: %s",
                new_title.c_str());
        }
    }
}

void SettingsPane::HandleColorDepthChange(GameData* gd)
{
    int old_depth = gd->color_depth;
    int new_depth = (color_depth_ == 1) ? 16 : 32;

    if (old_depth != new_depth)
    {
        show_color_depth_warning_ = true;
    }
}

void SettingsPane::HandleResolutionChange(GameData* gd)
{
    int old_w = gd->resolution_width;
    int old_h = gd->resolution_height;
    int new_w = resolution_width_;
    int new_h = resolution_height_;

    if (old_w != new_w || old_h != new_h)
    {
        pending_old_res_w_ = old_w;
        pending_old_res_h_ = old_h;
        pending_new_res_w_ = new_w;
        pending_new_res_h_ = new_h;
        show_resize_gui_offer_ = true;
    }
}

void SettingsPane::ResizeAllGUIs(GameData* gd,
    int old_w, int old_h, int new_w, int new_h)
{
    if (old_w <= 0 || old_h <= 0) return;

    int gui_count = 0;
    for (auto& gui : gd->guis)
    {
        gui.x = gui.x * new_w / old_w;
        gui.y = gui.y * new_h / old_h;
        gui.width = gui.width * new_w / old_w;
        gui.height = gui.height * new_h / old_h;

        for (auto& ctrl : gui.controls)
        {
            ctrl.x = ctrl.x * new_w / old_w;
            ctrl.y = ctrl.y * new_h / old_h;
            ctrl.width = ctrl.width * new_w / old_w;
            ctrl.height = ctrl.height * new_h / old_h;
        }
        gui_count++;
    }

    editor_.GetLogPanel().AddLog(
        "[Info] Resized %d GUIs from %dx%d to %dx%d.",
        gui_count, old_w, old_h, new_w, new_h);
}

} // namespace AGSEditor
