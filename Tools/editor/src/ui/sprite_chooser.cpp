// AGS Editor ImGui - Sprite Chooser Popup
#include "sprite_chooser.h"
#include "project/sprite_loader.h"
#include "project/texture_cache.h"
#include "core/dpi_helper.h"
#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <vector>
#include <cstring>

namespace AGSEditor
{

bool DrawSpriteChooserPopup(const char* popup_id, SpriteLoader* loader,
                            TextureCache& tex_cache, int* out_sprite,
                            int current_sprite)
{
    bool selected = false;

    ImGui::SetNextWindowSize(ImVec2(Dpi(500), Dpi(400)), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal(popup_id, nullptr,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        if (!loader || !loader->IsOpen())
        {
            ImGui::TextDisabled("Sprite set not loaded.");
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return false;
        }

        int total = loader->GetSpriteCount();

        // Filter input
        static char filter_buf[32] = "";
        ImGui::SetNextItemWidth(Dpi(120));
        ImGui::InputText("Filter ID", filter_buf, sizeof(filter_buf));
        ImGui::SameLine();
        ImGui::Text("%d sprites", total);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - Dpi(60));
        if (ImGui::Button("Cancel", ImVec2(Dpi(60), 0)))
            ImGui::CloseCurrentPopup();

        ImGui::Separator();

        // Grid parameters
        float cell_size = Dpi(64);
        float icon_size = cell_size - 4.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;

        ImGui::BeginChild("##SpriteGrid", ImVec2(0, 0), ImGuiChildFlags_None);

        float avail_w = ImGui::GetContentRegionAvail().x;
        int cols = std::max(1, (int)((avail_w + spacing) / (icon_size + spacing)));

        // Build filtered sprite list (indices of sprites that pass the filter)
        bool has_filter = (filter_buf[0] != '\0');
        std::vector<int> visible;
        visible.reserve(total);
        for (int i = 0; i < total; i++)
        {
            const auto* met = loader->GetMetrics(i);
            if (!met || !met->exists) continue;

            if (has_filter)
            {
                // Match filter against sprite ID string
                char id_str[16];
                std::snprintf(id_str, sizeof(id_str), "%d", i);
                if (strstr(id_str, filter_buf) == nullptr)
                    continue;
            }

            visible.push_back(i);
        }

        int visible_count = (int)visible.size();
        int row_count = (visible_count + cols - 1) / cols;

        // Use ImGuiListClipper to only render visible rows
        ImGuiListClipper clipper;
        clipper.Begin(row_count, icon_size + spacing);

        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            {
                int first_idx = row * cols;
                int last_idx = std::min(first_idx + cols, visible_count);

                for (int vi = first_idx; vi < last_idx; vi++)
                {
                    int i = visible[vi];
                    const auto* met = loader->GetMetrics(i);
                    if (!met) continue;

                    if (vi > first_idx)
                        ImGui::SameLine();

                    ImGui::PushID(i);

                    ImVec2 cursor = ImGui::GetCursorScreenPos();
                    ImVec2 btn_size(icon_size, icon_size);
                    bool clicked = ImGui::InvisibleButton("##s", btn_size);
                    ImDrawList* dl = ImGui::GetWindowDrawList();

                    // Background
                    bool is_current = (i == current_sprite);
                    ImU32 bg = is_current ? IM_COL32(77, 102, 153, 255) : IM_COL32(38, 38, 51, 255);
                    if (ImGui::IsItemHovered())
                        bg = IM_COL32(64, 89, 128, 255);
                    dl->AddRectFilled(cursor, ImVec2(cursor.x + icon_size, cursor.y + icon_size), bg);

                    // Render sprite texture
                    SDL_Texture* tex = tex_cache.GetSpriteTexture(i, loader);
                    if (tex)
                    {
                        float scale = std::min(icon_size / (float)met->width,
                                               icon_size / (float)met->height);
                        float dw = met->width * scale;
                        float dh = met->height * scale;
                        float ox = (icon_size - dw) * 0.5f;
                        float oy = (icon_size - dh) * 0.5f;
                        dl->AddImage((ImTextureID)(intptr_t)tex,
                            ImVec2(cursor.x + ox, cursor.y + oy),
                            ImVec2(cursor.x + ox + dw, cursor.y + oy + dh));
                    }

                    // ID label
                    char id_label[16];
                    std::snprintf(id_label, sizeof(id_label), "%d", i);
                    dl->AddText(ImVec2(cursor.x + 2, cursor.y + 1),
                        IM_COL32(200, 200, 200, 180), id_label);

                    // Selection border
                    if (is_current)
                        dl->AddRect(cursor, ImVec2(cursor.x + icon_size, cursor.y + icon_size),
                            IM_COL32(100, 180, 255, 255), 0.0f, 0, 2.0f);

                    if (clicked)
                    {
                        *out_sprite = i;
                        selected = true;
                        ImGui::CloseCurrentPopup();
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Sprite %d\nSize: %dx%d\nDepth: %d bpp",
                            i, met->width, met->height, met->color_depth);
                    }

                    ImGui::PopID();
                }
            }
        }

        ImGui::EndChild();
        ImGui::EndPopup();
    }

    return selected;
}

bool DrawSpriteField(const char* label, int* sprite_id,
                     SpriteLoader* loader, TextureCache& tex_cache)
{
    bool browse_clicked = false;

    ImGui::PushID(label);

    // Show small sprite preview thumbnail
    float thumb_size = Dpi(32);
    if (loader && loader->IsOpen() && *sprite_id >= 0)
    {
        SDL_Texture* tex = tex_cache.GetSpriteTexture(*sprite_id, loader);
        if (tex)
        {
            const auto* met = loader->GetMetrics(*sprite_id);
            if (met && met->exists)
            {
                float scale = std::min(thumb_size / (float)met->width,
                                       thumb_size / (float)met->height);
                float dw = met->width * scale;
                float dh = met->height * scale;

                ImVec2 cursor = ImGui::GetCursorScreenPos();
                ImGui::Dummy(ImVec2(thumb_size, thumb_size));
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(cursor,
                    ImVec2(cursor.x + thumb_size, cursor.y + thumb_size),
                    IM_COL32(38, 38, 51, 255));
                float ox = (thumb_size - dw) * 0.5f;
                float oy = (thumb_size - dh) * 0.5f;
                dl->AddImage((ImTextureID)(intptr_t)tex,
                    ImVec2(cursor.x + ox, cursor.y + oy),
                    ImVec2(cursor.x + ox + dw, cursor.y + oy + dh));
                dl->AddRect(cursor,
                    ImVec2(cursor.x + thumb_size, cursor.y + thumb_size),
                    IM_COL32(80, 80, 80, 200));
                ImGui::SameLine();
            }
        }
    }

    // Sprite ID input
    ImGui::SetNextItemWidth(Dpi(80));
    ImGui::InputInt(label, sprite_id);

    // Browse button
    ImGui::SameLine();
    browse_clicked = ImGui::Button("...");

    ImGui::PopID();
    return browse_clicked;
}

} // namespace AGSEditor
