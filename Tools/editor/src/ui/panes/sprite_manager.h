// AGS Editor ImGui - Sprite Manager pane
// Displays real sprite data from the game's acsprset.spr file.
// Renders sprite textures via SDL/ImGui::Image using the TextureCache.
#pragma once

#include "ui/editor_ui.h"
#include "project/game_data.h"
#include <vector>

namespace AGSEditor
{

// Undo/redo action for sprite operations
struct SpriteUndoAction
{
    enum Type { Delete, Import, Duplicate };
    Type type;
    std::vector<int> sprite_ids;                    // affected sprite IDs
    std::vector<GameData::SpriteInfo> saved_info;    // saved sprite metadata (for delete undo)
};

class SpriteManager : public EditorPane
{
public:
    explicit SpriteManager(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Sprite Manager"; }

    // Force refresh of sprite list from project data
    void RefreshFromProject();

private:
    void DrawToolbar();
    void DrawSpriteGrid();
    void DrawSpriteProperties();
    void DrawContextMenu();
    void DrawFolderTree();

    // Operations
    void ImportSprite();
    void BatchImportSprites();  // Import all images from a folder
    void ExportSprite();
    void DeleteSprite();
    void ReplaceSprite();
    void ReplaceFromSource();
    void DuplicateSprite();
    void AssignToView();
    void ShowUsage();

    // Undo/redo
    void Undo();
    void Redo();
    void PushUndo(SpriteUndoAction action);

    // Multi-select helpers
    bool IsSpriteSelected(int id) const;
    void ToggleSpriteSelection(int id);

    EditorUI& editor_;

    // Multi-select
    std::vector<int> selected_sprites_;   // sprite IDs
    int primary_sprite_ = -1;

    // Display
    float icon_size_ = 64.0f;
    bool show_properties_ = true;
    bool show_folder_tree_ = false;
    const void* selected_folder_ = nullptr; // const GameData::SpriteFolderInfo*, null = show all

    // Context menu
    bool show_context_menu_ = false;

    // State tracking
    bool needs_refresh_ = true;
    int last_sprite_count_ = -1;

    // Filter
    char filter_text_[128] = {};

    // Assign-to-view popup state
    int assign_view_id_ = 0;
    int assign_loop_id_ = 0;

    // Undo/redo stacks
    std::vector<SpriteUndoAction> undo_stack_;
    std::vector<SpriteUndoAction> redo_stack_;
};

} // namespace AGSEditor
