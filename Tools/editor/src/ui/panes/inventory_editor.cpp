// AGS Editor ImGui - Inventory Editor
// Uses real inventory data from GameData and renders sprites via TextureCache.
#include "inventory_editor.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "ui/project_panel.h"
#include "ui/events_widget.h"
#include "ui/custom_property_widgets.h"
#include "ui/folder_tree_widget.h"
#include "core/dpi_helper.h"
#include "project/project.h"
#include "project/sprite_loader.h"
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

InventoryEditor::InventoryEditor(EditorUI& editor, int item_id)
    : editor_(editor)
{
    if (item_id >= 0)
        selected_item_ = item_id;
}

void InventoryEditor::Draw()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
    {
        ImGui::TextDisabled("No project loaded. Open a game to view inventory items.");
        return;
    }

    auto* gd = project->GetGameData();
    if (!gd || gd->inventory_items.empty())
    {
        ImGui::TextDisabled("No inventory items in this game.");
        return;
    }

    float list_width = Dpi(180);
    float tree_width = show_folder_tree_ ? Dpi(160) : 0.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Folder tree panel
    if (show_folder_tree_)
    {
        ImGui::BeginChild("##InvFolders", ImVec2(tree_width, avail.y), ImGuiChildFlags_Borders);
        if (gd)
        {
            const auto* sel = static_cast<const FolderInfo*>(selected_folder_);
            const auto* new_sel = DrawFolderTreeWidget(sel, gd->inventory_folders, "All Items", &gd->inventory_folders);
            if (new_sel != sel)
                selected_folder_ = new_sel;
        }
        ImGui::EndChild();
        ImGui::SameLine();
    }

    ImGui::BeginChild("InvList", ImVec2(list_width, avail.y), ImGuiChildFlags_Borders);
    DrawItemList();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("InvProps", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
    DrawItemProperties();
    ImGui::EndChild();
}

void InventoryEditor::DrawItemList()
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd) return;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%d items", (int)gd->inventory_items.size());
    ImGui::SameLine();
    ImGui::Checkbox("Folders", &show_folder_tree_);

    if (ImGui::Button("+ New Item"))
    {
        // AGS engine limit: MAX_INV = 301 (index 0 reserved), so max 300 items
        if ((int)gd->inventory_items.size() >= 300)
        {
            editor_.GetLogPanel().AddLog("[Inventory] Cannot add more items (max 300).");
        }
        else
        {
            GameData::InventoryItemInfo new_item;
            new_item.id = (int)gd->inventory_items.size();
            char sname[32];
            std::snprintf(sname, sizeof(sname), "iItem%d", new_item.id);
            new_item.script_name = sname;
            new_item.description = "New Item";
            new_item.image = 0;
            new_item.cursor_image = -1;
            new_item.start_with = false;
            gd->inventory_items.push_back(new_item);
            selected_item_ = (int)gd->inventory_items.size() - 1;
            editor_.GetProjectPanel().MarkTreeDirty();
        }
    }

    ImGui::Separator();

    // Build folder filter set
    std::set<int> folder_ids;
    if (selected_folder_)
        CollectFolderItemIds(*static_cast<const FolderInfo*>(selected_folder_), folder_ids);

    for (int i = 0; i < (int)gd->inventory_items.size(); i++)
    {
        auto& item = gd->inventory_items[i];
        // Filter by folder
        if (selected_folder_ && folder_ids.find(item.id) == folder_ids.end())
            continue;
        char label[128];
        std::snprintf(label, sizeof(label), "%d: %s", item.id, item.script_name.c_str());
        if (ImGui::Selectable(label, selected_item_ == i))
            selected_item_ = i;
        BeginItemDragSource(item.id, label);

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Duplicate"))
            {
                if ((int)gd->inventory_items.size() >= 300)
                {
                    editor_.GetLogPanel().AddLog("[Inventory] Cannot add more items (max 300).");
                }
                else
                {
                    GameData::InventoryItemInfo dup = item;
                    dup.id = (int)gd->inventory_items.size();
                    dup.script_name += "_copy";
                    gd->inventory_items.push_back(dup);
                    editor_.GetProjectPanel().MarkTreeDirty();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete..."))
            {
                selected_item_ = i;
                confirm_delete_ = true;
            }
            ImGui::EndPopup();
        }
    }
}

void InventoryEditor::DrawItemProperties()
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd || gd->inventory_items.empty() || selected_item_ < 0 || selected_item_ >= (int)gd->inventory_items.size())
    {
        ImGui::TextDisabled("No item selected.");
        return;
    }

    // Deletion confirmation
    if (confirm_delete_)
    {
        ImGui::OpenPopup("Confirm Delete Item");
        confirm_delete_ = false;
    }
    if (ImGui::BeginPopupModal("Confirm Delete Item", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (selected_item_ >= 0 && selected_item_ < (int)gd->inventory_items.size())
        {
            ImGui::Text("Delete item '%s'?", gd->inventory_items[selected_item_].script_name.c_str());
            ImGui::Text("This action cannot be undone.");
            ImGui::Separator();
            if (ImGui::Button("Delete", ImVec2(Dpi(120), 0)))
            {
                gd->inventory_items.erase(gd->inventory_items.begin() + selected_item_);
                for (int i = 0; i < (int)gd->inventory_items.size(); i++) gd->inventory_items[i].id = i;
                if (selected_item_ >= (int)gd->inventory_items.size())
                    selected_item_ = std::max(0, (int)gd->inventory_items.size() - 1);
                editor_.GetProjectPanel().MarkTreeDirty();
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

    auto& item = gd->inventory_items[selected_item_];

    ImGui::Text("Inventory Item ID: %d", item.id);
    ImGui::Separator();

    float field_w = Dpi(200);

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputText("Script Name", &item.script_name);
        {
            std::string err = ValidateScriptName(*gd, item.script_name, "Inventory", item.id);
            if (!err.empty())
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", err.c_str());
        }
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputText("Description", &item.description);
    }

    if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Image (Sprite)", &item.image);
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Cursor Image", &item.cursor_image);
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Hotspot X", &item.hotspot_x);
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Hotspot Y", &item.hotspot_y);
        ImGui::TextDisabled("(Click on preview to set hotspot)");
    }

    if (ImGui::CollapsingHeader("Behavior"))
    {
        ImGui::Checkbox("Player Starts With", &item.start_with);
    }

    // Custom properties
    if (ImGui::CollapsingHeader("Custom Properties"))
    {
        if (!gd->custom_property_schemas.empty())
        {
            DrawCustomPropertyValues(gd->custom_property_schemas,
                                     item.custom_properties, "inv_items");
        }
        else
        {
            ImGui::TextDisabled("No custom properties defined");
        }
    }

    // Events section
    bool want_focus = focus_events_;
    focus_events_ = false;
    DrawEventsSection(
        item.interactions,
        InteractionSchemas::InventoryItem(),
        item.script_name,
        "GlobalScript.asc",
        [this](const std::string& script_module, const std::string& func_name) {
            editor_.OpenScriptFile(script_module);
            editor_.GetLogPanel().AddLog("[Events] Created handler: %s in %s",
                func_name.c_str(), script_module.c_str());
        },
        [this](const std::string& script_module, const std::string& func_name) {
            editor_.OpenScriptFile(script_module);
        },
        want_focus
    );

    DrawItemPreview();
}

void InventoryEditor::DrawItemPreview()
{
    if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto* gd = editor_.GetApp().GetProject()->GetGameData();
        if (!gd || selected_item_ < 0 || selected_item_ >= (int)gd->inventory_items.size()) return;
        auto& item = gd->inventory_items[selected_item_];

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
            IM_COL32(40, 40, 55, 255));

        // Try to render real sprite for the inventory image
        SDL_Texture* tex = nullptr;
        if (loader && item.image >= 0)
            tex = tex_cache.GetSpriteTexture(item.image, loader);

        float cx = pos.x + size.x * 0.5f;
        float cy = pos.y + size.y * 0.5f;

        float sprite_x = cx, sprite_y = cy;
        float scale = 1.0f;
        float sw = 0, sh = 0;

        if (tex)
        {
            const auto* info = tex_cache.FindSprite(item.image);
            sw = info ? (float)info->width : 32.0f;
            sh = info ? (float)info->height : 32.0f;
            scale = preview_zoom_;
            float dw = sw * scale;
            float dh = sh * scale;

            sprite_x = cx - dw / 2;
            sprite_y = cy - dh / 2;

            dl->AddImage((ImTextureID)(intptr_t)tex,
                ImVec2(sprite_x, sprite_y),
                ImVec2(sprite_x + dw, sprite_y + dh));

            // Draw hotspot marker on the sprite
            float hx = sprite_x + item.hotspot_x * scale;
            float hy = sprite_y + item.hotspot_y * scale;
            dl->AddCircleFilled(ImVec2(hx, hy), 4.0f, IM_COL32(255, 0, 0, 255));
            dl->AddCircle(ImVec2(hx, hy), 4.0f, IM_COL32(255, 255, 255, 255));
            dl->AddLine(ImVec2(hx - 8, hy), ImVec2(hx + 8, hy), IM_COL32(255, 0, 0, 180));
            dl->AddLine(ImVec2(hx, hy - 8), ImVec2(hx, hy + 8), IM_COL32(255, 0, 0, 180));
        }
        else
        {
            // Fallback colored rect
            float w = 32, h = 32;
            ImU32 col = IM_COL32(120 + (item.image * 23) % 135, 100 + (item.image * 37) % 155, 80, 255);
            dl->AddRectFilled(ImVec2(cx - w/2, cy - h/2), ImVec2(cx + w/2, cy + h/2), col);

            char buf[64];
            std::snprintf(buf, sizeof(buf), "Spr %d", item.image);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            dl->AddText(ImVec2(cx - ts.x/2, cy - ts.y/2), IM_COL32(255, 255, 255, 220), buf);
        }

        // Also show cursor image as a small inset if different
        if (item.cursor_image >= 0 && item.cursor_image != item.image)
        {
            SDL_Texture* cur_tex = loader ? tex_cache.GetSpriteTexture(item.cursor_image, loader) : nullptr;
            float inset_size = Dpi(32);
            float ix = pos.x + size.x - inset_size - 4;
            float iy = pos.y + 4;

            dl->AddRectFilled(ImVec2(ix, iy), ImVec2(ix + inset_size, iy + inset_size),
                IM_COL32(30, 30, 50, 200));

            if (cur_tex)
            {
                const auto* cinfo = tex_cache.FindSprite(item.cursor_image);
                float cw = cinfo ? (float)cinfo->width : inset_size;
                float ch = cinfo ? (float)cinfo->height : inset_size;
                float cscale = std::min(inset_size / cw, inset_size / ch);
                float cdw = cw * cscale;
                float cdh = ch * cscale;
                float cox = (inset_size - cdw) * 0.5f;
                float coy = (inset_size - cdh) * 0.5f;
                dl->AddImage((ImTextureID)(intptr_t)cur_tex,
                    ImVec2(ix + cox, iy + coy),
                    ImVec2(ix + cox + cdw, iy + coy + cdh));
            }

            dl->AddRect(ImVec2(ix, iy), ImVec2(ix + inset_size, iy + inset_size),
                IM_COL32(200, 200, 100, 200));

            char clbl[] = "Cursor";
            ImVec2 cts = ImGui::CalcTextSize(clbl);
            dl->AddText(ImVec2(ix + (inset_size - cts.x) * 0.5f, iy + inset_size + 1),
                IM_COL32(200, 200, 100, 180), clbl);
        }

        dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
            IM_COL32(80, 80, 80, 255));
        ImGui::InvisibleButton("##InvPreview", size);

        // Click on preview to set hotspot position
        if (tex && sw > 0 && sh > 0 && ImGui::IsItemClicked())
        {
            ImVec2 mouse = ImGui::GetMousePos();
            int new_hx = (int)((mouse.x - sprite_x) / scale);
            int new_hy = (int)((mouse.y - sprite_y) / scale);
            item.hotspot_x = std::max(0, std::min(new_hx, (int)sw - 1));
            item.hotspot_y = std::max(0, std::min(new_hy, (int)sh - 1));
        }
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
}

} // namespace AGSEditor
