// AGS Editor ImGui - Cursor Editor
// Uses real cursor data from GameData and renders sprites via TextureCache.
#include "cursor_editor.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
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

namespace AGSEditor
{

CursorEditor::CursorEditor(EditorUI& editor, int cursor_id)
    : editor_(editor)
{
    if (cursor_id >= 0)
        selected_cursor_ = cursor_id;
}

void CursorEditor::Draw()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
    {
        ImGui::TextDisabled("No project loaded. Open a game to view cursors.");
        return;
    }

    auto* gd = project->GetGameData();
    if (!gd || gd->cursors.empty())
    {
        ImGui::TextDisabled("No cursors in this game.");
        return;
    }

    float list_width = Dpi(180);
    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("CurList", ImVec2(list_width, avail.y), ImGuiChildFlags_Borders);
    DrawCursorList();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("CurProps", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
    DrawCursorProperties();
    ImGui::EndChild();
}

void CursorEditor::DrawCursorList()
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd) return;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%d cursors", (int)gd->cursors.size());

    if (ImGui::Button("+ New Cursor"))
    {
        GameData::CursorInfo new_cur;
        new_cur.id = (int)gd->cursors.size();
        char sname[32];
        std::snprintf(sname, sizeof(sname), "Cursor %d", new_cur.id);
        new_cur.name = sname;
        new_cur.image = 0;
        new_cur.hotspot_x = 0;
        new_cur.hotspot_y = 0;
        new_cur.process_click = false;
        new_cur.animate = false;
        new_cur.view = -1;
        gd->cursors.push_back(new_cur);
        selected_cursor_ = (int)gd->cursors.size() - 1;
    }

    ImGui::Separator();

    for (int i = 0; i < (int)gd->cursors.size(); i++)
    {
        auto& cur = gd->cursors[i];
        char label[128];
        std::snprintf(label, sizeof(label), "%d: %s", cur.id, cur.name.c_str());
        if (ImGui::Selectable(label, selected_cursor_ == i))
            selected_cursor_ = i;

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Duplicate"))
            {
                GameData::CursorInfo dup = cur;
                dup.id = (int)gd->cursors.size();
                dup.name += " (copy)";
                gd->cursors.push_back(dup);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete..."))
            {
                selected_cursor_ = i;
                confirm_delete_ = true;
            }
            ImGui::EndPopup();
        }
    }
}

void CursorEditor::DrawCursorProperties()
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd || gd->cursors.empty() || selected_cursor_ < 0 || selected_cursor_ >= (int)gd->cursors.size())
    {
        ImGui::TextDisabled("No cursor selected.");
        return;
    }

    // Deletion confirmation
    if (confirm_delete_)
    {
        ImGui::OpenPopup("Confirm Delete Cursor");
        confirm_delete_ = false;
    }
    if (ImGui::BeginPopupModal("Confirm Delete Cursor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (selected_cursor_ >= 0 && selected_cursor_ < (int)gd->cursors.size())
        {
            ImGui::Text("Delete cursor '%s'?", gd->cursors[selected_cursor_].name.c_str());
            ImGui::Text("This action cannot be undone.");
            ImGui::Separator();
            if (ImGui::Button("Delete", ImVec2(Dpi(120), 0)))
            {
                gd->cursors.erase(gd->cursors.begin() + selected_cursor_);
                for (int i = 0; i < (int)gd->cursors.size(); i++) gd->cursors[i].id = i;
                if (selected_cursor_ >= (int)gd->cursors.size())
                    selected_cursor_ = std::max(0, (int)gd->cursors.size() - 1);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
                ImGui::CloseCurrentPopup();
        }
        else
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    auto& cur = gd->cursors[selected_cursor_];

    ImGui::Text("Cursor Mode %d", cur.id);
    ImGui::Separator();

    float field_w = Dpi(200);

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputText("Name", &cur.name);
        {
            std::string err = ValidateScriptName(*gd, cur.name, "Cursor", cur.id);
            if (!err.empty())
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", err.c_str());
        }
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Image (Sprite)", &cur.image);
        ImGui::Checkbox("Process Click", &cur.process_click);
    }

    if (ImGui::CollapsingHeader("Hotspot", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Hotspot X", &cur.hotspot_x);
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Hotspot Y", &cur.hotspot_y);
        ImGui::TextDisabled("(Click on preview to set hotspot)");
    }

    if (ImGui::CollapsingHeader("Animation"))
    {
        ImGui::Checkbox("Animate", &cur.animate);
        if (cur.animate)
        {
            ImGui::SetNextItemWidth(field_w);
            ImGui::InputInt("View", &cur.view);
            if (cur.view >= 0) {
                auto* vinfo = gd->FindViewById(cur.view);
                if (vinfo)
                    ImGui::Text("View Name: %s", vinfo->name.c_str());
            }
            ImGui::Checkbox("Only on Hotspot", &cur.animate_only_on_hotspot);
            ImGui::Checkbox("Only When Moving", &cur.animate_only_when_moving);
            ImGui::SetNextItemWidth(field_w);
            ImGui::InputInt("Animation Delay", &cur.animation_delay);
        }
    }

    DrawCursorPreview();
}

void CursorEditor::DrawCursorPreview()
{
    if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto* gd = editor_.GetApp().GetProject()->GetGameData();
        if (!gd || selected_cursor_ < 0 || selected_cursor_ >= (int)gd->cursors.size()) return;
        auto& cur = gd->cursors[selected_cursor_];

        auto* project = editor_.GetApp().GetProject();
        auto* loader = project ? project->GetSpriteLoader() : nullptr;
        auto& tex_cache = editor_.GetApp().GetTextureCache();

        ImGui::SetNextItemWidth(Dpi(100));
        ImGui::SliderFloat("Zoom", &preview_zoom_, 1.0f, 8.0f, "%.0fx");

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float preview_h = std::max(Dpi(60), Dpi(40) + Dpi(32) * preview_zoom_);
        preview_h = std::min(preview_h, avail.y);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size(avail.x, preview_h);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
            IM_COL32(45, 45, 60, 255));

        float cx = pos.x + size.x * 0.5f;
        float cy = pos.y + size.y * 0.5f;

        // Try to render real cursor sprite
        SDL_Texture* tex = nullptr;
        if (loader && cur.image >= 0)
            tex = tex_cache.GetSpriteTexture(cur.image, loader);

        float sprite_x = cx, sprite_y = cy;
        float scale = 1.0f;
        float sw = 0, sh = 0;

        if (tex)
        {
            const auto* info = tex_cache.FindSprite(cur.image);
            sw = info ? (float)info->width : 32.0f;
            sh = info ? (float)info->height : 32.0f;
            scale = preview_zoom_;
            float dw = sw * scale;
            float dh = sh * scale;

            // Position so that (0,0) of sprite is at center
            sprite_x = cx - dw / 2;
            sprite_y = cy - dh / 2;

            dl->AddImage((ImTextureID)(intptr_t)tex,
                ImVec2(sprite_x, sprite_y),
                ImVec2(sprite_x + dw, sprite_y + dh));

            // Draw hotspot marker on the sprite
            float hx = sprite_x + cur.hotspot_x * scale;
            float hy = sprite_y + cur.hotspot_y * scale;
            dl->AddCircleFilled(ImVec2(hx, hy), 4.0f, IM_COL32(255, 0, 0, 255));
            dl->AddCircle(ImVec2(hx, hy), 4.0f, IM_COL32(255, 255, 255, 255));

            // Crosshair at hotspot
            dl->AddLine(ImVec2(hx - 8, hy), ImVec2(hx + 8, hy), IM_COL32(255, 0, 0, 180));
            dl->AddLine(ImVec2(hx, hy - 8), ImVec2(hx, hy + 8), IM_COL32(255, 0, 0, 180));
        }
        else
        {
            // Fallback: draw arrow cursor shape
            ImVec2 points[7] = {
                ImVec2(cx, cy - 16),
                ImVec2(cx + 12, cy + 4),
                ImVec2(cx + 4, cy + 4),
                ImVec2(cx + 8, cy + 14),
                ImVec2(cx + 4, cy + 16),
                ImVec2(cx, cy + 8),
                ImVec2(cx - 4, cy + 12)
            };
            dl->AddConvexPolyFilled(points, 7, IM_COL32(255, 255, 255, 230));
            dl->AddPolyline(points, 7, IM_COL32(0, 0, 0, 255), ImDrawFlags_Closed, 1.0f);

            // Hotspot marker
            float hx = cx + cur.hotspot_x;
            float hy = cy - 16 + cur.hotspot_y;
            dl->AddCircleFilled(ImVec2(hx, hy), 3.0f, IM_COL32(255, 0, 0, 255));
        }

        // Label
        char buf[96];
        std::snprintf(buf, sizeof(buf), "%s (Spr %d)", cur.name.c_str(), cur.image);
        ImVec2 ts = ImGui::CalcTextSize(buf);
        dl->AddText(ImVec2(pos.x + (size.x - ts.x) * 0.5f, pos.y + 3),
            IM_COL32(255, 255, 255, 180), buf);

        dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
            IM_COL32(80, 80, 80, 255));
        ImGui::InvisibleButton("##CursorPreview", size);

        // Click on preview to set hotspot position
        if (tex && sw > 0 && sh > 0 && ImGui::IsItemClicked())
        {
            ImVec2 mouse = ImGui::GetMousePos();
            int new_hx = (int)((mouse.x - sprite_x) / scale);
            int new_hy = (int)((mouse.y - sprite_y) / scale);
            cur.hotspot_x = std::max(0, std::min(new_hx, (int)sw - 1));
            cur.hotspot_y = std::max(0, std::min(new_hy, (int)sh - 1));
        }
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
}

} // namespace AGSEditor
