// AGS Editor ImGui - Room Editor pane (Phase 7: Full DrawList-based room editing)
#pragma once

#include "ui/editor_ui.h"
#include "project/room_loader.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <functional>

namespace AGSEditor
{

// Undo/Redo command for room editing
struct RoomUndoCommand
{
    std::string description;
    std::function<void()> undo_fn;
    std::function<void()> redo_fn;
};

// Editing tool modes
enum class RoomTool
{
    Select,     // Select and move objects/hotspots
    Freehand,   // Freehand continuous drawing on masks
    Draw,       // Draw/paint mask areas (rectangle brush)
    Line,       // Draw lines on masks
    Fill,       // Flood-fill mask areas
    Erase,      // Erase mask areas
    PlaceObject // Place new object
};

class RoomEditor : public EditorPane
{
public:
    explicit RoomEditor(EditorUI& editor, int room_number = 1);

    void Draw() override;
    const char* GetTitle() const override { return title_.c_str(); }
    void FocusEvents() override { focus_events_ = true; }
    int GetRoomNumber() const { return room_number_; }

private:
    // Main draw sections
    void DrawToolbar();
    void DrawCanvas();
    void DrawLayerPanel();
    void DrawPropertiesPanel();
    void DrawItemList();

    // Canvas drawing helpers (all use ImDrawList)
    void DrawRoomBackground(ImDrawList* dl, ImVec2 origin);
    void DrawGrid(ImDrawList* dl, ImVec2 origin);
    void DrawEdges(ImDrawList* dl, ImVec2 origin);
    void DrawHotspots(ImDrawList* dl, ImVec2 origin);
    void DrawWalkAreas(ImDrawList* dl, ImVec2 origin);
    void DrawWalkBehinds(ImDrawList* dl, ImVec2 origin);
    void DrawRegions(ImDrawList* dl, ImVec2 origin);
    void DrawObjects(ImDrawList* dl, ImVec2 origin);
    void DrawCharacters(ImDrawList* dl, ImVec2 origin);
    void DrawSelection(ImDrawList* dl, ImVec2 origin);
    void DrawToolPreview(ImDrawList* dl, ImVec2 origin, ImVec2 mouse_room);

    // Interaction
    void HandleCanvasInput(ImVec2 origin, ImVec2 canvas_size);
    ImVec2 ScreenToRoom(ImVec2 screen_pos, ImVec2 origin) const;
    ImVec2 RoomToScreen(ImVec2 room_pos, ImVec2 origin) const;
    ImVec2 RoomToScreen(float rx, float ry, ImVec2 origin) const;

    // Undo/Redo
    void PushUndo(const std::string& desc, std::function<void()> undo_fn, std::function<void()> redo_fn);
    void Undo();
    void Redo();

    // Color helpers
    static ImU32 GetAreaColor(int id, int alpha = 100);
    static ImU32 GetAreaOutlineColor(int id);

    // Mask helpers
    AGS::Common::Bitmap* GetActiveMask() const;
    AGS::Common::Bitmap* GetActiveMaskForLayer(int layer) const;
    void DrawMaskOverlay(ImDrawList* dl, ImVec2 origin, AGS::Common::Bitmap* mask,
                         int palette_offset, int alpha_fill, int alpha_outline, const char* prefix);

    // Load room data from project
    void LoadRoomData();
    void SaveRoomData();

    EditorUI& editor_;
    std::string title_;
    int room_number_;

    // Room data (loaded from RoomLoader)
    std::unique_ptr<RoomData> room_data_;
    bool data_loaded_ = false;
    bool load_attempted_ = false;

    // Layer visibility and editing
    enum Layer {
        Layer_Background = 0,
        Layer_Objects,
        Layer_Hotspots,
        Layer_WalkAreas,
        Layer_WalkBehinds,
        Layer_Regions,
        Layer_Characters,
        Layer_Edges,
        Layer_COUNT
    };

    static constexpr const char* kLayerNames[] = {
        "Background",
        "Objects",
        "Hotspots",
        "Walkable Areas",
        "Walk-behinds",
        "Regions",
        "Characters",
        "Edges"
    };

    int active_layer_ = Layer_Objects;
    bool layer_visible_[Layer_COUNT] = { true, true, true, true, true, true, true, true };

    // Current tool
    RoomTool current_tool_ = RoomTool::Select;
    int brush_size_ = 1;

    // Viewport
    float zoom_ = 1.0f;
    float scroll_x_ = 0.0f;
    float scroll_y_ = 0.0f;
    bool need_auto_fit_ = true;       // Auto-fit to canvas on first draw
    bool focus_events_ = false;        // F4: scroll to events section
    bool show_grid_ = true;
    int grid_size_ = 16;
    float mask_alpha_ = 0.4f;        // Transparency for mask overlays (0..1)
    bool grey_out_other_masks_ = true; // Grey out masks that aren't the active layer

    // Selection
    int selected_item_id_ = -1;      // Selected hotspot/object/region/etc index
    bool dragging_item_ = false;
    ImVec2 drag_start_room_;           // Where drag started in room coords
    ImVec2 drag_item_original_pos_;    // Original item position before drag

    // Edge dragging
    int dragging_edge_ = -1;          // 0=left, 1=right, 2=top, 3=bottom, -1=none
    int edge_drag_original_ = 0;

    // Drawing state
    bool is_drawing_ = false;
    ImVec2 draw_start_room_;
    int selected_bg_frame_ = 0; // Which background frame to display (0..bg_frame_count-1)

    // Sprite chooser state (for double-click on object)
    bool open_sprite_chooser_ = false;

    // Room number change state
    bool open_change_number_popup_ = false;
    int change_number_new_ = 0;

    // Mask resolution change state
    bool show_mask_res_popup_ = false;
    int pending_mask_res_ = 1;

    // Mask undo snapshot (captured before each draw operation)
    std::shared_ptr<AGS::Common::Bitmap> mask_undo_snapshot_;
    int mask_undo_layer_ = -1; // Which layer the snapshot is for

    // Undo stack
    std::vector<RoomUndoCommand> undo_stack_;
    int undo_pos_ = -1; // Points to current position in undo stack
    static const int kMaxUndoSteps = 100;
};

} // namespace AGSEditor
