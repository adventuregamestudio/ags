// AGS Editor ImGui - Character Editor pane
// Displays characters from loaded GameData with real view/sprite previews.
#pragma once

#include "ui/editor_ui.h"
#include "project/game_data.h"
#include <string>
#include <vector>

namespace AGSEditor
{

class CharacterEditor : public EditorPane
{
public:
    explicit CharacterEditor(EditorUI& editor, int char_id = -1);

    void Draw() override;
    const char* GetTitle() const override { return title_.c_str(); }
    void FocusEvents() override { focus_events_ = true; }

private:
    void DrawCharacterList();
    void DrawCharacterProperties();
    void DrawViewPreview(const char* label, int view_id, float& timer, int& frame);
    void DrawViewDropdown(const char* label, int& view_id);
    void DrawContextMenu();
    void DeleteCharacter(int index);
    void ChangeCharacterId(int old_index, int new_index);

    // Helper: get mutable characters vector
    std::vector<GameData::CharacterInfo>* GetCharacters();

    EditorUI& editor_;
    std::string title_;

    int selected_char_ = 0;
    bool confirm_delete_ = false;
    bool show_change_id_ = false;
    int change_id_target_ = 0;

    // Animation for previews
    float normal_timer_ = 0.0f;
    int normal_frame_ = 0;
    float speech_timer_ = 0.0f;
    int speech_frame_ = 0;
    float idle_timer_ = 0.0f;
    int idle_frame_ = 0;
    float preview_fps_ = 5.0f;
    bool preview_playing_ = true;
    float preview_zoom_ = 2.0f;
    bool focus_events_ = false;
    bool show_folder_tree_ = false;
    const void* selected_folder_ = nullptr;
};

} // namespace AGSEditor
