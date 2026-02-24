// AGS Editor ImGui - Audio Manager pane
#pragma once

#include "ui/editor_ui.h"
#include <string>

namespace AGSEditor
{

class AudioManager : public EditorPane
{
public:
    explicit AudioManager(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Audio"; }

    // Refresh clip list from project GameData
    void RefreshFromProject();

    // Set initial folder filter (0=All, 1..N=type index+1)
    void SetFolder(int folder) { selected_folder_ = folder; }

    // Set selected clip by index in audio_clips vector
    void SetSelectedClip(int clip_index) { selected_clip_ = clip_index; }

private:
    void DrawAudioTree();
    void DrawAudioProperties();
    void DrawAudioPreview();
    void DrawAudioTypeManager();
    void ReplaceSourceFile(const std::string& new_path);
    void ForceReimportClip(int clip_idx);

    EditorUI& editor_;

    int selected_clip_ = -1;
    int selected_folder_ = 0; // 0=All, 1=Music, 2=Sounds
    bool needs_refresh_ = true;
    bool confirm_delete_ = false;
    bool show_type_manager_ = false;
    int selected_type_ = -1;
    bool confirm_delete_type_ = false;
    int replace_clip_idx_ = -1;
    bool show_replace_paths_ = false;
    char replace_old_path_[256] = {};
    char replace_new_path_[256] = {};
    bool show_folder_tree_ = false;
    const void* selected_clip_folder_ = nullptr;
};

} // namespace AGSEditor
