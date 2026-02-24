// AGS Editor ImGui - Palette Editor pane
#pragma once

#include "ui/editor_ui.h"
#include <string>
#include <set>

namespace AGSEditor
{

class PaletteEditor : public EditorPane
{
public:
    explicit PaletteEditor(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Palette"; }

private:
    struct PaletteColor {
        int r, g, b;
        int colour_type;  // 0=Gamewide, 1=Locked, 2=Background
    };

    void DrawPaletteGrid();
    void DrawColorProperties();
    void DrawPaletteTools();
    bool LoadPaletteFromFile(const std::string& path, PaletteColor out[256]);
    void ReplaceSelectedFromFile(const std::string& path);

    bool IsSelected(int idx) const { return selected_colors_.count(idx) > 0; }

    EditorUI& editor_;

    PaletteColor colors_[256];
    int selected_color_ = 0;       // Primary selection (for properties panel)
    std::set<int> selected_colors_; // Multi-selection set
    float grid_cell_size_ = 16.0f; // Scaled by Dpi() at draw time

    // Context menu copy/paste
    PaletteColor copied_color_ = {};
    bool has_copied_color_ = false;
    bool replace_selected_pending_ = false;
};

} // namespace AGSEditor
