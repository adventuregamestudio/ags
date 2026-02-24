// AGS Editor ImGui - View/Animation Editor pane
// Displays views from loaded GameData with real sprite rendering.
// All loops shown stacked vertically with multi-loop frame selection via
// Shift+click (range) and Ctrl+click (toggle). Copy/cut/paste/flip/reverse
// operate on the full cross-loop selection.
#pragma once

#include "ui/editor_ui.h"
#include "project/game_data.h"
#include "view/view_renderer.h"
#include <string>
#include <vector>

namespace AGSEditor
{

class ViewEditor : public EditorPane
{
public:
    explicit ViewEditor(EditorUI& editor, int view_id = -1);

    void Draw() override;
    const char* GetTitle() const override { return title_.c_str(); }

    // A reference to a single frame: (loop index, frame index)
    struct FrameRef {
        int loop = -1;
        int frame = -1;
        bool operator==(const FrameRef& o) const { return loop == o.loop && frame == o.frame; }
        bool operator!=(const FrameRef& o) const { return !(*this == o); }
        bool IsValid() const { return loop >= 0 && frame >= 0; }
    };

private:
    void DrawViewList();
    void DrawLoopEditor();
    void DrawLoopSection(int loop_idx);
    void DrawFrameStrip(int loop_idx);
    void DrawFrameProperties();
    void DrawAnimationPreview();
    void DrawSelectedFrameSprite();
    void DrawFrameToolbar();
    void DrawContextMenu();

    // Selection helpers
    bool IsFrameSelected(int loop, int frame) const;
    void SelectRange(const FrameRef& from, const FrameRef& to);
    void ClearSelection();

    // Frame operations (work across loops)
    void CopySelectedFrames();
    void CutSelectedFrames();
    void PasteFrames();
    void DeleteSelectedFrames();
    void FlipSelectedFrames();
    void ReverseSelectedFrames();
    void ShiftFramesLeft();
    void ShiftFramesRight();
    void AutoAssignSprites(int start_sprite, int loop_idx);

    // Loop operations
    void CopyLoop(int loop_idx);
    void PasteLoop(int loop_idx);
    void DeleteLoop(int loop_idx);

    // Helper: get mutable views vector, or nullptr
    std::vector<GameData::ViewInfo>* GetViews();

    EditorUI& editor_;
    std::string title_;

    int selected_view_ = 0;

    // Multi-select frames across loops — (loop, frame) pairs
    std::vector<FrameRef> selected_frames_;
    FrameRef primary_frame_;  // last clicked frame

    // Frame clipboard (stores frames without loop context)
    std::vector<GameData::FrameData> frame_clipboard_;
    // Loop clipboard
    GameData::LoopData loop_clipboard_;
    bool has_loop_clipboard_ = false;

    // Animation playback
    bool playing_ = false;
    float anim_timer_ = 0.0f;
    int anim_frame_ = 0;
    int anim_loop_ = 0;
    float frame_rate_ = 5.0f;
    float preview_zoom_ = 2.0f;
    bool preview_loop_ = true;

    // Context menu
    bool show_context_menu_ = false;
    int context_menu_loop_ = -1;

    // Frame thumbnail size
    float thumb_size_ = 56.0f;

    // Undo/Redo
    struct ViewUndoCommand {
        std::string description;
        int view_idx;
        std::vector<GameData::LoopData> loops_snapshot;
    };
    std::vector<ViewUndoCommand> undo_stack_;
    int undo_pos_ = -1;
    static const int kMaxUndoSteps = 50;

    void PushUndoSnapshot(const std::string& desc);
    void Undo();
    void Redo();

    // Deletion confirmation
    bool pending_delete_ = false;
    std::string delete_usage_info_;
    bool show_folder_tree_ = false;
    const void* selected_folder_ = nullptr;

    // Native view renderer for composited loop strip rendering
    ViewRenderer view_renderer_;
    bool use_native_rendering_ = true;
    bool palette_set_ = false;
    void EnsurePaletteSet();
    void DrawNativeFrameStrip(int loop_idx);
    // Ring buffer for native strip textures (destroyed after N frames)
    static const int kStripTexRingSize = 16;
    SDL_Texture* strip_tex_ring_[kStripTexRingSize] = {};
    int strip_tex_ring_idx_ = 0;
};

} // namespace AGSEditor
