// AGS Editor ImGui - Room Editor implementation (Phase 7)
// Full DrawList-based rendering of room elements: hotspots, walkable areas,
// walk-behinds, regions, objects, characters, edges, with undo/redo.
#include "room_editor.h"
#include "ui/log_panel.h"
#include "ui/file_dialog.h"
#include "ui/sprite_chooser.h"
#include "ui/custom_property_widgets.h"
#include "project/project.h"
#include "project/game_data.h"
#include "project/room_loader.h"
#include "core/script_name_validator.h"
#include "project/texture_cache.h"
#include "project/sprite_loader.h"
#include "app.h"
#include "core/dpi_helper.h"
#include "ui/events_widget.h"
#include "gfx/bitmap.h"
#include "imgui.h"
#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

namespace AGSEditor
{

// Static member definition
constexpr const char* RoomEditor::kLayerNames[];

// ============================================================================
// Color helpers — distinct hues per area ID
// ============================================================================

ImU32 RoomEditor::GetAreaColor(int id, int alpha)
{
    // Use golden-angle hue distribution for visually distinct colors
    static const ImU32 kPalette[] = {
        IM_COL32(255, 80,  80, 100),  // 0: red
        IM_COL32( 80, 180, 255, 100), // 1: blue
        IM_COL32( 80, 255, 120, 100), // 2: green
        IM_COL32(255, 200,  60, 100), // 3: yellow
        IM_COL32(200, 100, 255, 100), // 4: purple
        IM_COL32(255, 140,  60, 100), // 5: orange
        IM_COL32( 60, 255, 220, 100), // 6: cyan
        IM_COL32(255,  80, 200, 100), // 7: pink
        IM_COL32(160, 255,  80, 100), // 8: lime
        IM_COL32(255, 160, 160, 100), // 9: salmon
        IM_COL32(100, 140, 255, 100), // 10: indigo
        IM_COL32(200, 200, 100, 100), // 11: khaki
        IM_COL32(140, 255, 180, 100), // 12: mint
        IM_COL32(255, 100, 140, 100), // 13: rose
        IM_COL32(100, 220, 200, 100), // 14: teal
        IM_COL32(220, 160, 255, 100), // 15: lavender
    };
    int idx = id % 16;
    ImU32 base = kPalette[idx];
    // Replace alpha
    return (base & 0x00FFFFFF) | ((ImU32)alpha << 24);
}

ImU32 RoomEditor::GetAreaOutlineColor(int id)
{
    return GetAreaColor(id, 220);
}

// ============================================================================
// Constructor / Data loading
// ============================================================================

RoomEditor::RoomEditor(EditorUI& editor, int room_number)
    : editor_(editor)
    , room_number_(room_number)
{
    // Build title with room description if available
    auto* project = editor_.GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    std::string desc;
    if (gd)
    {
        for (const auto& ri : gd->rooms)
        {
            if (ri.number == room_number && !ri.description.empty())
            {
                desc = ri.description;
                break;
            }
        }
    }
    char buf[256];
    if (!desc.empty())
        snprintf(buf, sizeof(buf), "Room %d: %s", room_number_, desc.c_str());
    else
        snprintf(buf, sizeof(buf), "Room %d", room_number_);
    title_ = buf;

    LoadRoomData();
}

void RoomEditor::LoadRoomData()
{
    load_attempted_ = true;
    auto* project = editor_.GetProject();
    if (!project || !project->IsLoaded())
    {
        editor_.GetLogPanel().AddLog("[Room] No project loaded.");
        return;
    }

    auto* loader = project->GetRoomLoader();
    if (!loader)
    {
        editor_.GetLogPanel().AddLog("[Room] Room loader not available.");
        return;
    }

    room_data_ = loader->LoadRoom(room_number_);
    if (room_data_)
    {
        data_loaded_ = true;
        editor_.GetLogPanel().AddLog("[Room] Loaded room %d (%dx%d), objects=%d, hotspots=%d.",
            room_number_, room_data_->width, room_data_->height,
            (int)room_data_->objects.size(), (int)room_data_->hotspots.size());
    }
    else
    {
        // Create default placeholder data
        room_data_ = std::make_unique<RoomData>();
        room_data_->number = room_number_;
        data_loaded_ = true;
        editor_.GetLogPanel().AddLog("[Room] Room %d: using default (no .crm file found).", room_number_);
    }
}

void RoomEditor::SaveRoomData()
{
    auto* project = editor_.GetProject();
    if (!project || !project->IsLoaded() || !room_data_)
        return;

    auto* loader = project->GetRoomLoader();
    if (loader && loader->SaveRoom(room_number_, *room_data_))
    {
        editor_.GetLogPanel().AddLog("[Room] Saved room %d.", room_number_);
    }
    else
    {
        editor_.GetLogPanel().AddLog("[Room] Save room %d failed (not yet implemented).", room_number_);
    }
}

// ============================================================================
// Coordinate helpers
// ============================================================================

ImVec2 RoomEditor::ScreenToRoom(ImVec2 screen_pos, ImVec2 origin) const
{
    return ImVec2(
        (screen_pos.x - origin.x - scroll_x_) / zoom_,
        (screen_pos.y - origin.y - scroll_y_) / zoom_
    );
}

ImVec2 RoomEditor::RoomToScreen(ImVec2 room_pos, ImVec2 origin) const
{
    return ImVec2(
        origin.x + scroll_x_ + room_pos.x * zoom_,
        origin.y + scroll_y_ + room_pos.y * zoom_
    );
}

ImVec2 RoomEditor::RoomToScreen(float rx, float ry, ImVec2 origin) const
{
    return RoomToScreen(ImVec2(rx, ry), origin);
}

// ============================================================================
// Main Draw
// ============================================================================

void RoomEditor::Draw()
{
    if (!room_data_ && !load_attempted_)
        LoadRoomData();

    DrawToolbar();
    ImGui::Separator();

    float avail_w = ImGui::GetContentRegionAvail().x;
    float sidebar_w = Dpi(220);

    // Canvas area (left)
    ImGui::BeginChild("##RoomCanvas", ImVec2(avail_w - sidebar_w - Dpi(8), 0), ImGuiChildFlags_Borders);
    DrawCanvas();
    ImGui::EndChild();

    ImGui::SameLine();

    // Sidebar (right): layers + properties + item list
    ImGui::BeginChild("##RoomSidebar", ImVec2(sidebar_w, 0), ImGuiChildFlags_Borders);
    DrawLayerPanel();
    ImGui::Spacing();
    DrawItemList();
    ImGui::Spacing();
    DrawPropertiesPanel();
    ImGui::EndChild();
}

// ============================================================================
// Toolbar
// ============================================================================

void RoomEditor::DrawToolbar()
{
    // Save
    if (ImGui::Button("Save"))
        SaveRoomData();
    ImGui::SameLine();

    // Undo / Redo
    bool can_undo = (undo_pos_ >= 0);
    bool can_redo = (undo_pos_ < (int)undo_stack_.size() - 1);
    if (!can_undo) ImGui::BeginDisabled();
    if (ImGui::Button("Undo"))
        Undo();
    if (!can_undo) ImGui::EndDisabled();
    ImGui::SameLine();
    if (!can_redo) ImGui::BeginDisabled();
    if (ImGui::Button("Redo"))
        Redo();
    if (!can_redo) ImGui::EndDisabled();
    ImGui::SameLine();

    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Tool buttons
    const char* tool_labels[] = { "Select", "Freehand", "Rect", "Line", "Fill", "Erase" };
    for (int i = 0; i < 6; i++)
    {
        bool active = ((int)current_tool_ == i);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(tool_labels[i]))
            current_tool_ = (RoomTool)i;
        if (active) ImGui::PopStyleColor();
        ImGui::SameLine();
    }

    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Zoom
    ImGui::SetNextItemWidth(Dpi(80));
    ImGui::SliderFloat("##Zoom", &zoom_, 0.1f, 8.0f, "%.1fx");
    ImGui::SameLine();
    if (ImGui::Button("1:1")) zoom_ = 1.0f;
    ImGui::SameLine();
    if (ImGui::Button("Fit"))
    {
        // Auto-fit room to viewport
        if (room_data_)
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float sidebar_w = Dpi(220) + Dpi(8);
            float canvas_w = avail.x - sidebar_w;
            float canvas_h = avail.y;
            if (canvas_w > 0 && canvas_h > 0 && room_data_->width > 0 && room_data_->height > 0)
            {
                float zoom_x = canvas_w / (float)room_data_->width;
                float zoom_y = canvas_h / (float)room_data_->height;
                zoom_ = std::min(zoom_x, zoom_y) * 0.95f; // 95% to leave small margin
                if (zoom_ < 0.1f) zoom_ = 0.1f;
                if (zoom_ > 8.0f) zoom_ = 8.0f;
                // Center the room
                scroll_x_ = (canvas_w - room_data_->width * zoom_) * 0.5f;
                scroll_y_ = (canvas_h - room_data_->height * zoom_) * 0.5f;
            }
        }
    }
    ImGui::SameLine();

    // Grid toggle
    ImGui::Checkbox("Grid", &show_grid_);
    ImGui::SameLine();
    if (show_grid_)
    {
        ImGui::SetNextItemWidth(Dpi(50));
        ImGui::InputInt("##GridSz", &grid_size_, 0);
        if (grid_size_ < 4) grid_size_ = 4;
        if (grid_size_ > 64) grid_size_ = 64;
        ImGui::SameLine();
    }

    // Brush size (for draw/erase/freehand tools)
    if (current_tool_ == RoomTool::Draw || current_tool_ == RoomTool::Erase || current_tool_ == RoomTool::Freehand)
    {
        ImGui::SetNextItemWidth(Dpi(60));
        ImGui::SliderInt("Brush", &brush_size_, 1, 10);
        ImGui::SameLine();
    }

    // Mask transparency slider (visible when a mask layer is active)
    if (active_layer_ == Layer_Hotspots || active_layer_ == Layer_WalkAreas ||
        active_layer_ == Layer_WalkBehinds || active_layer_ == Layer_Regions)
    {
        ImGui::SetNextItemWidth(Dpi(80));
        ImGui::SliderFloat("Mask Alpha", &mask_alpha_, 0.0f, 1.0f, "%.2f");
        ImGui::SameLine();
        ImGui::Checkbox("Grey Others", &grey_out_other_masks_);
        ImGui::SameLine();
    }

    // Room dimensions
    if (room_data_)
    {
        ImGui::TextDisabled("Room: %dx%d", room_data_->width, room_data_->height);
    }

    // Room script buttons
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    {
        char script_name[32];
        snprintf(script_name, sizeof(script_name), "room%d.asc", room_number_);
        if (ImGui::Button("Edit Script"))
            editor_.OpenScriptFile(script_name);
        ImGui::SameLine();
        char header_name[32];
        snprintf(header_name, sizeof(header_name), "room%d.ash", room_number_);
        if (ImGui::Button("Edit Header"))
            editor_.OpenScriptFile(header_name);
    }

    // Export / Import room
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    if (ImGui::Button("Export .crm"))
    {
        auto* project = editor_.GetApp().GetProject();
        std::string default_dir = (project && project->IsLoaded()) ? project->GetProjectDir() : ".";
        FileDialog::Open(FileDialogType::SaveFile, "Export Room",
            ".crm{AGS Room File}",
            default_dir,
            [this](const std::string& path) {
                auto* project2 = editor_.GetApp().GetProject();
                if (!project2 || !project2->IsLoaded()) return;
                auto* rl = project2->GetRoomLoader();
                if (rl && rl->ExportRoom(room_number_, path))
                    editor_.GetLogPanel().AddLog("[Room] Exported room %d to: %s", room_number_, path.c_str());
                else
                    editor_.GetLogPanel().AddLog("[Room] Failed to export room %d", room_number_);
            });
    }
    ImGui::SameLine();
    if (ImGui::Button("Import .crm"))
    {
        auto* project = editor_.GetApp().GetProject();
        std::string default_dir = (project && project->IsLoaded()) ? project->GetProjectDir() : ".";
        FileDialog::Open(FileDialogType::OpenFile, "Import Room",
            ".crm{AGS Room File}",
            default_dir,
            [this](const std::string& path) {
                auto* project2 = editor_.GetApp().GetProject();
                if (!project2 || !project2->IsLoaded()) return;
                auto* rl = project2->GetRoomLoader();
                if (!rl) return;
                int new_room = rl->ImportRoom(path);
                if (new_room >= 0)
                {
                    // Add to game data room list
                    auto* gd = project2->GetGameData();
                    if (gd)
                    {
                        GameData::RoomInfo ri;
                        ri.number = new_room;
                        ri.description = "Room " + std::to_string(new_room);
                        gd->rooms.push_back(ri);
                    }
                    editor_.GetLogPanel().AddLog("[Room] Imported room as room%d from: %s", new_room, path.c_str());
                }
                else
                {
                    editor_.GetLogPanel().AddLog("[Room] Failed to import room from: %s", path.c_str());
                }
            });
    }
}

// ============================================================================
// Canvas — the main room viewport
// ============================================================================

void RoomEditor::DrawCanvas()
{
    if (!room_data_) return;

    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Auto-fit room to canvas on first draw
    if (need_auto_fit_ && canvas_size.x > 0 && canvas_size.y > 0 &&
        room_data_->width > 0 && room_data_->height > 0)
    {
        need_auto_fit_ = false;
        float zoom_x = canvas_size.x / (float)room_data_->width;
        float zoom_y = canvas_size.y / (float)room_data_->height;
        zoom_ = std::min(zoom_x, zoom_y) * 0.95f;
        if (zoom_ < 0.1f) zoom_ = 0.1f;
        if (zoom_ > 8.0f) zoom_ = 8.0f;
        scroll_x_ = (canvas_size.x - room_data_->width * zoom_) * 0.5f;
        scroll_y_ = (canvas_size.y - room_data_->height * zoom_) * 0.5f;
    }

    // Clip to canvas
    dl->PushClipRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), true);

    ImVec2 origin = canvas_pos; // Top-left of the canvas in screen coords

    // 1) Background
    if (layer_visible_[Layer_Background])
        DrawRoomBackground(dl, origin);

    // 2) Grid overlay
    if (show_grid_)
        DrawGrid(dl, origin);

    // 3) Walkable areas (drawn as filled rectangles / region indicators)
    if (layer_visible_[Layer_WalkAreas])
        DrawWalkAreas(dl, origin);

    // 4) Walk-behinds (baseline lines)
    if (layer_visible_[Layer_WalkBehinds])
        DrawWalkBehinds(dl, origin);

    // 5) Regions (tinted overlays)
    if (layer_visible_[Layer_Regions])
        DrawRegions(dl, origin);

    // 6) Hotspots (labeled areas)
    if (layer_visible_[Layer_Hotspots])
        DrawHotspots(dl, origin);

    // 7) Objects (positioned sprites / markers)
    if (layer_visible_[Layer_Objects])
        DrawObjects(dl, origin);

    // 8) Characters
    if (layer_visible_[Layer_Characters])
        DrawCharacters(dl, origin);

    // 9) Edges (room boundary lines)
    if (layer_visible_[Layer_Edges])
        DrawEdges(dl, origin);

    // 10) Selection highlight
    DrawSelection(dl, origin);

    // 11) Tool preview (draw/line cursor)
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImVec2 mouse_room = ScreenToRoom(mouse_pos, origin);
    DrawToolPreview(dl, origin, mouse_room);

    dl->PopClipRect();

    // Draw scrollbar indicators
    if (room_data_)
    {
        float room_w = room_data_->width * zoom_;
        float room_h = room_data_->height * zoom_;
        float scrollbar_thickness = Dpi(8);
        ImU32 scrollbar_bg = IM_COL32(40, 40, 50, 120);
        ImU32 scrollbar_fg = IM_COL32(120, 120, 150, 180);

        // Horizontal scrollbar
        if (room_w > canvas_size.x)
        {
            float bar_y = canvas_pos.y + canvas_size.y - scrollbar_thickness;
            dl->AddRectFilled(
                ImVec2(canvas_pos.x, bar_y),
                ImVec2(canvas_pos.x + canvas_size.x, bar_y + scrollbar_thickness),
                scrollbar_bg);
            float visible_frac = canvas_size.x / room_w;
            float scroll_frac = -scroll_x_ / room_w;
            float thumb_w = canvas_size.x * std::min(visible_frac, 1.0f);
            float thumb_x = canvas_pos.x + canvas_size.x * std::clamp(scroll_frac, 0.0f, 1.0f - visible_frac);
            dl->AddRectFilled(
                ImVec2(thumb_x, bar_y + 1),
                ImVec2(thumb_x + thumb_w, bar_y + scrollbar_thickness - 1),
                scrollbar_fg, 3.0f);
        }

        // Vertical scrollbar
        if (room_h > canvas_size.y)
        {
            float bar_x = canvas_pos.x + canvas_size.x - scrollbar_thickness;
            dl->AddRectFilled(
                ImVec2(bar_x, canvas_pos.y),
                ImVec2(bar_x + scrollbar_thickness, canvas_pos.y + canvas_size.y),
                scrollbar_bg);
            float visible_frac = canvas_size.y / room_h;
            float scroll_frac = -scroll_y_ / room_h;
            float thumb_h = canvas_size.y * std::min(visible_frac, 1.0f);
            float thumb_y = canvas_pos.y + canvas_size.y * std::clamp(scroll_frac, 0.0f, 1.0f - visible_frac);
            dl->AddRectFilled(
                ImVec2(bar_x + 1, thumb_y),
                ImVec2(bar_x + scrollbar_thickness - 1, thumb_y + thumb_h),
                scrollbar_fg, 3.0f);
        }
    }

    // Invisible button for mouse interaction
    ImGui::InvisibleButton("##room_canvas", canvas_size,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);

    HandleCanvasInput(origin, canvas_size);
}

// ============================================================================
// DrawList renderers
// ============================================================================

void RoomEditor::DrawRoomBackground(ImDrawList* dl, ImVec2 origin)
{
    float w = room_data_->width * zoom_;
    float h = room_data_->height * zoom_;
    ImVec2 p0(origin.x + scroll_x_, origin.y + scroll_y_);
    ImVec2 p1(p0.x + w, p0.y + h);

    // Try to render real background bitmap
    bool rendered_bg = false;
    int frame_idx = std::clamp(selected_bg_frame_, 0,
        std::max(0, (int)room_data_->bg_frames.size() - 1));
    if (frame_idx < (int)room_data_->bg_frames.size() && room_data_->bg_frames[frame_idx])
    {
        auto& tex_cache = editor_.GetApp().GetTextureCache();
        char bg_key[64];
        snprintf(bg_key, sizeof(bg_key), "room%d_bg%d", room_number_, frame_idx);
        SDL_Texture* tex = tex_cache.GetOrCreateFromBitmap(bg_key, room_data_->bg_frames[frame_idx].get());
        if (tex)
        {
            dl->AddImage((ImTextureID)(intptr_t)tex, p0, p1);
            rendered_bg = true;
        }
    }

    if (!rendered_bg)
    {
        // Fallback: dark background fill
        dl->AddRectFilled(p0, p1, IM_COL32(30, 30, 42, 255));
        const char* label = "Room Background";
        ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(p0.x + (w - ts.x) * 0.5f, p0.y + 4), IM_COL32(100, 100, 140, 180), label);
    }

    // Border
    dl->AddRect(p0, p1, IM_COL32(100, 100, 130, 255), 0.0f, 0, 1.0f);

    // Show BG frame info
    char info[64];
    snprintf(info, sizeof(info), "%dx%d  %d bg frame(s)",
             room_data_->width, room_data_->height, room_data_->bg_frame_count);
    ImVec2 is = ImGui::CalcTextSize(info);
    dl->AddText(ImVec2(p0.x + (w - is.x) * 0.5f, p0.y + 4), IM_COL32(200, 200, 220, 180), info);
}

void RoomEditor::DrawGrid(ImDrawList* dl, ImVec2 origin)
{
    float w = room_data_->width * zoom_;
    float h = room_data_->height * zoom_;
    ImVec2 p0(origin.x + scroll_x_, origin.y + scroll_y_);

    float step = (float)grid_size_ * zoom_;
    if (step < 4.0f) return; // Don't draw if grid is too dense

    ImU32 col = IM_COL32(60, 60, 75, (step > 8.0f) ? 80 : 40);

    for (float x = 0; x <= w; x += step)
        dl->AddLine(ImVec2(p0.x + x, p0.y), ImVec2(p0.x + x, p0.y + h), col);
    for (float y = 0; y <= h; y += step)
        dl->AddLine(ImVec2(p0.x, p0.y + y), ImVec2(p0.x + w, p0.y + y), col);
}

void RoomEditor::DrawEdges(ImDrawList* dl, ImVec2 origin)
{
    float w = (float)room_data_->width;
    float h = (float)room_data_->height;
    float thickness = Dpi(2);
    // Left edge (vertical line)
    ImVec2 lt = RoomToScreen((float)room_data_->left_edge, 0, origin);
    ImVec2 lb = RoomToScreen((float)room_data_->left_edge, h, origin);
    dl->AddLine(lt, lb, IM_COL32(255, 100, 100, 200), thickness);
    dl->AddText(ImVec2(lt.x + 3, lt.y + 2), IM_COL32(255, 100, 100, 220), "L");

    // Right edge
    ImVec2 rt = RoomToScreen((float)room_data_->right_edge, 0, origin);
    ImVec2 rb = RoomToScreen((float)room_data_->right_edge, h, origin);
    dl->AddLine(rt, rb, IM_COL32(100, 255, 100, 200), thickness);
    dl->AddText(ImVec2(rt.x + 3, rt.y + 2), IM_COL32(100, 255, 100, 220), "R");

    // Top edge (horizontal line)
    ImVec2 tl = RoomToScreen(0, (float)room_data_->top_edge, origin);
    ImVec2 tr = RoomToScreen(w, (float)room_data_->top_edge, origin);
    dl->AddLine(tl, tr, IM_COL32(100, 100, 255, 200), thickness);
    dl->AddText(ImVec2(tl.x + 2, tl.y + 3), IM_COL32(100, 100, 255, 220), "T");

    // Bottom edge
    ImVec2 bl = RoomToScreen(0, (float)room_data_->bottom_edge, origin);
    ImVec2 br = RoomToScreen(w, (float)room_data_->bottom_edge, origin);
    dl->AddLine(bl, br, IM_COL32(255, 255, 100, 200), thickness);
    dl->AddText(ImVec2(bl.x + 2, bl.y - 14), IM_COL32(255, 255, 100, 220), "B");
}

void RoomEditor::DrawHotspots(ImDrawList* dl, ImVec2 origin)
{
    bool is_active_layer = (active_layer_ == Layer_Hotspots);
    float af = mask_alpha_;
    if (!is_active_layer && grey_out_other_masks_) af *= 0.3f;
    else if (!is_active_layer) af *= 0.5f;
    int alpha_fill = (int)(af * 128);
    int alpha_outline = (int)(af * 450);

    // If we have actual mask data, render from bitmap
    if (room_data_->hotspot_mask)
    {
        DrawMaskOverlay(dl, origin, room_data_->hotspot_mask.get(), 0, alpha_fill, alpha_outline, "HS");
        return;
    }

    // Fallback: distribute hotspot indicators evenly across the room
    float w = (float)room_data_->width;
    float h = (float)room_data_->height;
    int count = (int)room_data_->hotspots.size();

    for (int i = 1; i < count; i++) // Skip hotspot 0 (none)
    {
        auto& hs = room_data_->hotspots[i];

        // Distribute hotspot indicators evenly across the room
        int cols = std::max(1, (int)std::ceil(std::sqrt((double)(count - 1))));
        int row = (i - 1) / cols;
        int col = (i - 1) % cols;
        float cell_w = w / (float)cols;
        float cell_h = h / (float)std::max(1, (count - 1 + cols - 1) / cols);
        float cx = col * cell_w + cell_w * 0.5f;
        float cy = row * cell_h + cell_h * 0.5f;
        float half = std::min(cell_w, cell_h) * 0.35f;

        ImVec2 r0 = RoomToScreen(cx - half, cy - half, origin);
        ImVec2 r1 = RoomToScreen(cx + half, cy + half, origin);

        ImU32 fill = GetAreaColor(i, alpha_fill);
        ImU32 outline = GetAreaColor(i, alpha_outline);

        dl->AddRectFilled(r0, r1, fill);
        dl->AddRect(r0, r1, outline, 0.0f, 0, (selected_item_id_ == i && is_active_layer) ? 3.0f : 1.5f);

        // Label
        char label[64];
        if (!hs.name.empty())
            snprintf(label, sizeof(label), "%d: %s", i, hs.name.c_str());
        else
            snprintf(label, sizeof(label), "Hotspot %d", i);

        ImVec2 ts = ImGui::CalcTextSize(label);
        float lx = (r0.x + r1.x - ts.x) * 0.5f;
        float ly = (r0.y + r1.y - ts.y) * 0.5f;
        dl->AddText(ImVec2(lx, ly), GetAreaColor(i, 240), label);
    }
}

void RoomEditor::DrawWalkAreas(ImDrawList* dl, ImVec2 origin)
{
    bool is_active = (active_layer_ == Layer_WalkAreas);
    float af = mask_alpha_;
    if (!is_active && grey_out_other_masks_) af *= 0.3f;
    else if (!is_active) af *= 0.5f;
    int alpha_fill = (int)(af * 100);
    int alpha_outline = (int)(af * 400);

    // If we have actual mask data, render from bitmap
    if (room_data_->walkarea_mask)
    {
        DrawMaskOverlay(dl, origin, room_data_->walkarea_mask.get(), 5, alpha_fill, alpha_outline, "WA");
        return;
    }

    // Fallback: distribute walk area indicators as horizontal bands
    float w = (float)room_data_->width;
    float h = (float)room_data_->height;
    int count = (int)room_data_->walk_areas.size();

    for (int i = 1; i < count; i++) // Skip area 0 (none)
    {
        auto& wa = room_data_->walk_areas[i];

        // Distribute walk area indicators as horizontal bands
        int band_count = std::max(1, count - 1);
        float band_h = h / (float)band_count;
        float y0 = (i - 1) * band_h;
        float y1 = y0 + band_h;

        ImVec2 r0 = RoomToScreen(0, y0, origin);
        ImVec2 r1 = RoomToScreen(w, y1, origin);

        ImU32 fill = GetAreaColor(i + 5, alpha_fill); // Offset palette index
        ImU32 outline = GetAreaColor(i + 5, alpha_outline);

        dl->AddRectFilled(r0, r1, fill);
        dl->AddRect(r0, r1, outline, 0.0f, 0,
                    (selected_item_id_ == i && is_active) ? 2.5f : 1.0f);

        // Scaling info label
        char label[80];
        snprintf(label, sizeof(label), "WA %d (scale %d%%-%d%%)", i,
                 wa.scaling_min, wa.scaling_max);
        dl->AddText(ImVec2(r0.x + 4, r0.y + 2), GetAreaColor(i + 5, 220), label);
    }
}

void RoomEditor::DrawWalkBehinds(ImDrawList* dl, ImVec2 origin)
{
    bool is_active = (active_layer_ == Layer_WalkBehinds);
    float w = (float)room_data_->width;

    for (int i = 1; i < (int)room_data_->walk_behinds.size(); i++)
    {
        auto& wb = room_data_->walk_behinds[i];
        float baseline_y = (float)wb.baseline;

        ImVec2 left = RoomToScreen(0, baseline_y, origin);
        ImVec2 right = RoomToScreen(w, baseline_y, origin);

        ImU32 col = GetAreaColor(i + 2, is_active ? 200 : 120);
        float thickness = (selected_item_id_ == i && is_active) ? 3.0f : 1.5f;

        // Dashed line effect using segments
        float dash_len = Dpi(8);
        float gap_len = Dpi(4);
        float total_len = right.x - left.x;
        float pos = 0;
        while (pos < total_len)
        {
            float seg_end = std::min(pos + dash_len, total_len);
            dl->AddLine(
                ImVec2(left.x + pos, left.y),
                ImVec2(left.x + seg_end, left.y),
                col, thickness);
            pos = seg_end + gap_len;
        }

        // Label
        char label[48];
        snprintf(label, sizeof(label), "WB %d (base %d)", i, wb.baseline);
        dl->AddText(ImVec2(left.x + Dpi(4), left.y - Dpi(14)), col, label);

        // Small triangle marker on the left side
        dl->AddTriangleFilled(
            ImVec2(left.x, left.y - Dpi(5)),
            ImVec2(left.x, left.y + Dpi(5)),
            ImVec2(left.x + Dpi(8), left.y),
            col);
    }
}

void RoomEditor::DrawRegions(ImDrawList* dl, ImVec2 origin)
{
    bool is_active = (active_layer_ == Layer_Regions);
    float af = mask_alpha_;
    if (!is_active && grey_out_other_masks_) af *= 0.3f;
    else if (!is_active) af *= 0.5f;
    int alpha_fill = (int)(af * 88);
    int alpha_outline = (int)(af * 375);

    // If we have actual mask data, render from bitmap
    if (room_data_->region_mask)
    {
        DrawMaskOverlay(dl, origin, room_data_->region_mask.get(), 8, alpha_fill, alpha_outline, "R");
        return;
    }

    // Fallback: distribute as a grid of cells
    float w = (float)room_data_->width;
    float h = (float)room_data_->height;
    int count = (int)room_data_->regions.size();

    for (int i = 1; i < count; i++) // Skip region 0
    {
        auto& reg = room_data_->regions[i];

        // Distribute as a grid of cells
        int cols = std::max(1, (int)std::ceil(std::sqrt((double)(count - 1))));
        int row = (i - 1) / cols;
        int col = (i - 1) % cols;
        float cell_w = w / (float)cols;
        float cell_h = h / (float)std::max(1, (count - 1 + cols - 1) / cols);
        float x0 = col * cell_w + cell_w * 0.1f;
        float y0 = row * cell_h + cell_h * 0.1f;
        float x1 = (col + 1) * cell_w - cell_w * 0.1f;
        float y1 = (row + 1) * cell_h - cell_h * 0.1f;

        ImVec2 r0 = RoomToScreen(x0, y0, origin);
        ImVec2 r1 = RoomToScreen(x1, y1, origin);

        // Use tint color if set, otherwise palette color
        ImU32 fill, outl;
        if (reg.tint_r || reg.tint_g || reg.tint_b)
        {
            fill = IM_COL32(reg.tint_r, reg.tint_g, reg.tint_b, alpha_fill);
            outl = IM_COL32(reg.tint_r, reg.tint_g, reg.tint_b, alpha_outline);
        }
        else
        {
            fill = GetAreaColor(i + 8, alpha_fill);
            outl = GetAreaColor(i + 8, alpha_outline);
        }

        dl->AddRectFilled(r0, r1, fill);
        dl->AddRect(r0, r1, outl, 0.0f, 0,
                    (selected_item_id_ == i && is_active) ? 2.5f : 1.0f);

        // Cross-hatch pattern for regions (diagonal lines)
        float spacing = Dpi(12);
        for (float d = 0; d < (r1.x - r0.x) + (r1.y - r0.y); d += spacing)
        {
            float x_start = r0.x + d;
            float y_start = r0.y;
            float x_end = r0.x;
            float y_end = r0.y + d;

            // Clip to rectangle
            if (x_start > r1.x) { y_start += (x_start - r1.x); x_start = r1.x; }
            if (y_end > r1.y) { x_end += (y_end - r1.y); y_end = r1.y; }

            if (x_start >= r0.x && x_start <= r1.x && y_start >= r0.y && y_start <= r1.y &&
                x_end >= r0.x && x_end <= r1.x && y_end >= r0.y && y_end <= r1.y)
            {
                dl->AddLine(ImVec2(x_start, y_start), ImVec2(x_end, y_end),
                            (outl & 0x00FFFFFF) | (40 << 24));
            }
        }

        char label[64];
        snprintf(label, sizeof(label), "R%d (light:%d)", i, reg.light_level);
        dl->AddText(ImVec2(r0.x + 3, r0.y + 3), (outl & 0x00FFFFFF) | (220 << 24), label);
    }
}

void RoomEditor::DrawObjects(ImDrawList* dl, ImVec2 origin)
{
    bool is_active = (active_layer_ == Layer_Objects);
    auto* project = editor_.GetProject();
    auto* loader = project ? project->GetSpriteLoader() : nullptr;
    auto& tex_cache = editor_.GetApp().GetTextureCache();

    for (int i = 0; i < (int)room_data_->objects.size(); i++)
    {
        auto& obj = room_data_->objects[i];

        // Skip design-time hidden objects
        if (!obj.design_visible) continue;

        // Try to get real sprite dimensions from the sprite loader
        float obj_w = 24.0f;
        float obj_h = 24.0f;
        SDL_Texture* spr_tex = nullptr;
        if (loader && loader->IsOpen() && obj.sprite >= 0)
        {
            const auto* met = loader->GetMetrics(obj.sprite);
            if (met && met->exists)
            {
                obj_w = (float)met->width;
                obj_h = (float)met->height;
            }
            spr_tex = tex_cache.GetSpriteTexture(obj.sprite, loader);
        }

        // AGS objects: (x, y) is the bottom-left corner
        ImVec2 p0 = RoomToScreen((float)obj.x, (float)obj.y - obj_h, origin);
        ImVec2 p1 = RoomToScreen((float)obj.x + obj_w, (float)obj.y, origin);

        // Dimmed if not visible
        int alpha_fill = obj.visible ? 60 : 25;
        int alpha_outline = obj.visible ? 220 : 100;
        ImU32 fill_col = IM_COL32(80, 200, 255, alpha_fill);
        ImU32 outline_col = IM_COL32(80, 200, 255, alpha_outline);

        if (is_active && selected_item_id_ == i)
        {
            fill_col = IM_COL32(255, 220, 80, 80);
            outline_col = IM_COL32(255, 220, 80, 255);
        }

        if (spr_tex)
        {
            // Render actual sprite texture
            ImVec4 tint(1, 1, 1, obj.visible ? 1.0f : 0.4f);
            dl->AddImage((ImTextureID)(intptr_t)spr_tex, p0, p1,
                         ImVec2(0, 0), ImVec2(1, 1), ImGui::ColorConvertFloat4ToU32(tint));
        }
        else
        {
            // Fallback: colored rectangle
            dl->AddRectFilled(p0, p1, fill_col);
            char spr_label[16];
            snprintf(spr_label, sizeof(spr_label), "S%d", obj.sprite);
            ImVec2 ts = ImGui::CalcTextSize(spr_label);
            dl->AddText(ImVec2((p0.x + p1.x - ts.x) * 0.5f, (p0.y + p1.y - ts.y) * 0.5f),
                        IM_COL32(255, 255, 255, alpha_outline), spr_label);
        }

        // Selection / outline
        dl->AddRect(p0, p1, outline_col, 0.0f, 0, (is_active && selected_item_id_ == i) ? 2.5f : 1.0f);

        // Name label below
        const char* name = obj.name.empty() ? obj.script_name.c_str() : obj.name.c_str();
        if (name[0])
        {
            ImVec2 ns = ImGui::CalcTextSize(name);
            dl->AddText(ImVec2((p0.x + p1.x - ns.x) * 0.5f, p1.y + 1),
                        IM_COL32(180, 220, 255, alpha_outline), name);
        }

        // Position crosshair at object origin
        ImVec2 cross = RoomToScreen((float)obj.x, (float)obj.y, origin);
        dl->AddLine(ImVec2(cross.x - Dpi(5), cross.y), ImVec2(cross.x + Dpi(5), cross.y),
                    outline_col, 1.0f);
        dl->AddLine(ImVec2(cross.x, cross.y - Dpi(5)), ImVec2(cross.x, cross.y + Dpi(5)),
                    outline_col, 1.0f);

        // Lock indicator: X cross on locked selected objects
        if (obj.design_locked && is_active && selected_item_id_ == i)
        {
            float cx = (p0.x + p1.x) * 0.5f;
            float cy = (p0.y + p1.y) * 0.5f;
            float sz = Dpi(8);
            ImU32 lock_col = IM_COL32(255, 80, 80, 200);
            dl->AddLine(ImVec2(cx - sz, cy - sz), ImVec2(cx + sz, cy + sz), lock_col, 2.0f);
            dl->AddLine(ImVec2(cx + sz, cy - sz), ImVec2(cx - sz, cy + sz), lock_col, 2.0f);
        }
    }
}

void RoomEditor::DrawCharacters(ImDrawList* dl, ImVec2 origin)
{
    // Draw characters that are placed in this room
    auto* project = editor_.GetProject();
    if (!project || !project->GetGameData()) return;

    auto* gd = project->GetGameData();
    auto& chars = gd->characters;
    auto* loader = project->GetSpriteLoader();
    auto& tex_cache = editor_.GetApp().GetTextureCache();
    bool is_active = (active_layer_ == Layer_Characters);

    for (int i = 0; i < (int)chars.size(); i++)
    {
        auto& ch = chars[i];
        if (ch.room != room_number_) continue;

        // Position: character feet are at (x,y), sprite is drawn above
        ImVec2 feet = RoomToScreen((float)ch.x, (float)ch.y, origin);
        bool selected = is_active && selected_item_id_ == i;

        ImU32 outline_col = selected ? IM_COL32(255, 255, 100, 255)
                                     : IM_COL32(255, 160, 60, is_active ? 240 : 120);

        // Try to render actual sprite from normal_view -> loop 0 -> frame 0
        bool rendered_sprite = false;
        auto* char_view = (ch.normal_view >= 0) ? gd->FindViewById(ch.normal_view) : nullptr;
        if (loader && char_view)
        {
            auto& view = *char_view;
            if (!view.loops.empty() && !view.loops[0].frames.empty())
            {
                int sprite_id = view.loops[0].frames[0].sprite_id;
                SDL_Texture* tex = tex_cache.GetSpriteTexture(sprite_id, loader);
                if (tex)
                {
                    const auto* info = tex_cache.FindSprite(sprite_id);
                    if (info && info->width > 0 && info->height > 0)
                    {
                        float sw = (float)info->width * zoom_;
                        float sh = (float)info->height * zoom_;

                        // Sprite bottom-center aligns with character feet
                        ImVec2 p0(feet.x - sw * 0.5f, feet.y - sh);
                        ImVec2 p1(feet.x + sw * 0.5f, feet.y);

                        // Tint if inactive or selected
                        ImU32 tint = IM_COL32(255, 255, 255, is_active ? 255 : 160);
                        dl->AddImage((ImTextureID)(intptr_t)tex, p0, p1,
                                     ImVec2(0, 0), ImVec2(1, 1), tint);

                        // Selection outline around sprite
                        if (selected)
                            dl->AddRect(p0, p1, outline_col, 0, 0, 2.0f);

                        rendered_sprite = true;
                    }
                }
            }
        }

        // Fallback: diamond marker when no sprite available
        if (!rendered_sprite)
        {
            float diamond_r = Dpi(12);
            ImU32 fill = selected ? IM_COL32(255, 220, 80, 100)
                                  : IM_COL32(255, 160, 60, is_active ? 80 : 40);

            ImVec2 pts[4] = {
                ImVec2(feet.x, feet.y - diamond_r),
                ImVec2(feet.x + diamond_r, feet.y),
                ImVec2(feet.x, feet.y + diamond_r),
                ImVec2(feet.x - diamond_r, feet.y),
            };
            dl->AddConvexPolyFilled(pts, 4, fill);
            dl->AddPolyline(pts, 4, outline_col, ImDrawFlags_Closed,
                            selected ? 2.5f : 1.5f);
        }

        // Label below feet
        const char* name = ch.script_name.empty() ? ch.real_name.c_str() : ch.script_name.c_str();
        if (name[0])
        {
            ImVec2 ts = ImGui::CalcTextSize(name);
            dl->AddText(ImVec2(feet.x - ts.x * 0.5f, feet.y + 2),
                        outline_col, name);
        }
    }
}

void RoomEditor::DrawSelection(ImDrawList* dl, ImVec2 origin)
{
    // Draw a pulsing selection rectangle around the active drawing area (draw tool)
    if (is_drawing_ && (current_tool_ == RoomTool::Draw || current_tool_ == RoomTool::Line))
    {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 mouse_room = ScreenToRoom(mouse_pos, origin);

        ImVec2 s0 = RoomToScreen(
            std::min(draw_start_room_.x, mouse_room.x),
            std::min(draw_start_room_.y, mouse_room.y), origin);
        ImVec2 s1 = RoomToScreen(
            std::max(draw_start_room_.x, mouse_room.x),
            std::max(draw_start_room_.y, mouse_room.y), origin);

        if (current_tool_ == RoomTool::Draw)
        {
            dl->AddRectFilled(s0, s1, IM_COL32(255, 255, 100, 40));
            dl->AddRect(s0, s1, IM_COL32(255, 255, 100, 200), 0, 0, 1.5f);
        }
        else // Line
        {
            ImVec2 line_start = RoomToScreen(draw_start_room_.x, draw_start_room_.y, origin);
            dl->AddLine(line_start, mouse_pos, IM_COL32(255, 200, 50, 200), 2.0f);
        }
    }
}

void RoomEditor::DrawToolPreview(ImDrawList* dl, ImVec2 origin, ImVec2 mouse_room)
{
    if (!ImGui::IsItemHovered()) return;

    if (current_tool_ == RoomTool::Draw || current_tool_ == RoomTool::Erase || current_tool_ == RoomTool::Freehand)
    {
        // Show brush cursor
        float half = (float)brush_size_ * 0.5f;
        ImVec2 p0 = RoomToScreen(mouse_room.x - half, mouse_room.y - half, origin);
        ImVec2 p1 = RoomToScreen(mouse_room.x + half, mouse_room.y + half, origin);

        ImU32 col = (current_tool_ == RoomTool::Erase)
            ? IM_COL32(255, 100, 100, 120) : IM_COL32(100, 255, 100, 120);
        dl->AddRect(p0, p1, col, 0, 0, 1.5f);
    }
    else if (current_tool_ == RoomTool::Select)
    {
        // Crosshair cursor
        ImVec2 mp = ImGui::GetMousePos();
        dl->AddLine(ImVec2(mp.x - 8, mp.y), ImVec2(mp.x + 8, mp.y), IM_COL32(255, 255, 255, 100));
        dl->AddLine(ImVec2(mp.x, mp.y - 8), ImVec2(mp.x, mp.y + 8), IM_COL32(255, 255, 255, 100));
    }

    // Coordinate readout
    if (mouse_room.x >= 0 && mouse_room.y >= 0 &&
        mouse_room.x < room_data_->width && mouse_room.y < room_data_->height)
    {
        char coord[32];
        snprintf(coord, sizeof(coord), "(%d, %d)", (int)mouse_room.x, (int)mouse_room.y);
        ImVec2 mp = ImGui::GetMousePos();
        dl->AddText(ImVec2(mp.x + 14, mp.y + 2), IM_COL32(200, 200, 200, 200), coord);
    }
}

// ============================================================================
// Canvas Input Handling
// ============================================================================

void RoomEditor::HandleCanvasInput(ImVec2 origin, ImVec2 canvas_size)
{
    // Right-click drag to pan
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        scroll_x_ += delta.x;
        scroll_y_ += delta.y;
    }

    // Middle-click drag to pan
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        scroll_x_ += delta.x;
        scroll_y_ += delta.y;
    }

    // Space-bar + left-click drag to pan
    if (ImGui::IsItemActive() && ImGui::IsKeyDown(ImGuiKey_Space) &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        scroll_x_ += delta.x;
        scroll_y_ += delta.y;
    }

    // Mouse wheel to zoom (or Shift+Wheel for horizontal scroll)
    if (ImGui::IsItemHovered())
    {
        float wheel = ImGui::GetIO().MouseWheel;
        if (std::fabs(wheel) > 0.001f)
        {
            if (ImGui::GetIO().KeyShift)
            {
                // Shift+Wheel: horizontal scrolling
                scroll_x_ += wheel * 40.0f;
            }
            else
            {
                // Zoom toward mouse cursor
                ImVec2 mouse_pos = ImGui::GetMousePos();
                ImVec2 room_before = ScreenToRoom(mouse_pos, origin);

                zoom_ *= (wheel > 0) ? 1.1f : (1.0f / 1.1f);
                zoom_ = std::clamp(zoom_, 0.1f, 16.0f);

                // Adjust scroll to keep mouse position stable
                ImVec2 room_after_screen = RoomToScreen(room_before, origin);
                scroll_x_ += mouse_pos.x - room_after_screen.x;
                scroll_y_ += mouse_pos.y - room_after_screen.y;
            }
        }
    }

    // Left-click interactions depend on tool and layer
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImVec2 mouse_room = ScreenToRoom(mouse_pos, origin);

    // ---- Select tool ----
    if (current_tool_ == RoomTool::Select)
    {
        // Edge cursor feedback when hovering edges
        if (active_layer_ == Layer_Edges && room_data_ && !dragging_item_ && dragging_edge_ < 0)
        {
            float edge_thresh = Dpi(6) / zoom_;
            if (std::fabs(mouse_room.x - room_data_->left_edge) < edge_thresh ||
                std::fabs(mouse_room.x - room_data_->right_edge) < edge_thresh)
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            else if (std::fabs(mouse_room.y - room_data_->top_edge) < edge_thresh ||
                     std::fabs(mouse_room.y - room_data_->bottom_edge) < edge_thresh)
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }
        if (dragging_edge_ >= 0)
        {
            if (dragging_edge_ <= 1)
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            else
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            // Edge dragging detection (when edges layer is active)
            if (active_layer_ == Layer_Edges && room_data_)
            {
                float edge_thresh = Dpi(6) / zoom_; // pixel threshold in room coords
                dragging_edge_ = -1;

                if (std::fabs(mouse_room.x - room_data_->left_edge) < edge_thresh)
                { dragging_edge_ = 0; edge_drag_original_ = room_data_->left_edge; }
                else if (std::fabs(mouse_room.x - room_data_->right_edge) < edge_thresh)
                { dragging_edge_ = 1; edge_drag_original_ = room_data_->right_edge; }
                else if (std::fabs(mouse_room.y - room_data_->top_edge) < edge_thresh)
                { dragging_edge_ = 2; edge_drag_original_ = room_data_->top_edge; }
                else if (std::fabs(mouse_room.y - room_data_->bottom_edge) < edge_thresh)
                { dragging_edge_ = 3; edge_drag_original_ = room_data_->bottom_edge; }
            }

            // Object selection & drag start (uses actual sprite bounds)
            if (active_layer_ == Layer_Objects && room_data_)
            {
                selected_item_id_ = -1;
                auto* project = editor_.GetProject();
                auto* loader = project ? project->GetSpriteLoader() : nullptr;
                for (int i = (int)room_data_->objects.size() - 1; i >= 0; i--)
                {
                    auto& obj = room_data_->objects[i];
                    // Skip design-time hidden objects
                    if (!obj.design_visible) continue;
                    // Use actual sprite dimensions for hit-testing
                    float obj_w = 24.0f, obj_h = 24.0f;
                    if (loader && loader->IsOpen() && obj.sprite >= 0)
                    {
                        const auto* met = loader->GetMetrics(obj.sprite);
                        if (met && met->exists)
                        {
                            obj_w = (float)met->width;
                            obj_h = (float)met->height;
                        }
                    }
                    // AGS objects: (x, y) is the bottom-left corner
                    if (mouse_room.x >= obj.x && mouse_room.x <= obj.x + obj_w &&
                        mouse_room.y >= obj.y - obj_h && mouse_room.y <= obj.y)
                    {
                        selected_item_id_ = i;
                        // Double-click opens sprite chooser
                        if (ImGui::IsMouseDoubleClicked(0))
                        {
                            open_sprite_chooser_ = true;
                        }
                        else if (!obj.design_locked)
                        {
                            dragging_item_ = true;
                            drag_start_room_ = mouse_room;
                            drag_item_original_pos_ = ImVec2((float)obj.x, (float)obj.y);
                        }
                        break;
                    }
                }
            }

            // Character selection (uses actual sprite bounds)
            if (active_layer_ == Layer_Characters)
            {
                selected_item_id_ = -1;
                auto* project = editor_.GetProject();
                if (project && project->GetGameData())
                {
                    auto* gd = project->GetGameData();
                    auto& chars = gd->characters;
                    auto* loader = project->GetSpriteLoader();
                    for (int i = (int)chars.size() - 1; i >= 0; i--)
                    {
                        if (chars[i].room != room_number_) continue;
                        // Try to get sprite dimensions from normal_view
                        float half_w = 12.0f, spr_h = 24.0f;
                        auto* hit_view = (chars[i].normal_view >= 0) ?
                            gd->FindViewById(chars[i].normal_view) : nullptr;
                        if (loader && hit_view)
                        {
                            if (!hit_view->loops.empty() && !hit_view->loops[0].frames.empty())
                            {
                                int sprite_id = hit_view->loops[0].frames[0].sprite_id;
                                const auto* met = loader->GetMetrics(sprite_id);
                                if (met && met->exists)
                                {
                                    half_w = (float)met->width * 0.5f;
                                    spr_h = (float)met->height;
                                }
                            }
                        }
                        // Characters: feet at (x,y), sprite centered above
                        if (mouse_room.x >= chars[i].x - half_w &&
                            mouse_room.x <= chars[i].x + half_w &&
                            mouse_room.y >= chars[i].y - spr_h &&
                            mouse_room.y <= chars[i].y)
                        {
                            selected_item_id_ = i;
                            dragging_item_ = true;
                            drag_start_room_ = mouse_room;
                            drag_item_original_pos_ = ImVec2((float)chars[i].x, (float)chars[i].y);
                            break;
                        }
                    }
                }
            }

            // Hotspot/WalkArea/WalkBehind/Region selection by reading mask pixel
            if (active_layer_ == Layer_Hotspots || active_layer_ == Layer_WalkAreas ||
                active_layer_ == Layer_WalkBehinds || active_layer_ == Layer_Regions)
            {
                AGS::Common::Bitmap* mask = GetActiveMask();
                if (mask && current_tool_ == RoomTool::Select)
                {
                    int mr = room_data_->mask_resolution;
                    int mx = (int)(mouse_room.x) / mr;
                    int my = (int)(mouse_room.y) / mr;
                    if (mx >= 0 && mx < mask->GetWidth() && my >= 0 && my < mask->GetHeight())
                    {
                        int area_id = mask->GetPixel(mx, my);
                        selected_item_id_ = (area_id > 0) ? area_id : -1;
                    }
                    else
                    {
                        selected_item_id_ = -1;
                    }
                }
            }
        }

        // Drag objects
        if (dragging_item_ && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 delta_room(mouse_room.x - drag_start_room_.x, mouse_room.y - drag_start_room_.y);

            int drag_x = 0, drag_y = 0;
            if (active_layer_ == Layer_Objects && selected_item_id_ >= 0 &&
                selected_item_id_ < (int)room_data_->objects.size())
            {
                room_data_->objects[selected_item_id_].x = (int)(drag_item_original_pos_.x + delta_room.x);
                room_data_->objects[selected_item_id_].y = (int)(drag_item_original_pos_.y + delta_room.y);
                drag_x = room_data_->objects[selected_item_id_].x;
                drag_y = room_data_->objects[selected_item_id_].y;
            }
            else if (active_layer_ == Layer_Characters && selected_item_id_ >= 0)
            {
                auto* project = editor_.GetProject();
                if (project && project->GetGameData())
                {
                    auto& chars = project->GetGameData()->characters;
                    if (selected_item_id_ < (int)chars.size())
                    {
                        chars[selected_item_id_].x = (int)(drag_item_original_pos_.x + delta_room.x);
                        chars[selected_item_id_].y = (int)(drag_item_original_pos_.y + delta_room.y);
                        drag_x = chars[selected_item_id_].x;
                        drag_y = chars[selected_item_id_].y;
                    }
                }
            }

            // Coordinate overlay while dragging
            {
                char coord_buf[64];
                snprintf(coord_buf, sizeof(coord_buf), "(%d, %d)", drag_x, drag_y);
                ImVec2 text_pos(mouse_pos.x + Dpi(12), mouse_pos.y - Dpi(16));
                ImDrawList* fg_dl = ImGui::GetForegroundDrawList();
                ImVec2 text_size = ImGui::CalcTextSize(coord_buf);
                fg_dl->AddRectFilled(
                    ImVec2(text_pos.x - 2, text_pos.y - 2),
                    ImVec2(text_pos.x + text_size.x + 4, text_pos.y + text_size.y + 2),
                    IM_COL32(0, 0, 0, 180), 3.0f);
                fg_dl->AddText(text_pos, IM_COL32(255, 255, 100, 255), coord_buf);
            }
        }

        // Release drag — push undo
        if (dragging_item_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            if (active_layer_ == Layer_Objects && selected_item_id_ >= 0 &&
                selected_item_id_ < (int)room_data_->objects.size())
            {
                int idx = selected_item_id_;
                int old_x = (int)drag_item_original_pos_.x;
                int old_y = (int)drag_item_original_pos_.y;
                int new_x = room_data_->objects[idx].x;
                int new_y = room_data_->objects[idx].y;
                if (old_x != new_x || old_y != new_y)
                {
                    PushUndo("Move object",
                        [this, idx, old_x, old_y]() { room_data_->objects[idx].x = old_x; room_data_->objects[idx].y = old_y; },
                        [this, idx, new_x, new_y]() { room_data_->objects[idx].x = new_x; room_data_->objects[idx].y = new_y; });
                }
            }
            dragging_item_ = false;
        }

        // Edge dragging
        if (dragging_edge_ >= 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            int val;
            if (dragging_edge_ == 0 || dragging_edge_ == 1)
                val = (int)mouse_room.x;
            else
                val = (int)mouse_room.y;

            val = std::clamp(val, 0, (dragging_edge_ <= 1) ? room_data_->width : room_data_->height);

            switch (dragging_edge_)
            {
                case 0: room_data_->left_edge = val; break;
                case 1: room_data_->right_edge = val; break;
                case 2: room_data_->top_edge = val; break;
                case 3: room_data_->bottom_edge = val; break;
            }
        }

        if (dragging_edge_ >= 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            int old_val = edge_drag_original_;
            int new_val;
            int edge = dragging_edge_;
            switch (edge)
            {
                case 0: new_val = room_data_->left_edge; break;
                case 1: new_val = room_data_->right_edge; break;
                case 2: new_val = room_data_->top_edge; break;
                default: new_val = room_data_->bottom_edge; break;
            }

            if (old_val != new_val)
            {
                PushUndo("Move edge",
                    [this, edge, old_val]() {
                        switch(edge) {
                            case 0: room_data_->left_edge = old_val; break;
                            case 1: room_data_->right_edge = old_val; break;
                            case 2: room_data_->top_edge = old_val; break;
                            case 3: room_data_->bottom_edge = old_val; break;
                        }
                    },
                    [this, edge, new_val]() {
                        switch(edge) {
                            case 0: room_data_->left_edge = new_val; break;
                            case 1: room_data_->right_edge = new_val; break;
                            case 2: room_data_->top_edge = new_val; break;
                            case 3: room_data_->bottom_edge = new_val; break;
                        }
                    });
            }
            dragging_edge_ = -1;
        }
    }

    // ---- Draw / Erase / Line / Fill tools ----
    if (current_tool_ == RoomTool::Draw || current_tool_ == RoomTool::Line)
    {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            is_drawing_ = true;
            draw_start_room_ = mouse_room;
            // Capture mask snapshot for undo before drawing
            auto* mask = GetActiveMask();
            if (mask)
            {
                mask_undo_snapshot_.reset(AGS::Common::BitmapHelper::CreateBitmapCopy(mask));
                mask_undo_layer_ = active_layer_;
            }
        }
        if (is_drawing_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            is_drawing_ = false;
            auto* mask = GetActiveMask();
            if (mask && selected_item_id_ >= 0)
            {
                int mr = std::max(1, room_data_->mask_resolution);
                int x0 = std::clamp((int)(std::min(draw_start_room_.x, mouse_room.x) / mr), 0, mask->GetWidth() - 1);
                int y0 = std::clamp((int)(std::min(draw_start_room_.y, mouse_room.y) / mr), 0, mask->GetHeight() - 1);
                int x1 = std::clamp((int)(std::max(draw_start_room_.x, mouse_room.x) / mr), 0, mask->GetWidth() - 1);
                int y1 = std::clamp((int)(std::max(draw_start_room_.y, mouse_room.y) / mr), 0, mask->GetHeight() - 1);

                if (current_tool_ == RoomTool::Draw)
                {
                    // Apply brush size
                    int half = brush_size_ / 2;
                    x0 = std::max(0, x0 - half);
                    y0 = std::max(0, y0 - half);
                    x1 = std::min(mask->GetWidth() - 1, x1 + half);
                    y1 = std::min(mask->GetHeight() - 1, y1 + half);
                    mask->FillRect(Rect(x0, y0, x1, y1), selected_item_id_);
                }
                else // Line
                {
                    mask->DrawLine(Line(x0, y0, x1, y1), selected_item_id_);
                }

                // Push mask undo
                if (mask_undo_snapshot_)
                {
                    auto snapshot = mask_undo_snapshot_;
                    int layer = mask_undo_layer_;
                    auto after = std::shared_ptr<AGS::Common::Bitmap>(
                        AGS::Common::BitmapHelper::CreateBitmapCopy(mask));
                    PushUndo("Paint mask",
                        [this, snapshot, layer]() {
                            auto* m = GetActiveMaskForLayer(layer);
                            if (m) m->Blit(snapshot.get(), 0, 0, 0, 0, m->GetWidth(), m->GetHeight());
                        },
                        [this, after, layer]() {
                            auto* m = GetActiveMaskForLayer(layer);
                            if (m) m->Blit(after.get(), 0, 0, 0, 0, m->GetWidth(), m->GetHeight());
                        });
                    mask_undo_snapshot_.reset();
                }

                editor_.GetLogPanel().AddLog("[Room] Painted %s at (%d,%d)-(%d,%d) area %d on %s",
                    current_tool_ == RoomTool::Draw ? "rect" : "line",
                    x0, y0, x1, y1, selected_item_id_, kLayerNames[active_layer_]);
            }
            else
            {
                mask_undo_snapshot_.reset();
                editor_.GetLogPanel().AddLog("[Room] No mask data available for %s layer.", kLayerNames[active_layer_]);
            }
        }
    }

    // Freehand tool — paint continuously while dragging (like erase but uses selected_item_id_)
    if (current_tool_ == RoomTool::Freehand)
    {
        // Capture snapshot on drag start
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            auto* mask = GetActiveMask();
            if (mask)
            {
                mask_undo_snapshot_.reset(AGS::Common::BitmapHelper::CreateBitmapCopy(mask));
                mask_undo_layer_ = active_layer_;
            }
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            auto* mask = GetActiveMask();
            if (mask && selected_item_id_ >= 0)
            {
                int mr = std::max(1, room_data_->mask_resolution);
                int half = std::max(1, brush_size_ / 2);
                int cx = std::clamp((int)(mouse_room.x / mr), 0, mask->GetWidth() - 1);
                int cy = std::clamp((int)(mouse_room.y / mr), 0, mask->GetHeight() - 1);
                int fx0 = std::max(0, cx - half);
                int fy0 = std::max(0, cy - half);
                int fx1 = std::min(mask->GetWidth() - 1, cx + half);
                int fy1 = std::min(mask->GetHeight() - 1, cy + half);
                mask->FillRect(Rect(fx0, fy0, fx1, fy1), selected_item_id_);
            }
        }
        // Push undo on release
        if (mask_undo_snapshot_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            auto* mask = GetActiveMask();
            if (mask)
            {
                auto snapshot = mask_undo_snapshot_;
                int layer = mask_undo_layer_;
                auto after = std::shared_ptr<AGS::Common::Bitmap>(
                    AGS::Common::BitmapHelper::CreateBitmapCopy(mask));
                PushUndo("Freehand paint",
                    [this, snapshot, layer]() {
                        auto* m = GetActiveMaskForLayer(layer);
                        if (m) m->Blit(snapshot.get(), 0, 0, 0, 0, m->GetWidth(), m->GetHeight());
                    },
                    [this, after, layer]() {
                        auto* m = GetActiveMaskForLayer(layer);
                        if (m) m->Blit(after.get(), 0, 0, 0, 0, m->GetWidth(), m->GetHeight());
                    });
            }
            mask_undo_snapshot_.reset();
        }
    }

    // Erase tool — paint area ID 0 continuously while dragging
    if (current_tool_ == RoomTool::Erase)
    {
        // Capture snapshot on drag start
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            auto* mask = GetActiveMask();
            if (mask)
            {
                mask_undo_snapshot_.reset(AGS::Common::BitmapHelper::CreateBitmapCopy(mask));
                mask_undo_layer_ = active_layer_;
            }
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            auto* mask = GetActiveMask();
            if (mask)
            {
                int mr = std::max(1, room_data_->mask_resolution);
                int half = std::max(1, brush_size_ / 2);
                int cx = std::clamp((int)(mouse_room.x / mr), 0, mask->GetWidth() - 1);
                int cy = std::clamp((int)(mouse_room.y / mr), 0, mask->GetHeight() - 1);
                int ex0 = std::max(0, cx - half);
                int ey0 = std::max(0, cy - half);
                int ex1 = std::min(mask->GetWidth() - 1, cx + half);
                int ey1 = std::min(mask->GetHeight() - 1, cy + half);
                mask->FillRect(Rect(ex0, ey0, ex1, ey1), 0);
            }
        }
        // Push undo on release
        if (mask_undo_snapshot_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            auto* mask = GetActiveMask();
            if (mask)
            {
                auto snapshot = mask_undo_snapshot_;
                int layer = mask_undo_layer_;
                auto after = std::shared_ptr<AGS::Common::Bitmap>(
                    AGS::Common::BitmapHelper::CreateBitmapCopy(mask));
                PushUndo("Erase mask",
                    [this, snapshot, layer]() {
                        auto* m = GetActiveMaskForLayer(layer);
                        if (m) m->Blit(snapshot.get(), 0, 0, 0, 0, m->GetWidth(), m->GetHeight());
                    },
                    [this, after, layer]() {
                        auto* m = GetActiveMaskForLayer(layer);
                        if (m) m->Blit(after.get(), 0, 0, 0, 0, m->GetWidth(), m->GetHeight());
                    });
            }
            mask_undo_snapshot_.reset();
        }
    }

    // Fill tool — flood fill on click
    if (current_tool_ == RoomTool::Fill)
    {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            auto* mask = GetActiveMask();
            if (mask && selected_item_id_ >= 0)
            {
                // Capture snapshot before flood fill
                auto snapshot = std::shared_ptr<AGS::Common::Bitmap>(
                    AGS::Common::BitmapHelper::CreateBitmapCopy(mask));
                int layer = active_layer_;

                int mr = std::max(1, room_data_->mask_resolution);
                int mx = std::clamp((int)(mouse_room.x / mr), 0, mask->GetWidth() - 1);
                int my = std::clamp((int)(mouse_room.y / mr), 0, mask->GetHeight() - 1);
                mask->FloodFill(mx, my, selected_item_id_);

                // Push undo after flood fill
                auto after = std::shared_ptr<AGS::Common::Bitmap>(
                    AGS::Common::BitmapHelper::CreateBitmapCopy(mask));
                PushUndo("Flood fill",
                    [this, snapshot, layer]() {
                        auto* m = GetActiveMaskForLayer(layer);
                        if (m) m->Blit(snapshot.get(), 0, 0, 0, 0, m->GetWidth(), m->GetHeight());
                    },
                    [this, after, layer]() {
                        auto* m = GetActiveMaskForLayer(layer);
                        if (m) m->Blit(after.get(), 0, 0, 0, 0, m->GetWidth(), m->GetHeight());
                    });

                editor_.GetLogPanel().AddLog("[Room] Flood fill at (%d,%d) area %d on %s",
                    mx, my, selected_item_id_, kLayerNames[active_layer_]);
            }
        }
    }

    // Keyboard shortcuts
    auto& io = ImGui::GetIO();

    // Arrow key movement for selected objects/characters
    if (current_tool_ == RoomTool::Select && selected_item_id_ >= 0 && !dragging_item_ && !is_drawing_)
    {
        int dx = 0, dy = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  dx = -1;
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) dx = 1;
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))    dy = -1;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))  dy = 1;

        if (dx != 0 || dy != 0)
        {
            if (active_layer_ == Layer_Objects && room_data_ &&
                selected_item_id_ < (int)room_data_->objects.size())
            {
                auto& obj = room_data_->objects[selected_item_id_];
                if (!obj.design_locked)
                {
                    int old_x = obj.x, old_y = obj.y;
                    obj.x += dx; obj.y += dy;
                    int idx = selected_item_id_;
                    int nx = obj.x, ny = obj.y;
                    PushUndo("Move object",
                        [this, idx, old_x, old_y]() { room_data_->objects[idx].x = old_x; room_data_->objects[idx].y = old_y; },
                        [this, idx, nx, ny]() { room_data_->objects[idx].x = nx; room_data_->objects[idx].y = ny; });
                }
            }
            else if (active_layer_ == Layer_Characters)
            {
                auto* project = editor_.GetProject();
                if (project && project->GetGameData())
                {
                    auto& chars = project->GetGameData()->characters;
                    if (selected_item_id_ < (int)chars.size())
                    {
                        chars[selected_item_id_].x += dx;
                        chars[selected_item_id_].y += dy;
                    }
                }
            }
        }
    }

    // Context menu on right-click
    if (ImGui::BeginPopupContextItem("##RoomCanvasCtx"))
    {
        ImVec2 ctx_mouse = ImGui::GetMousePosOnOpeningCurrentPopup();
        ImVec2 ctx_room = ScreenToRoom(ctx_mouse, origin);

        ImGui::Text("Room (%d, %d)", (int)ctx_room.x, (int)ctx_room.y);
        ImGui::Separator();

        if (active_layer_ == Layer_Objects && room_data_)
        {
            if (ImGui::MenuItem("Place New Object Here"))
            {
                RoomData::ObjectData new_obj;
                new_obj.id = (int)room_data_->objects.size();
                new_obj.x = (int)ctx_room.x;
                new_obj.y = (int)ctx_room.y;
                new_obj.sprite = 0;
                new_obj.name = "NewObject";
                room_data_->objects.push_back(new_obj);
                selected_item_id_ = (int)room_data_->objects.size() - 1;
                editor_.GetLogPanel().AddLog("[Room] Placed new object at (%d, %d)",
                    new_obj.x, new_obj.y);
            }
            if (selected_item_id_ >= 0 && selected_item_id_ < (int)room_data_->objects.size())
            {
                auto& obj = room_data_->objects[selected_item_id_];
                ImGui::Separator();
                ImGui::Text("Object %d: %s", selected_item_id_, obj.name.c_str());
                if (ImGui::MenuItem("Copy Coordinates"))
                {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "%d, %d", obj.x, obj.y);
                    ImGui::SetClipboardText(buf);
                }
                if (ImGui::MenuItem("Delete Object"))
                {
                    room_data_->objects.erase(room_data_->objects.begin() + selected_item_id_);
                    selected_item_id_ = -1;
                    editor_.GetLogPanel().AddLog("[Room] Deleted object");
                }
            }
        }
        else if (active_layer_ == Layer_Characters)
        {
            auto* project = editor_.GetProject();
            if (project && project->GetGameData() && selected_item_id_ >= 0)
            {
                auto& chars = project->GetGameData()->characters;
                if (selected_item_id_ < (int)chars.size())
                {
                    ImGui::Text("Character: %s", chars[selected_item_id_].real_name.c_str());
                    if (ImGui::MenuItem("Copy Coordinates"))
                    {
                        char buf[64];
                        snprintf(buf, sizeof(buf), "%d, %d", chars[selected_item_id_].x, chars[selected_item_id_].y);
                        ImGui::SetClipboardText(buf);
                    }
                }
            }
        }
        else
        {
            if (ImGui::MenuItem("Copy Coordinates"))
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "%d, %d", (int)ctx_room.x, (int)ctx_room.y);
                ImGui::SetClipboardText(buf);
            }
        }

        ImGui::EndPopup();
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift)
        Undo();
    if (io.KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))))
        Redo();
}

// ============================================================================
// Layer panel (sidebar)
// ============================================================================

void RoomEditor::DrawLayerPanel()
{
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Layers");
    ImGui::Separator();

    for (int i = 0; i < Layer_COUNT; i++)
    {
        ImGui::PushID(i);

        // Visibility checkbox
        ImGui::Checkbox("##vis", &layer_visible_[i]);
        ImGui::SameLine();

        // Selectable layer name
        bool is_active = (active_layer_ == i);
        if (ImGui::Selectable(kLayerNames[i], is_active))
        {
            active_layer_ = i;
            selected_item_id_ = -1; // Reset selection when switching layers
        }

        ImGui::PopID();
    }
}

// ============================================================================
// Item list for current layer
// ============================================================================

void RoomEditor::DrawItemList()
{
    if (!room_data_) return;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s List", kLayerNames[active_layer_]);
    ImGui::Separator();

    float list_height = Dpi(150);
    ImGui::BeginChild("##ItemList", ImVec2(0, list_height), ImGuiChildFlags_Borders);

    auto draw_selectable = [&](int id, const char* label) {
        bool sel = (selected_item_id_ == id);
        if (ImGui::Selectable(label, sel))
            selected_item_id_ = id;
    };

    switch (active_layer_)
    {
    case Layer_Hotspots:
        for (int i = 1; i < (int)room_data_->hotspots.size(); i++)
        {
            char label[80];
            auto& hs = room_data_->hotspots[i];
            snprintf(label, sizeof(label), "%d: %s", i,
                     hs.name.empty() ? "(unnamed)" : hs.name.c_str());
            draw_selectable(i, label);
        }
        break;

    case Layer_Objects:
        for (int i = 0; i < (int)room_data_->objects.size(); i++)
        {
            char label[80];
            auto& obj = room_data_->objects[i];
            snprintf(label, sizeof(label), "%d: %s (%d,%d)",
                     i, obj.name.empty() ? obj.script_name.c_str() : obj.name.c_str(),
                     obj.x, obj.y);
            draw_selectable(i, label);
        }
        break;

    case Layer_WalkAreas:
        for (int i = 1; i < (int)room_data_->walk_areas.size(); i++)
        {
            char label[80];
            snprintf(label, sizeof(label), "Walk Area %d (scale %d%%-%d%%)",
                     i, room_data_->walk_areas[i].scaling_min,
                     room_data_->walk_areas[i].scaling_max);
            draw_selectable(i, label);
        }
        break;

    case Layer_WalkBehinds:
        for (int i = 1; i < (int)room_data_->walk_behinds.size(); i++)
        {
            char label[80];
            snprintf(label, sizeof(label), "Walk-behind %d (baseline: %d)",
                     i, room_data_->walk_behinds[i].baseline);
            draw_selectable(i, label);
        }
        break;

    case Layer_Regions:
        for (int i = 1; i < (int)room_data_->regions.size(); i++)
        {
            char label[80];
            auto& r = room_data_->regions[i];
            snprintf(label, sizeof(label), "Region %d (light: %d, tint: %d,%d,%d)",
                     i, r.light_level, r.tint_r, r.tint_g, r.tint_b);
            draw_selectable(i, label);
        }
        break;

    case Layer_Characters:
    {
        auto* project = editor_.GetProject();
        if (project && project->GetGameData())
        {
            auto& chars = project->GetGameData()->characters;
            for (int i = 0; i < (int)chars.size(); i++)
            {
                if (chars[i].room != room_number_) continue;
                char label[80];
                snprintf(label, sizeof(label), "%s (%d,%d)",
                         chars[i].script_name.c_str(), chars[i].x, chars[i].y);
                draw_selectable(i, label);
            }
        }
        break;
    }

    case Layer_Edges:
        ImGui::Text("Left: %d", room_data_->left_edge);
        ImGui::Text("Right: %d", room_data_->right_edge);
        ImGui::Text("Top: %d", room_data_->top_edge);
        ImGui::Text("Bottom: %d", room_data_->bottom_edge);
        break;

    default:
        ImGui::TextDisabled("(no items)");
        break;
    }

    ImGui::EndChild();
}

// ============================================================================
// Properties panel for selected item
// ============================================================================

void RoomEditor::DrawPropertiesPanel()
{
    if (!room_data_) return;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Properties");
    ImGui::Separator();

    ImGui::BeginChild("##Props", ImVec2(0, 0), ImGuiChildFlags_Borders);

    // Room-level properties (always shown)
    if (ImGui::CollapsingHeader("Room", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Number: %d", room_data_->number);
        ImGui::SameLine();
        if (ImGui::SmallButton("Change..."))
        {
            change_number_new_ = room_data_->number;
            open_change_number_popup_ = true;
        }

        // Change Room Number popup
        if (open_change_number_popup_)
        {
            ImGui::OpenPopup("Change Room Number");
            open_change_number_popup_ = false;
        }
        if (ImGui::BeginPopupModal("Change Room Number", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter new room number (0-999):");
            ImGui::InputInt("##NewRoomNum", &change_number_new_);
            change_number_new_ = std::clamp(change_number_new_, 0, 999);

            // Validate
            std::string error_msg;
            if (change_number_new_ == room_data_->number)
            {
                error_msg = "Same as current number.";
            }
            else
            {
                auto* project = editor_.GetProject();
                if (project && project->GetGameData())
                {
                    for (auto& ri : project->GetGameData()->rooms)
                    {
                        if (ri.number == change_number_new_)
                        {
                            error_msg = "Room " + std::to_string(change_number_new_) + " already exists.";
                            break;
                        }
                    }
                }
            }

            if (!error_msg.empty())
            {
                ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", error_msg.c_str());
            }

            bool can_apply = error_msg.empty() && change_number_new_ != room_data_->number;
            if (!can_apply) ImGui::BeginDisabled();
            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                int old_num = room_number_;
                int new_num = change_number_new_;
                auto* project = editor_.GetProject();
                std::string proj_dir;
                if (project) proj_dir = project->GetProjectDir();

                // 1. Save room data before renaming
                SaveRoomData();

                // 2. Close any open script editor panes for old room files
                char old_script[64], old_header[64];
                snprintf(old_script, sizeof(old_script), "room%d.asc", old_num);
                snprintf(old_header, sizeof(old_header), "room%d.ash", old_num);
                editor_.ClosePaneByScriptFilename(old_script);
                editor_.ClosePaneByScriptFilename(old_header);

                // 3. Rename files on disk (.crm, .asc, .ash)
                namespace fs = std::filesystem;
                bool rename_ok = true;
                struct FilePair { std::string from, to; };
                std::vector<FilePair> files_to_rename;

                const char* exts[] = { ".crm", ".asc", ".ash" };
                for (auto ext : exts)
                {
                    char from_name[64], to_name[64];
                    snprintf(from_name, sizeof(from_name), "room%d%s", old_num, ext);
                    snprintf(to_name, sizeof(to_name), "room%d%s", new_num, ext);
                    std::string from_path = proj_dir + "/" + from_name;
                    std::string to_path = proj_dir + "/" + to_name;

                    if (fs::exists(from_path))
                    {
                        if (fs::exists(to_path))
                        {
                            editor_.GetLogPanel().AddLog("[Room] Cannot rename: %s already exists.", to_path.c_str());
                            rename_ok = false;
                            break;
                        }
                        files_to_rename.push_back({from_path, to_path});
                    }
                }

                if (rename_ok)
                {
                    std::error_code ec;
                    for (auto& fp : files_to_rename)
                    {
                        fs::rename(fp.from, fp.to, ec);
                        if (ec)
                        {
                            editor_.GetLogPanel().AddLog("[Room] Failed to rename %s -> %s: %s",
                                fp.from.c_str(), fp.to.c_str(), ec.message().c_str());
                            rename_ok = false;
                            break;
                        }
                        editor_.GetLogPanel().AddLog("[Room] Renamed %s -> %s",
                            fp.from.c_str(), fp.to.c_str());
                    }
                }

                if (rename_ok)
                {
                    // 4. Update internal data structures
                    room_number_ = new_num;
                    room_data_->number = new_num;

                    // Update title
                    if (project && project->GetGameData())
                    {
                        auto& rooms = project->GetGameData()->rooms;
                        for (auto& ri : rooms)
                        {
                            if (ri.number == old_num)
                            {
                                ri.number = new_num;
                                char title_buf[256];
                                if (!ri.description.empty())
                                    snprintf(title_buf, sizeof(title_buf), "Room %d: %s", new_num, ri.description.c_str());
                                else
                                    snprintf(title_buf, sizeof(title_buf), "Room %d", new_num);
                                title_ = title_buf;
                                break;
                            }
                        }

                        // Sort rooms by number
                        std::sort(rooms.begin(), rooms.end(),
                            [](const GameData::RoomInfo& a, const GameData::RoomInfo& b) {
                                return a.number < b.number;
                            });
                    }

                    // 5. Rebuild project tree
                    editor_.NotifyProjectLoaded();

                    editor_.GetLogPanel().AddLog("[Room] Room number changed from %d to %d.", old_num, new_num);
                }

                ImGui::CloseCurrentPopup();
            }
            if (!can_apply) ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Mask resolution change popup
        if (show_mask_res_popup_)
        {
            ImGui::OpenPopup("Change Mask Resolution");
            show_mask_res_popup_ = false;
        }
        if (ImGui::BeginPopupModal("Change Mask Resolution", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Mask resolution determines the precision of\nhotspot, walkable area, and region masks.");
            ImGui::Text("Current: 1:%d", room_data_->mask_resolution);
            ImGui::Spacing();

            ImGui::Text("New resolution:");
            ImGui::Combo("##MaskResCombo", &pending_mask_res_,
                "1:1 (Full resolution)\0"
                "1:2 (Half resolution)\0"
                "1:3 (Third resolution)\0"
                "1:4 (Quarter resolution)\0");
            int new_res = pending_mask_res_ + 1; // Combo index 0-3 maps to 1-4

            if (new_res > room_data_->mask_resolution)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                    "Warning: Increasing the mask divisor will reduce\n"
                    "mask precision. Some area detail may be lost.");
            }

            bool same = (new_res == room_data_->mask_resolution);
            if (same) ImGui::BeginDisabled();
            if (ImGui::Button("OK", ImVec2(Dpi(120), 0)))
            {
                int old_res = room_data_->mask_resolution;
                room_data_->mask_resolution = new_res;

                // Rescale masks using BitmapHelper::AdjustBitmapSize
                int base_w = room_data_->width;
                int base_h = room_data_->height;
                int new_w = base_w / new_res;
                int new_h = base_h / new_res;

                auto RescaleMask = [&](std::shared_ptr<AGS::Common::Bitmap>& mask, int tw, int th)
                {
                    if (!mask) return;
                    using namespace AGS::Common;
                    Bitmap* resized = BitmapHelper::AdjustBitmapSize(mask.get(), tw, th);
                    if (resized != mask.get())
                        mask.reset(resized);
                };

                // Hotspot, walkarea, region masks use mask_resolution
                RescaleMask(room_data_->hotspot_mask, new_w, new_h);
                RescaleMask(room_data_->walkarea_mask, new_w, new_h);
                RescaleMask(room_data_->region_mask, new_w, new_h);
                // Walk-behinds are always 1:1 with room size
                RescaleMask(room_data_->walkbehind_mask, base_w, base_h);

                editor_.GetLogPanel().AddLog(
                    "[Room] Mask resolution changed from 1:%d to 1:%d (masks rescaled to %dx%d).",
                    old_res, new_res, new_w, new_h);

                ImGui::CloseCurrentPopup();
            }
            if (same) ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        // Room description (from project data, not room file)
        auto* project = editor_.GetProject();
        if (project && project->GetGameData())
        {
            auto& rooms = project->GetGameData()->rooms;
            for (auto& ri : rooms)
            {
                if (ri.number == room_data_->number)
                {
                    char desc_buf[256];
                    strncpy(desc_buf, ri.description.c_str(), sizeof(desc_buf) - 1);
                    desc_buf[sizeof(desc_buf) - 1] = 0;
                    if (ImGui::InputText("Description", desc_buf, sizeof(desc_buf)))
                    {
                        ri.description = desc_buf;
                        // Update tab title to reflect new description
                        char title_buf[256];
                        if (!ri.description.empty())
                            snprintf(title_buf, sizeof(title_buf), "Room %d: %s", room_number_, ri.description.c_str());
                        else
                            snprintf(title_buf, sizeof(title_buf), "Room %d", room_number_);
                        title_ = title_buf;
                    }
                    break;
                }
            }
        }

        int w = room_data_->width, h = room_data_->height;
        if (ImGui::InputInt("Width", &w))
        {
            w = std::clamp(w, 64, 10000);
            if (w != room_data_->width)
            {
                int old_w = room_data_->width;
                PushUndo("Resize room width",
                    [this, old_w]() { room_data_->width = old_w; },
                    [this, w]() { room_data_->width = w; });
                room_data_->width = w;
            }
        }
        if (ImGui::InputInt("Height", &h))
        {
            h = std::clamp(h, 64, 10000);
            if (h != room_data_->height)
            {
                int old_h = room_data_->height;
                PushUndo("Resize room height",
                    [this, old_h]() { room_data_->height = old_h; },
                    [this, h]() { room_data_->height = h; });
                room_data_->height = h;
            }
        }
        ImGui::Text("Mask Resolution: 1:%d", room_data_->mask_resolution);
        ImGui::SameLine();
        if (ImGui::SmallButton("Change...##MaskRes"))
        {
            pending_mask_res_ = room_data_->mask_resolution - 1; // Combo index 0-3
            show_mask_res_popup_ = true;
        }

        // Background frame selector
        ImGui::Text("BG Frames: %d", room_data_->bg_frame_count);
        {
            const char* frame_labels[] = { "Main background", "Frame 2", "Frame 3", "Frame 4", "Frame 5" };
            int max_frames = std::max(1, std::min(room_data_->bg_frame_count, 5));
            ImGui::SetNextItemWidth(Dpi(160));
            ImGui::Combo("Show Frame", &selected_bg_frame_, frame_labels, max_frames);
        }

        // Background management buttons
        if (ImGui::Button("Change..."))
        {
            FileDialog::Open(FileDialogType::OpenFile,
                "Change Background Image",
                "Image files{.bmp,.png,.jpg,.jpeg,.gif,.tif,.tiff}",
                ".",
                [this](const std::string& path) {
                    auto* bmp = AGS::Common::BitmapHelper::LoadFromFile(path.c_str());
                    if (!bmp)
                    {
                        fprintf(stderr, "[RoomEditor] Failed to load image: %s\n", path.c_str());
                        return;
                    }
                    int idx = std::clamp(selected_bg_frame_, 0,
                        std::max(0, (int)room_data_->bg_frames.size() - 1));
                    if (idx == 0)
                    {
                        // Changing main bg may change room dimensions
                        room_data_->width = bmp->GetWidth();
                        room_data_->height = bmp->GetHeight();
                    }
                    else if (bmp->GetWidth() != room_data_->width ||
                             bmp->GetHeight() != room_data_->height)
                    {
                        fprintf(stderr, "[RoomEditor] Background frame must match main background size (%dx%d)\n",
                                room_data_->width, room_data_->height);
                        delete bmp;
                        return;
                    }
                    // Validate color depth for non-primary frames
                    if (idx > 0 && bmp->GetBPP() != room_data_->bg_bpp)
                    {
                        fprintf(stderr, "[RoomEditor] Background frame color depth (%d-bit) must match main background (%d-bit)\n",
                                bmp->GetColorDepth(), room_data_->bg_bpp * 8);
                        delete bmp;
                        return;
                    }
                    auto shared_bmp = std::shared_ptr<AGS::Common::Bitmap>(bmp);
                    if (idx < (int)room_data_->bg_frames.size())
                        room_data_->bg_frames[idx] = shared_bmp;
                    // Determine color depth in bytes
                    int bpp = bmp->GetBPP();
                    if (idx == 0) room_data_->bg_bpp = bpp;
                    // Invalidate cached texture
                    auto& tex_cache = editor_.GetApp().GetTextureCache();
                    char bg_key[64];
                    snprintf(bg_key, sizeof(bg_key), "room%d_bg%d", room_number_, idx);
                    tex_cache.Evict(bg_key);
                    fprintf(stderr, "[RoomEditor] Changed background frame %d from: %s\n", idx, path.c_str());
                });
        }

        ImGui::SameLine();
        bool can_import = room_data_->bg_frame_count < 5;
        if (!can_import) ImGui::BeginDisabled();
        if (ImGui::Button("Import New..."))
        {
            FileDialog::Open(FileDialogType::OpenFile,
                "Import Background Frame",
                "Image files{.bmp,.png,.jpg,.jpeg,.gif,.tif,.tiff}",
                ".",
                [this](const std::string& path) {
                    auto* bmp = AGS::Common::BitmapHelper::LoadFromFile(path.c_str());
                    if (!bmp)
                    {
                        fprintf(stderr, "[RoomEditor] Failed to load image: %s\n", path.c_str());
                        return;
                    }
                    if (bmp->GetWidth() != room_data_->width ||
                        bmp->GetHeight() != room_data_->height)
                    {
                        fprintf(stderr, "[RoomEditor] Background frame must match main background size (%dx%d)\n",
                                room_data_->width, room_data_->height);
                        delete bmp;
                        return;
                    }
                    if (bmp->GetBPP() != room_data_->bg_bpp)
                    {
                        fprintf(stderr, "[RoomEditor] Background frame color depth (%d-bit) must match main background (%d-bit)\n",
                                bmp->GetColorDepth(), room_data_->bg_bpp * 8);
                        delete bmp;
                        return;
                    }
                    auto shared_bmp = std::shared_ptr<AGS::Common::Bitmap>(bmp);
                    room_data_->bg_frames.push_back(shared_bmp);
                    room_data_->bg_frame_count = (int)room_data_->bg_frames.size();
                    selected_bg_frame_ = room_data_->bg_frame_count - 1;
                    fprintf(stderr, "[RoomEditor] Imported background frame %d from: %s\n",
                            room_data_->bg_frame_count - 1, path.c_str());
                });
        }
        if (!can_import) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Export..."))
        {
            int idx = std::clamp(selected_bg_frame_, 0,
                std::max(0, (int)room_data_->bg_frames.size() - 1));
            if (idx < (int)room_data_->bg_frames.size() && room_data_->bg_frames[idx])
            {
                FileDialog::Open(FileDialogType::SaveFile,
                    "Export Background Frame",
                    "BMP files{.bmp}",
                    ".",
                    [this, idx](const std::string& path) {
                        if (idx < (int)room_data_->bg_frames.size() && room_data_->bg_frames[idx])
                        {
                            if (AGS::Common::BitmapHelper::SaveToFile(
                                    room_data_->bg_frames[idx].get(), path.c_str()))
                                fprintf(stderr, "[RoomEditor] Exported background frame %d to: %s\n", idx, path.c_str());
                            else
                                fprintf(stderr, "[RoomEditor] Failed to export background to: %s\n", path.c_str());
                        }
                    });
            }
        }

        ImGui::SameLine();
        bool can_delete = selected_bg_frame_ > 0 &&
            selected_bg_frame_ < (int)room_data_->bg_frames.size();
        if (!can_delete) ImGui::BeginDisabled();
        if (ImGui::Button("Delete Frame"))
        {
            int idx = selected_bg_frame_;
            room_data_->bg_frames.erase(room_data_->bg_frames.begin() + idx);
            room_data_->bg_frame_count = (int)room_data_->bg_frames.size();
            // Invalidate all bg textures from idx onward (indices shifted)
            auto& tex_cache = editor_.GetApp().GetTextureCache();
            for (int i = idx; i <= 4; i++)
            {
                char bg_key[64];
                snprintf(bg_key, sizeof(bg_key), "room%d_bg%d", room_number_, i);
                tex_cache.Evict(bg_key);
            }
            if (selected_bg_frame_ >= room_data_->bg_frame_count)
                selected_bg_frame_ = room_data_->bg_frame_count - 1;
            fprintf(stderr, "[RoomEditor] Deleted background frame %d\n", idx);
        }
        if (!can_delete) ImGui::EndDisabled();

        ImGui::Text("Messages: %d", room_data_->message_count);
    }

    // Room messages
    if (!room_data_->messages.empty() && ImGui::CollapsingHeader("Messages"))
    {
        for (auto& msg : room_data_->messages)
        {
            ImGui::PushID(msg.id);
            ImGui::Text("Message %d:", msg.id);
            char msg_buf[2048];
            strncpy(msg_buf, msg.text.c_str(), sizeof(msg_buf) - 1);
            msg_buf[sizeof(msg_buf) - 1] = 0;
            if (ImGui::InputTextMultiline("##msg", msg_buf, sizeof(msg_buf),
                    ImVec2(-1, ImGui::GetTextLineHeight() * 4)))
                msg.text = msg_buf;
            ImGui::InputInt("Display As", &msg.display_as);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("0 = standard window, >=1 = as character speech (character number)");
            ImGui::Checkbox("Auto-Remove", &msg.auto_remove);
            ImGui::Separator();
            ImGui::PopID();
        }
    }

    // Room custom properties
    if (ImGui::CollapsingHeader("Custom Properties"))
    {
        auto* project = editor_.GetProject();
        auto* gd = project ? project->GetGameData() : nullptr;
        if (gd && !gd->custom_property_schemas.empty())
        {
            DrawCustomPropertyValues(gd->custom_property_schemas,
                                     room_data_->custom_properties, "rooms");
        }
        else
        {
            ImGui::TextDisabled("No custom properties defined");
        }
    }

    // Room-level events
    {
        bool want_focus = focus_events_;
        focus_events_ = false;
        char room_script[64];
        snprintf(room_script, sizeof(room_script), "room%d.asc", room_data_->number);
        DrawEventsSection(
            room_data_->interactions,
            InteractionSchemas::Room(),
            "room",
            room_script,
            [this, room_script](const std::string&, const std::string&) {
                editor_.OpenScriptFile(room_script);
            },
            [this, room_script](const std::string&, const std::string&) {
                editor_.OpenScriptFile(room_script);
            },
            want_focus
        );
    }

    // Mask import/export (shown when a mask layer is active)
    if ((active_layer_ == Layer_Hotspots || active_layer_ == Layer_WalkAreas ||
         active_layer_ == Layer_WalkBehinds || active_layer_ == Layer_Regions) &&
        ImGui::CollapsingHeader("Mask Import/Export"))
    {
        auto* mask = GetActiveMask();
        const char* layer_name = kLayerNames[active_layer_];

        if (ImGui::Button("Export Mask to BMP"))
        {
            if (mask)
            {
                char default_name[128];
                snprintf(default_name, sizeof(default_name), "room%d_%s_mask.bmp",
                    room_number_, layer_name);
                int layer = active_layer_;
                FileDialog::Open(FileDialogType::SaveFile, "Export Mask",
                    ".bmp", ".",
                    [this, layer](const std::string& path) {
                        auto* m = GetActiveMaskForLayer(layer);
                        if (m && AGS::Common::BitmapHelper::SaveToFile(m, path.c_str()))
                            editor_.GetLogPanel().AddLog("[Room] Exported %s mask to: %s",
                                kLayerNames[layer], path.c_str());
                        else
                            editor_.GetLogPanel().AddLog("[Room] Failed to export mask to: %s", path.c_str());
                    });
            }
        }

        if (ImGui::Button("Import Mask from BMP"))
        {
            int layer = active_layer_;
            FileDialog::Open(FileDialogType::OpenFile, "Import Mask",
                ".bmp,.pcx", ".",
                [this, layer](const std::string& path) {
                    auto* target = GetActiveMaskForLayer(layer);
                    if (!target) return;
                    AGS::Common::Bitmap* imported =
                        AGS::Common::BitmapHelper::LoadFromFile(path.c_str());
                    if (!imported)
                    {
                        editor_.GetLogPanel().AddLog("[Room] Failed to load mask from: %s", path.c_str());
                        return;
                    }
                    if (imported->GetWidth() != target->GetWidth() ||
                        imported->GetHeight() != target->GetHeight())
                    {
                        editor_.GetLogPanel().AddLog(
                            "[Room] Mask size mismatch: imported %dx%d, expected %dx%d",
                            imported->GetWidth(), imported->GetHeight(),
                            target->GetWidth(), target->GetHeight());
                        delete imported;
                        return;
                    }
                    // Capture undo snapshot before import
                    auto snapshot = std::shared_ptr<AGS::Common::Bitmap>(
                        AGS::Common::BitmapHelper::CreateBitmapCopy(target));
                    // Copy imported bitmap data
                    target->Blit(imported, 0, 0, 0, 0, target->GetWidth(), target->GetHeight());
                    auto after = std::shared_ptr<AGS::Common::Bitmap>(
                        AGS::Common::BitmapHelper::CreateBitmapCopy(target));
                    PushUndo("Import mask",
                        [this, snapshot, layer]() {
                            auto* m = GetActiveMaskForLayer(layer);
                            if (m) m->Blit(snapshot.get(), 0, 0, 0, 0, m->GetWidth(), m->GetHeight());
                        },
                        [this, after, layer]() {
                            auto* m = GetActiveMaskForLayer(layer);
                            if (m) m->Blit(after.get(), 0, 0, 0, 0, m->GetWidth(), m->GetHeight());
                        });
                    delete imported;
                    editor_.GetLogPanel().AddLog("[Room] Imported %s mask from: %s",
                        kLayerNames[layer], path.c_str());
                });
        }
    }

    // Edge properties
    if (active_layer_ == Layer_Edges && ImGui::CollapsingHeader("Edges", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool changed = false;
        int le = room_data_->left_edge;
        int re = room_data_->right_edge;
        int te = room_data_->top_edge;
        int be = room_data_->bottom_edge;

        changed |= ImGui::InputInt("Left Edge", &le);
        changed |= ImGui::InputInt("Right Edge", &re);
        changed |= ImGui::InputInt("Top Edge", &te);
        changed |= ImGui::InputInt("Bottom Edge", &be);

        if (changed)
        {
            le = std::clamp(le, 0, room_data_->width);
            re = std::clamp(re, 0, room_data_->width);
            te = std::clamp(te, 0, room_data_->height);
            be = std::clamp(be, 0, room_data_->height);
            // Enforce left < right and top < bottom
            if (le >= re)
            {
                if (le == room_data_->width)
                    le = re - 1;
                else
                    re = le + 1;
            }
            if (te >= be)
            {
                if (te == room_data_->height)
                    te = be - 1;
                else
                    be = te + 1;
            }
            room_data_->left_edge = le;
            room_data_->right_edge = re;
            room_data_->top_edge = te;
            room_data_->bottom_edge = be;
        }
    }

    // Selected item properties
    if (selected_item_id_ >= 0)
    {
        switch (active_layer_)
        {
        case Layer_Hotspots:
            if (selected_item_id_ < (int)room_data_->hotspots.size())
            {
                auto& hs = room_data_->hotspots[selected_item_id_];
                if (ImGui::CollapsingHeader("Hotspot", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("ID: %d", hs.id);
                    char name_buf[256];
                    strncpy(name_buf, hs.name.c_str(), sizeof(name_buf) - 1);
                    name_buf[sizeof(name_buf) - 1] = 0;
                    if (ImGui::InputText("Name", name_buf, sizeof(name_buf)))
                        hs.name = name_buf;

                    char sname_buf[256];
                    strncpy(sname_buf, hs.script_name.c_str(), sizeof(sname_buf) - 1);
                    sname_buf[sizeof(sname_buf) - 1] = 0;
                    if (ImGui::InputText("Script Name", sname_buf, sizeof(sname_buf)))
                        hs.script_name = sname_buf;
                    {
                        std::string err = ValidateScriptNameFormat(hs.script_name);
                        if (!err.empty())
                            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", err.c_str());
                    }

                    char desc_buf[256];
                    strncpy(desc_buf, hs.description.c_str(), sizeof(desc_buf) - 1);
                    desc_buf[sizeof(desc_buf) - 1] = 0;
                    if (ImGui::InputText("Description", desc_buf, sizeof(desc_buf)))
                        hs.description = desc_buf;

                    ImGui::InputInt("Walk-to X", &hs.walk_to_x);
                    ImGui::InputInt("Walk-to Y", &hs.walk_to_y);
                }

                // Custom properties
                if (ImGui::CollapsingHeader("Custom Properties##hs"))
                {
                    auto* project = editor_.GetProject();
                    auto* gd = project ? project->GetGameData() : nullptr;
                    if (gd && !gd->custom_property_schemas.empty())
                        DrawCustomPropertyValues(gd->custom_property_schemas,
                                                 hs.custom_properties, "hotspots");
                    else
                        ImGui::TextDisabled("No custom properties defined");
                }

                // Events
                {
                    char room_script[64];
                    snprintf(room_script, sizeof(room_script), "room%d.asc", room_data_->number);
                    std::string script_name = hs.script_name.empty() ?
                        std::string("hHotspot") + std::to_string(hs.id) : hs.script_name;
                    DrawEventsSection(
                        hs.interactions,
                        InteractionSchemas::RoomHotspot(),
                        script_name,
                        room_script,
                        [this, room_script](const std::string&, const std::string&) {
                            editor_.OpenScriptFile(room_script);
                        },
                        [this, room_script](const std::string&, const std::string&) {
                            editor_.OpenScriptFile(room_script);
                        }
                    );
                }
            }
            break;

        case Layer_Objects:
            if (selected_item_id_ < (int)room_data_->objects.size())
            {
                auto& obj = room_data_->objects[selected_item_id_];
                if (ImGui::CollapsingHeader("Object", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("ID: %d", obj.id);
                    char name_buf[256];
                    strncpy(name_buf, obj.name.c_str(), sizeof(name_buf) - 1);
                    name_buf[sizeof(name_buf) - 1] = 0;
                    if (ImGui::InputText("Name", name_buf, sizeof(name_buf)))
                        obj.name = name_buf;

                    char sname_buf[256];
                    strncpy(sname_buf, obj.script_name.c_str(), sizeof(sname_buf) - 1);
                    sname_buf[sizeof(sname_buf) - 1] = 0;
                    if (ImGui::InputText("Script Name", sname_buf, sizeof(sname_buf)))
                        obj.script_name = sname_buf;
                    {
                        std::string err = ValidateScriptNameFormat(obj.script_name);
                        if (!err.empty())
                            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", err.c_str());
                    }

                    ImGui::InputInt("X", &obj.x);
                    ImGui::InputInt("Y", &obj.y);

                    // Sprite field with preview thumbnail and browse button
                    {
                        auto* project = editor_.GetProject();
                        auto* loader = project ? project->GetSpriteLoader() : nullptr;
                        auto& tex_cache = editor_.GetApp().GetTextureCache();
                        if (DrawSpriteField("Sprite", &obj.sprite, loader, tex_cache)
                            || open_sprite_chooser_)
                        {
                            ImGui::OpenPopup("Choose Sprite##ObjSpr");
                            open_sprite_chooser_ = false;
                        }

                        int chosen = obj.sprite;
                        if (DrawSpriteChooserPopup("Choose Sprite##ObjSpr", loader,
                                tex_cache, &chosen, obj.sprite))
                            obj.sprite = chosen;
                    }

                    ImGui::InputInt("Baseline", &obj.baseline);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("-1 = use object's Y position");
                    ImGui::Checkbox("Visible", &obj.visible);
                    ImGui::Checkbox("Clickable", &obj.clickable);

                    if (ImGui::CollapsingHeader("Design Time", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::Checkbox("Show in Editor", &obj.design_visible);
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Hide this object in the editor canvas\n(does not affect game)");
                        ImGui::Checkbox("Locked", &obj.design_locked);
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Prevent moving this object in the editor\n(does not affect game)");
                    }
                }

                // Custom properties
                if (ImGui::CollapsingHeader("Custom Properties##obj"))
                {
                    auto* project = editor_.GetProject();
                    auto* gd = project ? project->GetGameData() : nullptr;
                    if (gd && !gd->custom_property_schemas.empty())
                        DrawCustomPropertyValues(gd->custom_property_schemas,
                                                 obj.custom_properties, "objects");
                    else
                        ImGui::TextDisabled("No custom properties defined");
                }

                // Events
                {
                    char room_script[64];
                    snprintf(room_script, sizeof(room_script), "room%d.asc", room_data_->number);
                    std::string script_name = obj.script_name.empty() ?
                        std::string("oObject") + std::to_string(obj.id) : obj.script_name;
                    DrawEventsSection(
                        obj.interactions,
                        InteractionSchemas::RoomObject(),
                        script_name,
                        room_script,
                        [this, room_script](const std::string&, const std::string&) {
                            editor_.OpenScriptFile(room_script);
                        },
                        [this, room_script](const std::string&, const std::string&) {
                            editor_.OpenScriptFile(room_script);
                        }
                    );
                }
            }
            break;

        case Layer_WalkAreas:
            if (selected_item_id_ < (int)room_data_->walk_areas.size())
            {
                auto& wa = room_data_->walk_areas[selected_item_id_];
                if (ImGui::CollapsingHeader("Walk Area", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("ID: %d", wa.id);
                    ImGui::Checkbox("Continuous Scaling", &wa.continuous_scaling);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("When enabled, scaling interpolates between\nmin at Top and max at Bottom Y positions");
                    if (wa.continuous_scaling)
                    {
                        ImGui::SliderInt("Scale Near (Top)", &wa.scaling_min, 1, 200, "%d%%");
                        ImGui::SliderInt("Scale Far (Bottom)", &wa.scaling_max, 1, 200, "%d%%");
                        ImGui::InputInt("Top Y", &wa.top);
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Y position of nearest scaling point");
                        ImGui::InputInt("Bottom Y", &wa.bottom);
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Y position of farthest scaling point");
                        // Ensure top <= bottom
                        if (wa.top > wa.bottom)
                            wa.bottom = wa.top;
                    }
                    else
                    {
                        ImGui::SliderInt("Scaling", &wa.scaling_max, 0, 200, "%d%%");
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Uniform area scaling (0 = default game scaling)");
                    }
                }
            }
            break;

        case Layer_WalkBehinds:
            if (selected_item_id_ < (int)room_data_->walk_behinds.size())
            {
                auto& wb = room_data_->walk_behinds[selected_item_id_];
                if (ImGui::CollapsingHeader("Walk-behind", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("ID: %d", wb.id);
                    int old_baseline = wb.baseline;
                    if (ImGui::InputInt("Baseline", &wb.baseline))
                    {
                        wb.baseline = std::clamp(wb.baseline, 0, room_data_->height);
                        if (wb.baseline != old_baseline)
                        {
                            int idx = selected_item_id_;
                            PushUndo("Change baseline",
                                [this, idx, old_baseline]() { room_data_->walk_behinds[idx].baseline = old_baseline; },
                                [this, idx, v = wb.baseline]() { room_data_->walk_behinds[idx].baseline = v; });
                        }
                    }
                }
            }
            break;

        case Layer_Regions:
            if (selected_item_id_ < (int)room_data_->regions.size())
            {
                auto& reg = room_data_->regions[selected_item_id_];
                if (ImGui::CollapsingHeader("Region", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("ID: %d", reg.id);
                    ImGui::SliderInt("Light Level", &reg.light_level, -100, 100);

                    float tint[3] = { reg.tint_r / 255.0f, reg.tint_g / 255.0f, reg.tint_b / 255.0f };
                    if (ImGui::ColorEdit3("Tint", tint))
                    {
                        reg.tint_r = (int)(tint[0] * 255.0f);
                        reg.tint_g = (int)(tint[1] * 255.0f);
                        reg.tint_b = (int)(tint[2] * 255.0f);
                    }
                }

                // Events
                {
                    char room_script[64];
                    snprintf(room_script, sizeof(room_script), "room%d.asc", room_data_->number);
                    char region_name[32];
                    snprintf(region_name, sizeof(region_name), "region%d", reg.id);
                    DrawEventsSection(
                        reg.interactions,
                        InteractionSchemas::RoomRegion(),
                        region_name,
                        room_script,
                        [this, room_script](const std::string&, const std::string&) {
                            editor_.OpenScriptFile(room_script);
                        },
                        [this, room_script](const std::string&, const std::string&) {
                            editor_.OpenScriptFile(room_script);
                        }
                    );
                }
            }
            break;

        case Layer_Characters:
        {
            auto* project = editor_.GetProject();
            if (project && project->GetGameData() &&
                selected_item_id_ < (int)project->GetGameData()->characters.size())
            {
                auto& ch = project->GetGameData()->characters[selected_item_id_];
                if (ImGui::CollapsingHeader("Character", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("Script: %s", ch.script_name.c_str());
                    ImGui::Text("Name: %s", ch.real_name.c_str());
                    ImGui::InputInt("X", &ch.x);
                    ImGui::InputInt("Y", &ch.y);
                    ImGui::Text("Room: %d", ch.room);
                }
            }
            break;
        }

        default:
            break;
        }
    }
    else
    {
        ImGui::TextDisabled("(no item selected)");
    }

    ImGui::EndChild();
}

// ============================================================================
// Undo / Redo
// ============================================================================

void RoomEditor::PushUndo(const std::string& desc, std::function<void()> undo_fn, std::function<void()> redo_fn)
{
    // Truncate redo history beyond current position
    if (undo_pos_ < (int)undo_stack_.size() - 1)
        undo_stack_.resize(undo_pos_ + 1);

    undo_stack_.push_back({ desc, std::move(undo_fn), std::move(redo_fn) });
    undo_pos_ = (int)undo_stack_.size() - 1;

    // Trim old entries
    while ((int)undo_stack_.size() > kMaxUndoSteps)
    {
        undo_stack_.erase(undo_stack_.begin());
        undo_pos_--;
    }
}

void RoomEditor::Undo()
{
    if (undo_pos_ < 0) return;
    undo_stack_[undo_pos_].undo_fn();
    editor_.GetLogPanel().AddLog("[Room] Undo: %s", undo_stack_[undo_pos_].description.c_str());
    undo_pos_--;
}

void RoomEditor::Redo()
{
    if (undo_pos_ >= (int)undo_stack_.size() - 1) return;
    undo_pos_++;
    undo_stack_[undo_pos_].redo_fn();
    editor_.GetLogPanel().AddLog("[Room] Redo: %s", undo_stack_[undo_pos_].description.c_str());
}

// ============================================================================
// Mask helpers — get active mask bitmap and render mask overlays
// ============================================================================

AGS::Common::Bitmap* RoomEditor::GetActiveMask() const
{
    if (!room_data_) return nullptr;
    switch (active_layer_)
    {
        case Layer_Hotspots:    return room_data_->hotspot_mask.get();
        case Layer_WalkAreas:   return room_data_->walkarea_mask.get();
        case Layer_WalkBehinds: return room_data_->walkbehind_mask.get();
        case Layer_Regions:     return room_data_->region_mask.get();
        default: return nullptr;
    }
}

AGS::Common::Bitmap* RoomEditor::GetActiveMaskForLayer(int layer) const
{
    if (!room_data_) return nullptr;
    switch (layer)
    {
        case Layer_Hotspots:    return room_data_->hotspot_mask.get();
        case Layer_WalkAreas:   return room_data_->walkarea_mask.get();
        case Layer_WalkBehinds: return room_data_->walkbehind_mask.get();
        case Layer_Regions:     return room_data_->region_mask.get();
        default: return nullptr;
    }
}

void RoomEditor::DrawMaskOverlay(ImDrawList* dl, ImVec2 origin,
    AGS::Common::Bitmap* mask, int palette_offset, int alpha_fill, int alpha_outline, const char* prefix)
{
    if (!mask) return;

    using namespace AGS::Common;
    int mw = mask->GetWidth();
    int mh = mask->GetHeight();
    int mr = room_data_->mask_resolution;

    // For performance, render in horizontal runs of same area ID
    for (int my = 0; my < mh; my++)
    {
        const unsigned char* scanline = mask->GetScanLine(my);
        int run_start = 0;
        int run_id = scanline[0];

        for (int mx = 1; mx <= mw; mx++)
        {
            int cur_id = (mx < mw) ? scanline[mx] : -1; // Force flush at end
            if (cur_id != run_id)
            {
                if (run_id > 0) // Skip area 0 (none)
                {
                    ImVec2 p0 = RoomToScreen((float)(run_start * mr), (float)(my * mr), origin);
                    ImVec2 p1 = RoomToScreen((float)(mx * mr), (float)((my + 1) * mr), origin);
                    ImU32 fill = GetAreaColor(run_id + palette_offset, alpha_fill);
                    dl->AddRectFilled(p0, p1, fill);
                }
                run_start = mx;
                run_id = cur_id;
            }
        }
    }
}

} // namespace AGSEditor
