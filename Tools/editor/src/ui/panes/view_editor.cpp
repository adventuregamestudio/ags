// AGS Editor ImGui - View/Animation Editor
// All loops displayed stacked vertically with multi-loop frame selection.
// Uses real view/loop/frame data from GameData and renders sprites via TextureCache.
#include "view_editor.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "ui/project_panel.h"
#include "ui/folder_tree_widget.h"
#include "core/dpi_helper.h"
#include "project/project.h"
#include "project/texture_cache.h"
#include "project/game_data.h"
#include "core/script_name_validator.h"
#include "app.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include <SDL.h>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <map>

namespace AGSEditor
{

ViewEditor::ViewEditor(EditorUI& editor, int view_id)
    : editor_(editor)
    , title_("Views")
{
    if (view_id >= 0)
        selected_view_ = view_id;
    // Initialize native view renderer with the SDL renderer
    view_renderer_.SetRenderer(editor_.GetApp().GetRenderer());
}

std::vector<GameData::ViewInfo>* ViewEditor::GetViews()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded()) return nullptr;
    auto* gd = project->GetGameData();
    if (!gd) return nullptr;
    return &gd->views;
}

// ============================================================================
// Selection helpers
// ============================================================================

bool ViewEditor::IsFrameSelected(int loop, int frame) const
{
    FrameRef ref{loop, frame};
    for (const auto& s : selected_frames_)
        if (s == ref) return true;
    return false;
}

void ViewEditor::ClearSelection()
{
    selected_frames_.clear();
    primary_frame_ = FrameRef{};
}

void ViewEditor::SelectRange(const FrameRef& from, const FrameRef& to)
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];

    int min_loop, max_loop, min_frame_in_first, max_frame_in_last;

    if (from.loop < to.loop || (from.loop == to.loop && from.frame <= to.frame))
    {
        min_loop = from.loop;
        max_loop = to.loop;
        min_frame_in_first = from.frame;
        max_frame_in_last = to.frame;
    }
    else
    {
        min_loop = to.loop;
        max_loop = from.loop;
        min_frame_in_first = to.frame;
        max_frame_in_last = from.frame;
    }

    selected_frames_.clear();

    if (min_loop == max_loop)
    {
        // Same loop: simple range
        if (min_loop < (int)view.loops.size())
        {
            int lo = std::min(min_frame_in_first, max_frame_in_last);
            int hi = std::max(min_frame_in_first, max_frame_in_last);
            for (int f = lo; f <= hi && f < (int)view.loops[min_loop].frames.size(); f++)
                selected_frames_.push_back({min_loop, f});
        }
    }
    else
    {
        // Cross-loop range selection:
        // 1. From min_frame_in_first to end of first loop
        if (min_loop < (int)view.loops.size())
        {
            for (int f = min_frame_in_first; f < (int)view.loops[min_loop].frames.size(); f++)
                selected_frames_.push_back({min_loop, f});
        }
        // 2. All frames in intermediate loops
        for (int l = min_loop + 1; l < max_loop && l < (int)view.loops.size(); l++)
        {
            for (int f = 0; f < (int)view.loops[l].frames.size(); f++)
                selected_frames_.push_back({l, f});
        }
        // 3. From start of last loop to max_frame_in_last
        if (max_loop < (int)view.loops.size())
        {
            for (int f = 0; f <= max_frame_in_last && f < (int)view.loops[max_loop].frames.size(); f++)
                selected_frames_.push_back({max_loop, f});
        }
    }
}

// ============================================================================
// Main Draw
// ============================================================================

void ViewEditor::Draw()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
    {
        ImGui::TextDisabled("No project loaded. Open a game to view animations.");
        return;
    }

    auto* views = GetViews();
    if (!views)
    {
        ImGui::TextDisabled("No game data available.");
        return;
    }

    float list_width = Dpi(180);
    float tree_width = show_folder_tree_ ? Dpi(160) : 0.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Folder tree panel
    if (show_folder_tree_)
    {
        ImGui::BeginChild("##ViewFolders", ImVec2(tree_width, avail.y), ImGuiChildFlags_Borders);
        auto* gd = project->GetGameData();
        if (gd)
        {
            const auto* sel = static_cast<const FolderInfo*>(selected_folder_);
            const auto* new_sel = DrawFolderTreeWidget(sel, gd->view_folders, "All Views", &gd->view_folders);
            if (new_sel != sel)
                selected_folder_ = new_sel;
        }
        ImGui::EndChild();
        ImGui::SameLine();
    }

    ImGui::BeginChild("##ViewList", ImVec2(list_width, avail.y), ImGuiChildFlags_Borders);
    DrawViewList();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##ViewContent", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
    if (!views->empty() && selected_view_ >= 0 && selected_view_ < (int)views->size())
    {
        DrawLoopEditor();
        ImGui::Separator();
        DrawAnimationPreview();
        ImGui::Separator();
        DrawSelectedFrameSprite();
    }
    else
    {
        ImGui::TextDisabled("No view selected.");
    }
    ImGui::EndChild();

    // Undo/Redo keyboard shortcuts
    auto& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift)
        Undo();
    if (io.KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))))
        Redo();
}

// ============================================================================
// View List
// ============================================================================

void ViewEditor::DrawViewList()
{
    auto* views = GetViews();
    if (!views) return;

    if (ImGui::Button("Add"))
    {
        GameData::ViewInfo v;
        int max_id = 0;
        for (auto& ev : *views) { if (ev.id > max_id) max_id = ev.id; }
        v.id = max_id + 1;
        v.name = "VIEW" + std::to_string(v.id);
        v.loop_count = 1;
        GameData::LoopData loop;
        GameData::FrameData f{};
        loop.frames.push_back(f);
        v.loops.push_back(loop);
        views->push_back(v);
        selected_view_ = (int)views->size() - 1;
        ClearSelection();
        editor_.GetLogPanel().AddLog("[View] Added view '%s'.", v.name.c_str());
        editor_.GetProjectPanel().MarkTreeDirty();
    }
    ImGui::SameLine();
    if (ImGui::Button("Del") && !views->empty())
    {
        delete_usage_info_.clear();
        auto* project = editor_.GetApp().GetProject();
        if (project && project->GetGameData())
        {
            auto* gd = project->GetGameData();
            int view_id = (*views)[selected_view_].id;
            for (const auto& ch : gd->characters)
            {
                if (ch.normal_view == view_id || ch.speech_view == view_id ||
                    ch.idle_view == view_id || ch.thinking_view == view_id ||
                    ch.blinking_view == view_id)
                {
                    delete_usage_info_ += "  Character: " + ch.script_name + "\n";
                }
            }
            for (size_t ci = 0; ci < gd->cursors.size(); ci++)
            {
                if (gd->cursors[ci].view == view_id)
                    delete_usage_info_ += "  Cursor: " + gd->cursors[ci].name + "\n";
            }
        }
        ImGui::OpenPopup("Confirm Delete View");
        pending_delete_ = true;
    }

    // Delete confirmation modal
    if (ImGui::BeginPopupModal("Confirm Delete View", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Delete view '%s'?", (*views)[selected_view_].name.c_str());
        if (!delete_usage_info_.empty())
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "Warning: This view is referenced by:");
            ImGui::TextUnformatted(delete_usage_info_.c_str());
        }
        ImGui::Separator();
        if (ImGui::Button("Delete", ImVec2(Dpi(120), 0)))
        {
            editor_.GetLogPanel().AddLog("[View] Deleted view '%s'.", (*views)[selected_view_].name.c_str());
            views->erase(views->begin() + selected_view_);
            if (selected_view_ >= (int)views->size())
                selected_view_ = std::max(0, (int)views->size() - 1);
            ClearSelection();
            pending_delete_ = false;
            editor_.GetProjectPanel().MarkTreeDirty();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
        {
            pending_delete_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Dup") && !views->empty())
    {
        GameData::ViewInfo copy = (*views)[selected_view_];
        int max_id = 0;
        for (auto& ev : *views) { if (ev.id > max_id) max_id = ev.id; }
        copy.id = max_id + 1;
        copy.name += "_copy";
        views->push_back(copy);
        selected_view_ = (int)views->size() - 1;
        editor_.GetLogPanel().AddLog("[View] Duplicated view to '%s'.", copy.name.c_str());
        editor_.GetProjectPanel().MarkTreeDirty();
    }

    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%d views", (int)views->size());
    ImGui::SameLine();
    ImGui::Checkbox("Folders", &show_folder_tree_);
    ImGui::Separator();

    // Build folder filter set
    std::set<int> folder_ids;
    if (selected_folder_)
        CollectFolderItemIds(*static_cast<const FolderInfo*>(selected_folder_), folder_ids);

    for (int i = 0; i < (int)views->size(); i++)
    {
        auto& view = (*views)[i];
        // Filter by folder
        if (selected_folder_ && folder_ids.find(view.id) == folder_ids.end())
            continue;
        char label[128];
        std::snprintf(label, sizeof(label), "%d: %s (%d loops)",
                      view.id, view.name.c_str(), (int)view.loops.size());
        if (ImGui::Selectable(label, selected_view_ == i))
        {
            selected_view_ = i;
            ClearSelection();
        }
        BeginItemDragSource(view.id, label);
    }
}

// ============================================================================
// Loop Editor — all loops stacked vertically (like C# editor)
// ============================================================================

void ViewEditor::DrawLoopEditor()
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];

    // View header
    ImGui::Text("View: %s (ID %d)", view.name.c_str(), view.id);
    ImGui::SetNextItemWidth(Dpi(200));
    if (ImGui::InputText("##ViewName", &view.name))
        editor_.GetProjectPanel().MarkTreeDirty();
    {
        auto* gd = editor_.GetApp().GetProject()->GetGameData();
        if (gd)
        {
            std::string err = ValidateScriptName(*gd, view.name, "View", view.id);
            if (!err.empty())
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", err.c_str());
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Add Loop"))
    {
        GameData::LoopData loop;
        GameData::FrameData f{};
        loop.frames.push_back(f);
        view.loops.push_back(loop);
        view.loop_count = (int)view.loops.size();
        editor_.GetProjectPanel().MarkTreeDirty();
    }

    // Global frame toolbar — operates on multi-loop selection
    DrawFrameToolbar();

    ImGui::Separator();

    // Thumb size slider
    ImGui::SetNextItemWidth(Dpi(60));
    ImGui::SliderFloat("Frame Size", &thumb_size_, 32.0f, 96.0f, "%.0f");
    ImGui::SameLine();
    ImGui::Checkbox("Native Render", &use_native_rendering_);

    ImGui::Separator();

    // Draw all loops stacked vertically in a scrollable area
    for (int l = 0; l < (int)view.loops.size(); l++)
    {
        ImGui::PushID(l);
        DrawLoopSection(l);
        ImGui::PopID();
        if (l < (int)view.loops.size() - 1)
            ImGui::Separator();
    }

    // Frame properties for primary selection (any loop)
    if (primary_frame_.IsValid())
    {
        ImGui::Separator();
        DrawFrameProperties();
    }

    DrawContextMenu();
}

// ============================================================================
// Single Loop Section — header + frame strip for one loop
// ============================================================================

void ViewEditor::DrawLoopSection(int loop_idx)
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];
    if (loop_idx < 0 || loop_idx >= (int)view.loops.size()) return;
    auto& loop = view.loops[loop_idx];

    const char* dir_names[] = { "Down", "Left", "Right", "Up",
                                "Down-Left", "Down-Right", "Up-Left", "Up-Right" };

    // Loop header
    if (loop_idx < 8)
        ImGui::Text("Loop %d (%s) - %d frames", loop_idx, dir_names[loop_idx], (int)loop.frames.size());
    else
        ImGui::Text("Loop %d - %d frames", loop_idx, (int)loop.frames.size());

    ImGui::SameLine();
    ImGui::Checkbox("Run Next##RN", &loop.run_next_loop);

    // Per-loop buttons
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Frame"))
    {
        GameData::FrameData f{};
        loop.frames.push_back(f);
        ClearSelection();
        selected_frames_.push_back({loop_idx, (int)loop.frames.size() - 1});
        primary_frame_ = {loop_idx, (int)loop.frames.size() - 1};
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Copy Loop"))
        CopyLoop(loop_idx);
    ImGui::SameLine();
    if (ImGui::SmallButton("Paste Loop") && has_loop_clipboard_)
        PasteLoop(loop_idx);
    ImGui::SameLine();
    if (ImGui::SmallButton("Del Loop") && view.loops.size() > 1)
        DeleteLoop(loop_idx);

    // Auto-assign sprites (per-loop)
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    static int auto_start = 0;
    ImGui::SetNextItemWidth(Dpi(50));
    ImGui::InputInt("##AutoS", &auto_start, 0, 0);
    ImGui::SameLine();
    if (ImGui::SmallButton("Auto-Assign"))
        AutoAssignSprites(auto_start, loop_idx);

    // Draw the frame strip
    DrawFrameStrip(loop_idx);
}

// ============================================================================
// Global frame toolbar — copy/cut/paste/flip/reverse/shift across all loops
// ============================================================================

void ViewEditor::DrawFrameToolbar()
{
    bool has_sel = !selected_frames_.empty();
    bool multi = selected_frames_.size() > 1;

    if (has_sel)
    {
        // Count how many loops are represented in selection
        int min_loop = selected_frames_[0].loop, max_loop = selected_frames_[0].loop;
        for (const auto& r : selected_frames_)
        {
            if (r.loop < min_loop) min_loop = r.loop;
            if (r.loop > max_loop) max_loop = r.loop;
        }
        bool cross_loop = (min_loop != max_loop);

        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "%d frame(s) selected%s",
            (int)selected_frames_.size(), cross_loop ? " (multi-loop)" : "");
        ImGui::SameLine();
    }

    if (ImGui::Button("Copy##F") && has_sel) CopySelectedFrames();
    ImGui::SameLine();
    if (ImGui::Button("Cut##F") && has_sel) CutSelectedFrames();
    ImGui::SameLine();
    if (ImGui::Button("Paste##F") && !frame_clipboard_.empty()) PasteFrames();
    ImGui::SameLine();
    if (ImGui::Button("Delete##F") && has_sel) DeleteSelectedFrames();
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    if (ImGui::Button("Flip") && has_sel) FlipSelectedFrames();
    ImGui::SameLine();
    if (ImGui::Button("Reverse") && multi) ReverseSelectedFrames();
    ImGui::SameLine();
    if (ImGui::Button("<") && has_sel) ShiftFramesLeft();
    ImGui::SameLine();
    if (ImGui::Button(">") && has_sel) ShiftFramesRight();
}

// ============================================================================
// Frame Strip — renders real sprite textures from TextureCache for one loop
// ============================================================================

void ViewEditor::DrawFrameStrip(int loop_idx)
{
    // Use native composited rendering if enabled
    if (use_native_rendering_)
    {
        DrawNativeFrameStrip(loop_idx);
        return;
    }

    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];
    if (loop_idx < 0 || loop_idx >= (int)view.loops.size()) return;
    auto& loop = view.loops[loop_idx];

    auto* project = editor_.GetApp().GetProject();
    auto* loader = project ? project->GetSpriteLoader() : nullptr;
    auto& tex_cache = editor_.GetApp().GetTextureCache();

    ImVec2 start_pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float pad = Dpi(4);
    float total_w = (thumb_size_ + pad) * loop.frames.size() + pad;
    float strip_h = thumb_size_ + Dpi(22);

    // Strip background
    dl->AddRectFilled(start_pos,
        ImVec2(start_pos.x + std::max(total_w, ImGui::GetContentRegionAvail().x), start_pos.y + strip_h),
        IM_COL32(35, 35, 45, 255));

    for (int f = 0; f < (int)loop.frames.size(); f++)
    {
        auto& frame = loop.frames[f];
        float fx = start_pos.x + pad + f * (thumb_size_ + pad);
        float fy = start_pos.y + pad;

        bool is_selected = IsFrameSelected(loop_idx, f);
        bool is_primary = (primary_frame_.loop == loop_idx && primary_frame_.frame == f);

        // Background
        ImU32 bg = is_selected
            ? (is_primary ? IM_COL32(80, 120, 200, 255) : IM_COL32(60, 90, 160, 255))
            : IM_COL32(55, 55, 75, 255);
        dl->AddRectFilled(ImVec2(fx, fy), ImVec2(fx + thumb_size_, fy + thumb_size_), bg);

        // Try to render real sprite texture
        int spr = frame.sprite_id;
        SDL_Texture* tex = nullptr;
        if (loader && spr >= 0)
            tex = tex_cache.GetSpriteTexture(spr, loader);

        if (tex)
        {
            const auto* info = tex_cache.FindSprite(spr);
            float sw = info ? (float)info->width : thumb_size_;
            float sh = info ? (float)info->height : thumb_size_;
            float ins = Dpi(2);
            float avail_w = thumb_size_ - ins * 2;
            float avail_h = thumb_size_ - ins * 2;
            float scale = std::min(avail_w / sw, avail_h / sh);
            float dw = sw * scale;
            float dh = sh * scale;
            float ox = (thumb_size_ - dw) * 0.5f;
            float oy = (thumb_size_ - dh) * 0.5f;

            if (frame.flipped)
            {
                dl->AddImage((ImTextureID)(intptr_t)tex,
                    ImVec2(fx + ox + dw, fy + oy),
                    ImVec2(fx + ox, fy + oy + dh),
                    ImVec2(0, 0), ImVec2(1, 1));
            }
            else
            {
                dl->AddImage((ImTextureID)(intptr_t)tex,
                    ImVec2(fx + ox, fy + oy),
                    ImVec2(fx + ox + dw, fy + oy + dh));
            }
        }
        else
        {
            // Fallback: colored rect + sprite ID
            ImU32 spr_col = IM_COL32(100 + (spr * 17) % 155, 80 + (spr * 31) % 175, 120 + (spr * 7) % 120, 255);
            float ins = Dpi(4);
            dl->AddRectFilled(ImVec2(fx + ins, fy + ins),
                              ImVec2(fx + thumb_size_ - ins, fy + thumb_size_ - ins), spr_col);

            char sp_txt[16];
            std::snprintf(sp_txt, sizeof(sp_txt), "S%d", spr);
            ImVec2 ts = ImGui::CalcTextSize(sp_txt);
            dl->AddText(ImVec2(fx + (thumb_size_ - ts.x) * 0.5f, fy + (thumb_size_ - ts.y) * 0.5f),
                        IM_COL32(255, 255, 255, 230), sp_txt);
        }

        // Flip indicator
        if (frame.flipped)
            dl->AddText(ImVec2(fx + 2, fy + 2), IM_COL32(255, 200, 0, 255), "F");

        // Sound indicator
        if (frame.sound >= 0)
        {
            float sx = fx + thumb_size_ - Dpi(14);
            dl->AddText(ImVec2(sx, fy + 2), IM_COL32(100, 255, 100, 255), "S");
        }

        // Border
        ImU32 border_col = is_selected ? IM_COL32(255, 255, 0, 255) : IM_COL32(120, 120, 120, 200);
        dl->AddRect(ImVec2(fx, fy), ImVec2(fx + thumb_size_, fy + thumb_size_),
                    border_col, 0, 0, is_selected ? 2.0f : 1.0f);

        // Frame number label below
        char frame_lbl[16];
        std::snprintf(frame_lbl, sizeof(frame_lbl), "%d", f);
        ImVec2 lts = ImGui::CalcTextSize(frame_lbl);
        dl->AddText(ImVec2(fx + (thumb_size_ - lts.x) * 0.5f, fy + thumb_size_ + 2),
                    IM_COL32(180, 180, 180, 200), frame_lbl);

        // Delay indicator
        if (frame.delay > 0)
        {
            char delay_txt[8];
            std::snprintf(delay_txt, sizeof(delay_txt), "+%d", frame.delay);
            ImVec2 dts = ImGui::CalcTextSize(delay_txt);
            dl->AddText(ImVec2(fx + thumb_size_ - dts.x - 2, fy + thumb_size_ - dts.y - 2),
                        IM_COL32(255, 180, 80, 200), delay_txt);
        }
    }

    // Invisible interaction region
    char strip_id[32];
    std::snprintf(strip_id, sizeof(strip_id), "##frame_strip_%d", loop_idx);
    ImGui::SetCursorScreenPos(start_pos);
    ImGui::InvisibleButton(strip_id, ImVec2(std::max(total_w, ImGui::GetContentRegionAvail().x), strip_h));

    // Handle click selection (with cross-loop Shift+click)
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        ImVec2 mouse = ImGui::GetMousePos();
        int clicked_frame = -1;
        for (int f = 0; f < (int)loop.frames.size(); f++)
        {
            float fx = start_pos.x + pad + f * (thumb_size_ + pad);
            float fy = start_pos.y + pad;
            if (mouse.x >= fx && mouse.x <= fx + thumb_size_ && mouse.y >= fy && mouse.y <= fy + thumb_size_)
            {
                clicked_frame = f;
                break;
            }
        }

        if (clicked_frame >= 0)
        {
            bool ctrl = ImGui::GetIO().KeyCtrl;
            bool shift = ImGui::GetIO().KeyShift;
            FrameRef clicked{loop_idx, clicked_frame};

            if (shift && primary_frame_.IsValid())
            {
                // Range select across loops
                SelectRange(primary_frame_, clicked);
                // primary_frame_ stays where it was (anchor point)
            }
            else if (ctrl)
            {
                // Toggle individual frame
                auto it = std::find(selected_frames_.begin(), selected_frames_.end(), clicked);
                if (it != selected_frames_.end())
                    selected_frames_.erase(it);
                else
                    selected_frames_.push_back(clicked);
                primary_frame_ = clicked;
            }
            else
            {
                // Single select
                selected_frames_ = { clicked };
                primary_frame_ = clicked;
            }
        }
        else
        {
            if (!ImGui::GetIO().KeyCtrl)
                ClearSelection();
        }
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        show_context_menu_ = true;
        context_menu_loop_ = loop_idx;
    }
}

// ============================================================================
// Frame Properties — shows primary frame info, applies changes to all selected
// ============================================================================

void ViewEditor::DrawFrameProperties()
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];
    if (!primary_frame_.IsValid()) return;
    if (primary_frame_.loop < 0 || primary_frame_.loop >= (int)view.loops.size()) return;
    auto& ploop = view.loops[primary_frame_.loop];
    if (primary_frame_.frame < 0 || primary_frame_.frame >= (int)ploop.frames.size()) return;
    auto& fr = ploop.frames[primary_frame_.frame];

    bool multi = (selected_frames_.size() > 1);
    if (multi)
        ImGui::Text("Frame L%d:F%d properties (%d selected):",
            primary_frame_.loop, primary_frame_.frame, (int)selected_frames_.size());
    else
        ImGui::Text("Frame L%d:F%d:", primary_frame_.loop, primary_frame_.frame);

    ImGui::SetNextItemWidth(Dpi(100));
    if (ImGui::InputInt("Sprite", &fr.sprite_id))
    {
        if (multi)
        {
            for (const auto& sel : selected_frames_)
            {
                if (sel == primary_frame_) continue;
                if (sel.loop >= 0 && sel.loop < (int)view.loops.size())
                {
                    auto& sl = view.loops[sel.loop];
                    if (sel.frame >= 0 && sel.frame < (int)sl.frames.size())
                        sl.frames[sel.frame].sprite_id = fr.sprite_id;
                }
            }
        }
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(Dpi(60));
    if (ImGui::InputInt("Delay", &fr.delay))
    {
        if (multi)
        {
            for (const auto& sel : selected_frames_)
            {
                if (sel == primary_frame_) continue;
                if (sel.loop >= 0 && sel.loop < (int)view.loops.size())
                {
                    auto& sl = view.loops[sel.loop];
                    if (sel.frame >= 0 && sel.frame < (int)sl.frames.size())
                        sl.frames[sel.frame].delay = fr.delay;
                }
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Flipped", &fr.flipped))
    {
        if (multi)
        {
            for (const auto& sel : selected_frames_)
            {
                if (sel == primary_frame_) continue;
                if (sel.loop >= 0 && sel.loop < (int)view.loops.size())
                {
                    auto& sl = view.loops[sel.loop];
                    if (sel.frame >= 0 && sel.frame < (int)sl.frames.size())
                        sl.frames[sel.frame].flipped = fr.flipped;
                }
            }
        }
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(Dpi(80));
    if (ImGui::InputInt("Sound", &fr.sound))
    {
        if (multi)
        {
            for (const auto& sel : selected_frames_)
            {
                if (sel == primary_frame_) continue;
                if (sel.loop >= 0 && sel.loop < (int)view.loops.size())
                {
                    auto& sl = view.loops[sel.loop];
                    if (sel.frame >= 0 && sel.frame < (int)sl.frames.size())
                        sl.frames[sel.frame].sound = fr.sound;
                }
            }
        }
    }
}

// ============================================================================
// Animation Preview — renders real sprite from TextureCache
// ============================================================================

void ViewEditor::DrawAnimationPreview()
{
    if (ImGui::CollapsingHeader("Animation Preview", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto* views = GetViews();
        if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
        auto& view = (*views)[selected_view_];
        if (view.loops.empty()) return;

        // Pick the loop to preview: use the primary frame's loop, or loop 0
        int preview_loop = (primary_frame_.IsValid() && primary_frame_.loop < (int)view.loops.size())
            ? primary_frame_.loop : 0;
        if (anim_loop_ < 0 || anim_loop_ >= (int)view.loops.size())
            anim_loop_ = preview_loop;

        auto* project = editor_.GetApp().GetProject();
        auto* loader = project ? project->GetSpriteLoader() : nullptr;
        auto& tex_cache = editor_.GetApp().GetTextureCache();

        // Loop selector for preview
        ImGui::SetNextItemWidth(Dpi(80));
        if (ImGui::InputInt("Preview Loop", &anim_loop_, 1, 1))
        {
            if (anim_loop_ < 0) anim_loop_ = 0;
            if (anim_loop_ >= (int)view.loops.size()) anim_loop_ = (int)view.loops.size() - 1;
            anim_frame_ = 0;
        }

        auto& active_loop = view.loops[anim_loop_];
        if (active_loop.frames.empty())
        {
            ImGui::TextDisabled("No frames in preview loop %d.", anim_loop_);
            return;
        }

        // Controls
        ImGui::SameLine();
        if (ImGui::Button(playing_ ? "Stop" : "Play"))
        {
            playing_ = !playing_;
            if (playing_) { anim_frame_ = 0; anim_timer_ = 0.0f; }
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(Dpi(100));
        ImGui::SliderFloat("FPS", &frame_rate_, 1.0f, 30.0f);
        ImGui::SameLine();
        ImGui::Checkbox("Loop", &preview_loop_);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(Dpi(80));
        ImGui::SliderFloat("Zoom##Prev", &preview_zoom_, 1.0f, 6.0f, "%.0fx");
        ImGui::SameLine();
        ImGui::Text("Frame: %d / %d", anim_frame_ + 1, (int)active_loop.frames.size());

        // Advance animation
        if (playing_)
        {
            if (anim_frame_ >= (int)active_loop.frames.size())
                anim_frame_ = 0;
            float dt = ImGui::GetIO().DeltaTime;
            anim_timer_ += dt;
            float interval = 1.0f / frame_rate_;
            float frame_delay = active_loop.frames[anim_frame_].delay * interval * 0.5f;
            if (anim_timer_ >= interval + frame_delay)
            {
                anim_timer_ = 0.0f;
                anim_frame_++;
                if (anim_frame_ >= (int)active_loop.frames.size())
                {
                    anim_frame_ = preview_loop_ ? 0 : (int)active_loop.frames.size() - 1;
                    if (!preview_loop_) playing_ = false;
                }
            }
        }
        if (anim_frame_ >= (int)active_loop.frames.size())
            anim_frame_ = 0;

        // Preview canvas
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        float base_w = Dpi(48), base_h = Dpi(64);
        ImVec2 canvas_size(base_w * preview_zoom_ + Dpi(40), base_h * preview_zoom_ + Dpi(40));
        ImDrawList* dl = ImGui::GetWindowDrawList();

        dl->AddRectFilled(canvas_pos,
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(25, 25, 40, 255));

        float cx = canvas_pos.x + canvas_size.x * 0.5f;
        float cy = canvas_pos.y + canvas_size.y * 0.5f;
        int spr = active_loop.frames[anim_frame_].sprite_id;
        bool flipped = active_loop.frames[anim_frame_].flipped;

        SDL_Texture* tex = nullptr;
        if (loader && spr >= 0)
            tex = tex_cache.GetSpriteTexture(spr, loader);

        if (tex)
        {
            const auto* info = tex_cache.FindSprite(spr);
            float sw = info ? (float)info->width : 32.0f;
            float sh = info ? (float)info->height : 32.0f;
            float dw = sw * preview_zoom_;
            float dh = sh * preview_zoom_;

            if (flipped)
            {
                dl->AddImage((ImTextureID)(intptr_t)tex,
                    ImVec2(cx + dw / 2, cy - dh / 2),
                    ImVec2(cx - dw / 2, cy + dh / 2),
                    ImVec2(0, 0), ImVec2(1, 1));
            }
            else
            {
                dl->AddImage((ImTextureID)(intptr_t)tex,
                    ImVec2(cx - dw / 2, cy - dh / 2),
                    ImVec2(cx + dw / 2, cy + dh / 2));
            }
        }
        else
        {
            float sw = base_w * preview_zoom_ * 0.5f;
            float sh = base_h * preview_zoom_ * 0.5f;
            ImU32 col = IM_COL32(100 + (spr * 17) % 155, 80 + (spr * 31) % 175, 120 + (spr * 7) % 120, 255);
            dl->AddRectFilled(ImVec2(cx - sw, cy - sh), ImVec2(cx + sw, cy + sh), col);

            char buf[32];
            std::snprintf(buf, sizeof(buf), "Spr %d%s", spr, flipped ? " [F]" : "");
            ImVec2 ts = ImGui::CalcTextSize(buf);
            dl->AddText(ImVec2(cx - ts.x / 2, cy - ts.y / 2), IM_COL32(255, 255, 255, 255), buf);
        }

        if (flipped)
        {
            float sw = base_w * preview_zoom_ * 0.5f;
            float sh = base_h * preview_zoom_ * 0.5f;
            dl->AddTriangleFilled(
                ImVec2(cx - sw + 4, cy - sh + 4),
                ImVec2(cx - sw + 14, cy - sh + 8),
                ImVec2(cx - sw + 4, cy - sh + 12),
                IM_COL32(255, 200, 0, 200));
        }

        dl->AddRect(canvas_pos,
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(80, 80, 80, 255));

        ImGui::Dummy(canvas_size);

        ImGui::SetCursorScreenPos(canvas_pos);
        ImGui::InvisibleButton("##preview_click", canvas_size);
        if (ImGui::IsItemClicked() && !playing_)
        {
            anim_frame_ = (anim_frame_ + 1) % (int)active_loop.frames.size();
        }
    }
}

// ============================================================================
// Selected Frame Sprite — large view of the currently selected frame's sprite
// ============================================================================

void ViewEditor::DrawSelectedFrameSprite()
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];
    if (!primary_frame_.IsValid()) return;
    if (primary_frame_.loop < 0 || primary_frame_.loop >= (int)view.loops.size()) return;
    auto& loop = view.loops[primary_frame_.loop];
    if (primary_frame_.frame < 0 || primary_frame_.frame >= (int)loop.frames.size()) return;

    if (!ImGui::CollapsingHeader("Selected Frame Sprite", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    auto& fr = loop.frames[primary_frame_.frame];
    int spr = fr.sprite_id;

    auto* project = editor_.GetApp().GetProject();
    auto* loader = project ? project->GetSpriteLoader() : nullptr;
    auto& tex_cache = editor_.GetApp().GetTextureCache();

    SDL_Texture* tex = nullptr;
    if (loader && spr >= 0)
        tex = tex_cache.GetSpriteTexture(spr, loader);

    const auto* info = tex_cache.FindSprite(spr);
    if (info)
    {
        ImGui::Text("Sprite %d: %dx%d (Loop %d, Frame %d)",
            spr, info->width, info->height, primary_frame_.loop, primary_frame_.frame);
    }
    else
    {
        ImGui::Text("Sprite %d (Loop %d, Frame %d)", spr, primary_frame_.loop, primary_frame_.frame);
    }
    if (fr.flipped)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "(Flipped)");
    }
    if (fr.delay > 0)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("Delay: +%d", fr.delay);
    }
    if (fr.sound >= 0)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("Sound: %d", fr.sound);
    }

    // Draw the sprite at a larger size
    static float sprite_zoom = 2.0f;
    ImGui::SetNextItemWidth(Dpi(80));
    ImGui::SliderFloat("Zoom##Sprite", &sprite_zoom, 1.0f, 8.0f, "%.0fx");

    if (tex && info)
    {
        float sw = (float)info->width * sprite_zoom;
        float sh = (float)info->height * sprite_zoom;

        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size(sw + Dpi(8), sh + Dpi(8));
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Checkerboard transparency background
        float check_size = Dpi(8);
        for (float cy = canvas_pos.y; cy < canvas_pos.y + canvas_size.y; cy += check_size)
        {
            for (float cx = canvas_pos.x; cx < canvas_pos.x + canvas_size.x; cx += check_size)
            {
                int ix = (int)((cx - canvas_pos.x) / check_size);
                int iy = (int)((cy - canvas_pos.y) / check_size);
                ImU32 col = ((ix + iy) % 2 == 0) ? IM_COL32(200, 200, 200, 255) : IM_COL32(160, 160, 160, 255);
                float w = std::min(check_size, canvas_pos.x + canvas_size.x - cx);
                float h = std::min(check_size, canvas_pos.y + canvas_size.y - cy);
                dl->AddRectFilled(ImVec2(cx, cy), ImVec2(cx + w, cy + h), col);
            }
        }

        float ox = canvas_pos.x + Dpi(4);
        float oy = canvas_pos.y + Dpi(4);

        if (fr.flipped)
        {
            dl->AddImage((ImTextureID)(intptr_t)tex,
                ImVec2(ox + sw, oy), ImVec2(ox, oy + sh),
                ImVec2(0, 0), ImVec2(1, 1));
        }
        else
        {
            dl->AddImage((ImTextureID)(intptr_t)tex,
                ImVec2(ox, oy), ImVec2(ox + sw, oy + sh));
        }

        dl->AddRect(canvas_pos,
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(80, 80, 80, 255));

        ImGui::Dummy(canvas_size);
    }
    else
    {
        ImGui::TextDisabled("No sprite texture available for sprite %d.", spr);
    }
}

// ============================================================================
// Context Menu
// ============================================================================

void ViewEditor::DrawContextMenu()
{
    if (show_context_menu_)
    {
        ImGui::OpenPopup("##FrameCtxMenu");
        show_context_menu_ = false;
    }

    if (ImGui::BeginPopup("##FrameCtxMenu"))
    {
        bool has_sel = !selected_frames_.empty();
        if (ImGui::MenuItem("Copy", "Ctrl+C", false, has_sel)) CopySelectedFrames();
        if (ImGui::MenuItem("Cut", "Ctrl+X", false, has_sel)) CutSelectedFrames();
        if (ImGui::MenuItem("Paste", "Ctrl+V", false, !frame_clipboard_.empty())) PasteFrames();
        if (ImGui::MenuItem("Delete", "Del", false, has_sel)) DeleteSelectedFrames();
        ImGui::Separator();
        if (ImGui::MenuItem("Flip", nullptr, false, has_sel)) FlipSelectedFrames();
        if (ImGui::MenuItem("Reverse Order", nullptr, false, selected_frames_.size() > 1)) ReverseSelectedFrames();
        if (ImGui::MenuItem("Shift Left", nullptr, false, has_sel)) ShiftFramesLeft();
        if (ImGui::MenuItem("Shift Right", nullptr, false, has_sel)) ShiftFramesRight();
        ImGui::EndPopup();
    }
}

// ============================================================================
// Frame Operations — all work with (loop, frame) selection spanning loops
// ============================================================================

void ViewEditor::CopySelectedFrames()
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];
    frame_clipboard_.clear();

    // Sort by loop then frame for a consistent copy order
    std::vector<FrameRef> sorted = selected_frames_;
    std::sort(sorted.begin(), sorted.end(), [](const FrameRef& a, const FrameRef& b) {
        return a.loop < b.loop || (a.loop == b.loop && a.frame < b.frame);
    });

    for (const auto& ref : sorted)
    {
        if (ref.loop >= 0 && ref.loop < (int)view.loops.size())
        {
            auto& loop = view.loops[ref.loop];
            if (ref.frame >= 0 && ref.frame < (int)loop.frames.size())
                frame_clipboard_.push_back(loop.frames[ref.frame]);
        }
    }
}

void ViewEditor::CutSelectedFrames()
{
    CopySelectedFrames();
    PushUndoSnapshot("Cut frames");
    DeleteSelectedFrames();
}

void ViewEditor::PasteFrames()
{
    if (frame_clipboard_.empty()) return;
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    PushUndoSnapshot("Paste frames");
    auto& view = (*views)[selected_view_];

    // Paste into the primary frame's loop, or loop 0
    int target_loop = primary_frame_.IsValid() ? primary_frame_.loop : 0;
    if (target_loop < 0 || target_loop >= (int)view.loops.size()) target_loop = 0;
    auto& loop = view.loops[target_loop];

    int insert_pos = primary_frame_.IsValid() && primary_frame_.loop == target_loop
        ? primary_frame_.frame + 1
        : (int)loop.frames.size();

    loop.frames.insert(loop.frames.begin() + insert_pos,
                       frame_clipboard_.begin(), frame_clipboard_.end());

    ClearSelection();
    for (int i = 0; i < (int)frame_clipboard_.size(); i++)
        selected_frames_.push_back({target_loop, insert_pos + i});
    primary_frame_ = {target_loop, insert_pos};
}

void ViewEditor::DeleteSelectedFrames()
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    PushUndoSnapshot("Delete frames");
    auto& view = (*views)[selected_view_];

    // Group selected frames by loop and delete in reverse order within each loop
    std::map<int, std::vector<int>> per_loop;
    for (const auto& ref : selected_frames_)
        per_loop[ref.loop].push_back(ref.frame);

    for (auto& [loop_idx, frames] : per_loop)
    {
        if (loop_idx < 0 || loop_idx >= (int)view.loops.size()) continue;
        auto& loop = view.loops[loop_idx];
        std::sort(frames.rbegin(), frames.rend());
        for (int idx : frames)
            if (idx >= 0 && idx < (int)loop.frames.size())
                loop.frames.erase(loop.frames.begin() + idx);
    }
    ClearSelection();
}

void ViewEditor::FlipSelectedFrames()
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    PushUndoSnapshot("Flip frames");
    auto& view = (*views)[selected_view_];
    for (const auto& ref : selected_frames_)
    {
        if (ref.loop >= 0 && ref.loop < (int)view.loops.size())
        {
            auto& loop = view.loops[ref.loop];
            if (ref.frame >= 0 && ref.frame < (int)loop.frames.size())
                loop.frames[ref.frame].flipped = !loop.frames[ref.frame].flipped;
        }
    }
}

void ViewEditor::ReverseSelectedFrames()
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    PushUndoSnapshot("Reverse frames");
    auto& view = (*views)[selected_view_];

    // Reverse within each loop separately
    std::map<int, std::vector<int>> per_loop;
    for (const auto& ref : selected_frames_)
        per_loop[ref.loop].push_back(ref.frame);

    for (auto& [loop_idx, frames] : per_loop)
    {
        if (loop_idx < 0 || loop_idx >= (int)view.loops.size()) continue;
        if (frames.size() < 2) continue;
        auto& loop = view.loops[loop_idx];
        std::sort(frames.begin(), frames.end());
        int lo = 0, hi = (int)frames.size() - 1;
        while (lo < hi)
        {
            if (frames[lo] < (int)loop.frames.size() && frames[hi] < (int)loop.frames.size())
                std::swap(loop.frames[frames[lo]], loop.frames[frames[hi]]);
            lo++; hi--;
        }
    }
}

void ViewEditor::ShiftFramesLeft()
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];

    // Per-loop shift
    std::map<int, std::vector<int>> per_loop;
    for (const auto& ref : selected_frames_)
        per_loop[ref.loop].push_back(ref.frame);

    // Check: can all selected frames shift left?
    for (auto& [loop_idx, frames] : per_loop)
    {
        std::sort(frames.begin(), frames.end());
        if (!frames.empty() && frames[0] <= 0) return;
    }

    PushUndoSnapshot("Shift frames left");
    for (auto& [loop_idx, frames] : per_loop)
    {
        if (loop_idx < 0 || loop_idx >= (int)view.loops.size()) continue;
        auto& loop = view.loops[loop_idx];
        std::sort(frames.begin(), frames.end());
        for (int idx : frames)
        {
            if (idx > 0 && idx < (int)loop.frames.size())
                std::swap(loop.frames[idx], loop.frames[idx - 1]);
        }
    }
    // Update selected references
    for (auto& ref : selected_frames_)
        ref.frame--;
    if (primary_frame_.IsValid())
        primary_frame_.frame--;
}

void ViewEditor::ShiftFramesRight()
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];

    // Per-loop shift
    std::map<int, std::vector<int>> per_loop;
    for (const auto& ref : selected_frames_)
        per_loop[ref.loop].push_back(ref.frame);

    // Check: can all selected frames shift right?
    for (auto& [loop_idx, frames] : per_loop)
    {
        if (loop_idx < 0 || loop_idx >= (int)view.loops.size()) continue;
        auto& loop = view.loops[loop_idx];
        std::sort(frames.rbegin(), frames.rend());
        if (!frames.empty() && frames[0] >= (int)loop.frames.size() - 1) return;
    }

    PushUndoSnapshot("Shift frames right");
    for (auto& [loop_idx, frames] : per_loop)
    {
        if (loop_idx < 0 || loop_idx >= (int)view.loops.size()) continue;
        auto& loop = view.loops[loop_idx];
        std::sort(frames.rbegin(), frames.rend());
        for (int idx : frames)
        {
            if (idx >= 0 && idx < (int)loop.frames.size() - 1)
                std::swap(loop.frames[idx], loop.frames[idx + 1]);
        }
    }
    for (auto& ref : selected_frames_)
        ref.frame++;
    if (primary_frame_.IsValid())
        primary_frame_.frame++;
}

void ViewEditor::AutoAssignSprites(int start_sprite, int loop_idx)
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];
    if (loop_idx < 0 || loop_idx >= (int)view.loops.size()) return;
    PushUndoSnapshot("Auto-assign sprites");
    auto& loop = view.loops[loop_idx];

    // If there are selected frames in this loop, auto-assign only those
    std::vector<int> frames_in_loop;
    for (const auto& ref : selected_frames_)
        if (ref.loop == loop_idx)
            frames_in_loop.push_back(ref.frame);

    if (frames_in_loop.empty())
    {
        // Assign to all frames
        for (int i = 0; i < (int)loop.frames.size(); i++)
            loop.frames[i].sprite_id = start_sprite + i;
    }
    else
    {
        std::sort(frames_in_loop.begin(), frames_in_loop.end());
        for (int i = 0; i < (int)frames_in_loop.size(); i++)
            if (frames_in_loop[i] < (int)loop.frames.size())
                loop.frames[frames_in_loop[i]].sprite_id = start_sprite + i;
    }
}

// ============================================================================
// Loop Operations
// ============================================================================

void ViewEditor::CopyLoop(int loop_idx)
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];
    if (loop_idx < 0 || loop_idx >= (int)view.loops.size()) return;
    loop_clipboard_ = view.loops[loop_idx];
    has_loop_clipboard_ = true;
}

void ViewEditor::PasteLoop(int loop_idx)
{
    if (!has_loop_clipboard_) return;
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    PushUndoSnapshot("Paste loop");
    auto& view = (*views)[selected_view_];
    view.loops.insert(view.loops.begin() + loop_idx + 1, loop_clipboard_);
    view.loop_count = (int)view.loops.size();
    ClearSelection();
    editor_.GetProjectPanel().MarkTreeDirty();
}

void ViewEditor::DeleteLoop(int loop_idx)
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];
    if (view.loops.size() <= 1) return;
    if (loop_idx < 0 || loop_idx >= (int)view.loops.size()) return;
    PushUndoSnapshot("Delete loop");
    view.loops.erase(view.loops.begin() + loop_idx);
    view.loop_count = (int)view.loops.size();
    ClearSelection();
    editor_.GetProjectPanel().MarkTreeDirty();
}

// ============================================================================
// Undo / Redo
// ============================================================================

void ViewEditor::PushUndoSnapshot(const std::string& desc)
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;

    if (undo_pos_ < (int)undo_stack_.size() - 1)
        undo_stack_.resize(undo_pos_ + 1);

    ViewUndoCommand cmd;
    cmd.description = desc;
    cmd.view_idx = selected_view_;
    cmd.loops_snapshot = (*views)[selected_view_].loops;
    undo_stack_.push_back(std::move(cmd));
    undo_pos_ = (int)undo_stack_.size() - 1;

    while ((int)undo_stack_.size() > kMaxUndoSteps)
    {
        undo_stack_.erase(undo_stack_.begin());
        undo_pos_--;
    }
}

void ViewEditor::Undo()
{
    if (undo_pos_ < 0 || undo_pos_ >= (int)undo_stack_.size()) return;
    auto* views = GetViews();
    if (!views) return;

    auto& cmd = undo_stack_[undo_pos_];
    if (cmd.view_idx >= 0 && cmd.view_idx < (int)views->size())
    {
        auto& view = (*views)[cmd.view_idx];
        auto current_loops = view.loops;
        view.loops = cmd.loops_snapshot;
        cmd.loops_snapshot = current_loops;
        view.loop_count = (int)view.loops.size();

        selected_view_ = cmd.view_idx;
        ClearSelection();
    }
    undo_pos_--;
}

void ViewEditor::Redo()
{
    if (undo_pos_ >= (int)undo_stack_.size() - 1) return;
    undo_pos_++;
    auto* views = GetViews();
    if (!views) return;

    auto& cmd = undo_stack_[undo_pos_];
    if (cmd.view_idx >= 0 && cmd.view_idx < (int)views->size())
    {
        auto& view = (*views)[cmd.view_idx];
        auto current_loops = view.loops;
        view.loops = cmd.loops_snapshot;
        cmd.loops_snapshot = current_loops;
        view.loop_count = (int)view.loops.size();

        selected_view_ = cmd.view_idx;
        ClearSelection();
    }
}

// ============================================================================
// Palette setup for native rendering
// ============================================================================

void ViewEditor::EnsurePaletteSet()
{
    if (palette_set_) return;

    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded()) return;
    auto* gd = project->GetGameData();
    if (!gd || gd->palette.empty()) return;

    // Convert GameData palette (r,g,b per entry) to ARGB32 array
    uint32_t pal[256];
    int count = std::min((int)gd->palette.size(), 256);
    for (int i = 0; i < count; i++)
    {
        auto& pe = gd->palette[i];
        pal[i] = 0xFF000000
            | (((uint32_t)pe.r & 0xFF) << 16)
            | (((uint32_t)pe.g & 0xFF) <<  8)
            | (((uint32_t)pe.b & 0xFF) <<  0);
    }
    for (int i = count; i < 256; i++)
        pal[i] = 0xFF000000;

    view_renderer_.SetPalette(pal, 256);

    // Also set palette on the texture cache for correct 8-bit sprite rendering
    editor_.GetApp().GetTextureCache().SetPalette(pal, 256);

    palette_set_ = true;
}

// ============================================================================
// Native Frame Strip — composited using ViewRenderer (like NativeProxy)
// ============================================================================

void ViewEditor::DrawNativeFrameStrip(int loop_idx)
{
    auto* views = GetViews();
    if (!views || selected_view_ < 0 || selected_view_ >= (int)views->size()) return;
    auto& view = (*views)[selected_view_];
    if (loop_idx < 0 || loop_idx >= (int)view.loops.size()) return;
    auto& loop = view.loops[loop_idx];

    EnsurePaletteSet();

    auto* project = editor_.GetApp().GetProject();
    auto* loader = project ? project->GetSpriteLoader() : nullptr;

    ImVec2 start_pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float pad = Dpi(4);
    float total_w = (thumb_size_ + pad) * loop.frames.size() + pad;
    float strip_h = thumb_size_ + Dpi(22);

    // Build frame input list for the renderer
    std::vector<ViewRenderer::FrameInput> frame_inputs;
    for (auto& fr : loop.frames)
    {
        ViewRenderer::FrameInput fi;
        fi.sprite_id = fr.sprite_id;
        fi.flipped = fr.flipped;
        frame_inputs.push_back(fi);
    }

    // Render the composited strip via ViewRenderer
    auto strip = view_renderer_.RenderLoopStrip(frame_inputs, (int)thumb_size_, loader);

    if (strip.texture && strip.width > 0 && strip.height > 0)
    {
        // Draw the composited strip as a single ImGui image
        float scale_x = total_w / strip.width;
        float scale_y = (strip_h - Dpi(18)) / strip.height;
        float scale = std::min(scale_x, scale_y);
        float draw_w = strip.width * scale;
        float draw_h = strip.height * scale;

        dl->AddImage((ImTextureID)(intptr_t)strip.texture,
            start_pos,
            ImVec2(start_pos.x + draw_w, start_pos.y + draw_h));

        // Manage texture lifetime with ring buffer
        if (strip_tex_ring_[strip_tex_ring_idx_])
            SDL_DestroyTexture(strip_tex_ring_[strip_tex_ring_idx_]);
        strip_tex_ring_[strip_tex_ring_idx_] = strip.texture;
        strip_tex_ring_idx_ = (strip_tex_ring_idx_ + 1) % kStripTexRingSize;
    }
    else
    {
        // Fallback: dark background
        dl->AddRectFilled(start_pos,
            ImVec2(start_pos.x + std::max(total_w, ImGui::GetContentRegionAvail().x),
                   start_pos.y + strip_h),
            IM_COL32(35, 35, 45, 255));
    }

    // Draw selection overlays, frame numbers, and indicators on top
    for (int f = 0; f < (int)loop.frames.size(); f++)
    {
        auto& frame = loop.frames[f];
        float fx = start_pos.x + pad + f * (thumb_size_ + pad);
        float fy = start_pos.y + pad;

        bool is_selected = IsFrameSelected(loop_idx, f);
        bool is_primary = (primary_frame_.loop == loop_idx && primary_frame_.frame == f);

        // Selection highlight overlay
        if (is_selected)
        {
            ImU32 sel_col = is_primary
                ? IM_COL32(80, 120, 200, 80)
                : IM_COL32(60, 90, 160, 60);
            dl->AddRectFilled(ImVec2(fx, fy),
                ImVec2(fx + thumb_size_, fy + thumb_size_), sel_col);
        }

        // Flip indicator
        if (frame.flipped)
            dl->AddText(ImVec2(fx + 2, fy + 2), IM_COL32(255, 200, 0, 255), "F");

        // Sound indicator
        if (frame.sound >= 0)
        {
            float sx = fx + thumb_size_ - Dpi(14);
            dl->AddText(ImVec2(sx, fy + 2), IM_COL32(100, 255, 100, 255), "S");
        }

        // Selection border
        ImU32 border_col = is_selected ? IM_COL32(255, 255, 0, 255) : IM_COL32(120, 120, 120, 200);
        dl->AddRect(ImVec2(fx, fy), ImVec2(fx + thumb_size_, fy + thumb_size_),
                    border_col, 0, 0, is_selected ? 2.0f : 1.0f);

        // Frame number label
        char frame_lbl[16];
        std::snprintf(frame_lbl, sizeof(frame_lbl), "%d", f);
        ImVec2 lts = ImGui::CalcTextSize(frame_lbl);
        dl->AddText(ImVec2(fx + (thumb_size_ - lts.x) * 0.5f, fy + thumb_size_ + 2),
                    IM_COL32(180, 180, 180, 200), frame_lbl);

        // Delay indicator
        if (frame.delay > 0)
        {
            char delay_txt[8];
            std::snprintf(delay_txt, sizeof(delay_txt), "+%d", frame.delay);
            ImVec2 dts = ImGui::CalcTextSize(delay_txt);
            dl->AddText(ImVec2(fx + thumb_size_ - dts.x - 2, fy + thumb_size_ - dts.y - 2),
                        IM_COL32(255, 180, 80, 200), delay_txt);
        }
    }

    // Invisible interaction region (same as original)
    char strip_id[32];
    std::snprintf(strip_id, sizeof(strip_id), "##frame_strip_%d", loop_idx);
    ImGui::SetCursorScreenPos(start_pos);
    ImGui::InvisibleButton(strip_id, ImVec2(std::max(total_w, ImGui::GetContentRegionAvail().x), strip_h));

    // Handle click selection
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        ImVec2 mouse = ImGui::GetMousePos();
        int clicked_frame = -1;
        for (int f = 0; f < (int)loop.frames.size(); f++)
        {
            float fx = start_pos.x + pad + f * (thumb_size_ + pad);
            float fy = start_pos.y + pad;
            if (mouse.x >= fx && mouse.x <= fx + thumb_size_ && mouse.y >= fy && mouse.y <= fy + thumb_size_)
            {
                clicked_frame = f;
                break;
            }
        }

        if (clicked_frame >= 0)
        {
            bool ctrl = ImGui::GetIO().KeyCtrl;
            bool shift = ImGui::GetIO().KeyShift;
            FrameRef clicked{loop_idx, clicked_frame};

            if (shift && primary_frame_.IsValid())
            {
                SelectRange(primary_frame_, clicked);
            }
            else if (ctrl)
            {
                auto it = std::find(selected_frames_.begin(), selected_frames_.end(), clicked);
                if (it != selected_frames_.end())
                    selected_frames_.erase(it);
                else
                    selected_frames_.push_back(clicked);
                primary_frame_ = clicked;
            }
            else
            {
                selected_frames_ = { clicked };
                primary_frame_ = clicked;
            }
        }
        else
        {
            if (!ImGui::GetIO().KeyCtrl)
                ClearSelection();
        }
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        show_context_menu_ = true;
        context_menu_loop_ = loop_idx;
    }
}

} // namespace AGSEditor
