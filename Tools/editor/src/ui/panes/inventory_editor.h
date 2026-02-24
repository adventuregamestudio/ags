// AGS Editor ImGui - Inventory Editor pane
// Displays inventory items from loaded GameData with real sprite rendering.
#pragma once

#include "ui/editor_ui.h"
#include "project/game_data.h"
#include <string>
#include <vector>

namespace AGSEditor
{

class InventoryEditor : public EditorPane
{
public:
    explicit InventoryEditor(EditorUI& editor, int item_id = -1);

    void Draw() override;
    const char* GetTitle() const override { return "Inventory"; }
    void FocusEvents() override { focus_events_ = true; }

private:
    void DrawItemList();
    void DrawItemProperties();
    void DrawItemPreview();

    EditorUI& editor_;
    int selected_item_ = 0;
    bool confirm_delete_ = false;
    float preview_zoom_ = 2.0f;
    bool focus_events_ = false;
    bool show_folder_tree_ = false;
    const void* selected_folder_ = nullptr;
};

} // namespace AGSEditor
