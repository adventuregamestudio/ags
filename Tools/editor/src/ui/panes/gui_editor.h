// AGS Editor ImGui - GUI Editor pane (Phase 8: Full visual GUI designer)
#pragma once

#include "ui/editor_ui.h"
#include "project/game_data.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <functional>

namespace AGSEditor
{

// Undo/Redo command for GUI editing
struct GUIUndoCommand
{
    std::string description;
    std::function<void()> undo_fn;
    std::function<void()> redo_fn;
};

// GUI control types matching the C# editor
enum class GUIControlType
{
    Button = 0,
    Label,
    TextBox,
    ListBox,
    Slider,
    InvWindow,
    TextWindowEdge,
    COUNT
};

// Editing mode for control placement
enum class GUIEditMode
{
    Select,
    AddButton,
    AddLabel,
    AddTextBox,
    AddListBox,
    AddSlider,
    AddInvWindow
};

struct GUIControl
{
    int id = 0;
    char name[64] = {};
    GUIControlType type = GUIControlType::Button;
    int x = 0, y = 0, width = 60, height = 20;
    int z_order = 0;
    char text[256] = {};
    bool visible = true;
    bool enabled = true;
    bool clickable = true;
    bool locked = false;

    // Appearance
    int font = 0;
    int text_color = 15;       // AGS color index
    int bg_color = 0;
    int border_color = 15;
    bool show_border = true;
    int transparency = 0;      // 0-100%
    int text_align = 0;        // 0=left, 1=center, 2=right

    // Button-specific
    int image = -1;
    int mouseover_image = -1;
    int pushed_image = -1;

    // Slider-specific
    int min_value = 0;
    int max_value = 100;
    int value = 50;
    int handle_image = -1;
    int bg_image = -1;

    // InvWindow-specific
    int item_width = 40;
    int item_height = 22;
    int char_id = -1;          // -1 = player character

    // Events
    Interactions events;

    // Group (0 = no group)
    int group_id = 0;
};

// GUI type
enum class GUIType { Normal, TextWindow };

struct GUIEntry
{
    int id = 0;
    char name[64] = {};
    GUIType type = GUIType::Normal;
    int width = 160, height = 80;
    int x = 0, y = 0;
    bool visible = true;
    bool clickable = true;
    int bg_color = 0;
    int border_color = 15;
    int transparency = 0;    // 0-100
    int z_order = 0;
    int popup_style = 0;     // 0=normal, 1=mouse-y, 2=script-only, 3=persistent
    Interactions events;     // GUI-level events (OnClick)
    std::vector<GUIControl> controls;
};

class GUIEditor : public EditorPane
{
public:
    explicit GUIEditor(EditorUI& editor, int gui_id = -1);

    void Draw() override;
    const char* GetTitle() const override { return title_.c_str(); }
    void FocusEvents() override { focus_events_ = true; }

private:
    // Layout sections
    void DrawGUIList();
    void DrawToolbar();
    void DrawGUICanvas();
    void DrawControlPalette();
    void DrawControlProperties();
    void DrawGUIProperties();
    void DrawContextMenu();

    // Canvas drawing (DrawList-based)
    void DrawGUIBackground(ImDrawList* dl, ImVec2 gui_origin, float zoom);
    void DrawControl(ImDrawList* dl, ImVec2 gui_origin, float zoom, GUIControl& ctrl, int index);
    void DrawSelectionHandles(ImDrawList* dl, ImVec2 gui_origin, float zoom, GUIControl& ctrl);
    void DrawSnapGuides(ImDrawList* dl, ImVec2 gui_origin, float zoom);
    void DrawRubberBand(ImDrawList* dl);
    void DrawAddPreview(ImDrawList* dl, ImVec2 gui_origin, float zoom);

    // Interaction
    void HandleCanvasInput(ImVec2 gui_origin, float zoom, ImVec2 canvas_pos, ImVec2 canvas_size);
    int HitTestControl(ImVec2 gui_origin, float zoom, ImVec2 mouse);
    int HitTestResizeHandle(ImVec2 gui_origin, float zoom, ImVec2 mouse, int ctrl_idx);

    // Operations
    void DeleteSelectedControls();
    void CopySelectedControls();
    void PasteControls(ImVec2 pos);
    void BringToFront();
    void SendToBack();
    void AlignControls(int mode); // 0=left, 1=top, 2=right, 3=bottom
    void DistributeControls(bool horizontal);

    // Import/Export (.guf files — compatible with C# editor)
    void ExportGUI(int gui_idx, const std::string& path);
    void ImportGUI(const std::string& path);

    // Undo/Redo
    void PushUndo(const std::string& desc, std::function<void()> undo_fn, std::function<void()> redo_fn);
    void Undo();
    void Redo();

    // Helpers
    ImVec2 SnapPosition(int ctrl_idx, int x, int y);
    int NextControlId();
    static const char* ControlTypeName(GUIControlType type);
    static ImU32 ControlTypeColor(GUIControlType type);

    EditorUI& editor_;
    std::string title_;

    // Data
    std::vector<GUIEntry> guis_;
    int selected_gui_ = 0;

    // Selection (multi-select)
    std::vector<int> selected_controls_;  // indices into current GUI's controls
    int primary_selection_ = -1;          // primary selected control index

    // Canvas state
    float canvas_zoom_ = 2.0f;
    GUIEditMode edit_mode_ = GUIEditMode::Select;
    bool snap_enabled_ = true;
    static constexpr float kSnapThreshold = 5.0f;

    // Drag state
    bool dragging_ = false;
    bool resizing_ = false;
    int resize_side_ = -1;  // 0-7: TL,T,TR,R,BR,B,BL,L
    ImVec2 drag_start_;
    ImVec2 drag_offset_;
    std::vector<ImVec2> drag_original_pos_;
    ImVec2 drag_original_size_;

    // Rubber-band selection
    bool rubber_banding_ = false;
    ImVec2 rubber_start_;
    ImVec2 rubber_end_;

    // Add-control drawing
    bool adding_control_ = false;
    ImVec2 add_start_;
    ImVec2 add_end_;

    // Snap guide lines to draw
    struct SnapGuide { ImVec2 from, to; };
    std::vector<SnapGuide> snap_guides_;

    // Clipboard
    std::vector<GUIControl> clipboard_;

    // Context menu
    bool show_context_menu_ = false;

    // Multi-line text editing (Ctrl+E)
    bool show_text_edit_popup_ = false;
    char text_edit_buf_[256] = {};

    // Change GUI ID
    bool show_change_gui_id_ = false;
    int change_gui_id_target_ = 0;

    // Deletion confirmation
    bool confirm_delete_gui_ = false;

    // Focus events section
    bool focus_events_ = false;

    // Next group ID
    int next_group_id_ = 1;

    // Undo/Redo stack
    std::vector<GUIUndoCommand> undo_stack_;
    int undo_pos_ = -1;
    static const int kMaxUndoSteps = 50;
    bool show_folder_tree_ = false;
    const void* selected_folder_ = nullptr;
};

} // namespace AGSEditor
