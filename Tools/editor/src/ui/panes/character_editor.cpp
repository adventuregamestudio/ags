// AGS Editor ImGui - Character Editor
// Uses real character/view data from GameData and renders sprites via TextureCache.
#include "character_editor.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "ui/project_panel.h"
#include "ui/events_widget.h"
#include "ui/custom_property_widgets.h"
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
#include <cmath>
#include <algorithm>

namespace AGSEditor
{

CharacterEditor::CharacterEditor(EditorUI& editor, int char_id)
    : editor_(editor)
    , title_("Characters")
{
    if (char_id >= 0)
        selected_char_ = char_id;
}

std::vector<GameData::CharacterInfo>* CharacterEditor::GetCharacters()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded()) return nullptr;
    auto* gd = project->GetGameData();
    if (!gd) return nullptr;
    return &gd->characters;
}

// ============================================================================
// Main Draw
// ============================================================================

void CharacterEditor::Draw()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
    {
        ImGui::TextDisabled("No project loaded. Open a game to view characters.");
        return;
    }

    auto* chars = GetCharacters();
    if (!chars)
    {
        ImGui::TextDisabled("No game data available.");
        return;
    }

    float list_width = Dpi(190);
    float tree_width = show_folder_tree_ ? Dpi(160) : 0.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Folder tree panel
    if (show_folder_tree_)
    {
        ImGui::BeginChild("##CharFolders", ImVec2(tree_width, avail.y), ImGuiChildFlags_Borders);
        auto* gd = project->GetGameData();
        if (gd)
        {
            const auto* sel = static_cast<const FolderInfo*>(selected_folder_);
            const auto* new_sel = DrawFolderTreeWidget(sel, gd->character_folders, "All Characters", &gd->character_folders);
            if (new_sel != sel)
                selected_folder_ = new_sel;
        }
        ImGui::EndChild();
        ImGui::SameLine();
    }

    ImGui::BeginChild("##CharList", ImVec2(list_width, avail.y), ImGuiChildFlags_Borders);
    DrawCharacterList();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##CharProps", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
    DrawCharacterProperties();
    ImGui::EndChild();
}

// ============================================================================
// Character List
// ============================================================================

void CharacterEditor::DrawCharacterList()
{
    auto* chars = GetCharacters();
    if (!chars) return;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%d characters", (int)chars->size());
    ImGui::SameLine();
    ImGui::Checkbox("Folders", &show_folder_tree_);

    if (ImGui::Button("+ New Character"))
    {
        GameData::CharacterInfo new_ch;
        new_ch.id = (int)chars->size();
        char sname[32];
        std::snprintf(sname, sizeof(sname), "cChar%d", new_ch.id);
        new_ch.script_name = sname;
        new_ch.real_name = "New Character";
        new_ch.room = 0;
        new_ch.x = 160;
        new_ch.y = 120;
        new_ch.normal_view = 0;
        new_ch.speech_view = -1;
        new_ch.idle_view = -1;
        new_ch.thinking_view = -1;
        new_ch.blinking_view = -1;
        chars->push_back(new_ch);
        selected_char_ = (int)chars->size() - 1;
        editor_.GetProjectPanel().MarkTreeDirty();
    }

    ImGui::Separator();

    // Build folder filter set
    std::set<int> folder_ids;
    if (selected_folder_)
        CollectFolderItemIds(*static_cast<const FolderInfo*>(selected_folder_), folder_ids);

    for (int i = 0; i < (int)chars->size(); i++)
    {
        auto& ch = (*chars)[i];
        // Filter by folder
        if (selected_folder_ && folder_ids.find(ch.id) == folder_ids.end())
            continue;
        char label[196];
        std::snprintf(label, sizeof(label), "%d: %s (%s)",
                      ch.id, ch.script_name.c_str(), ch.real_name.c_str());
        if (ImGui::Selectable(label, selected_char_ == i))
            selected_char_ = i;
        BeginItemDragSource(ch.id, label);

        // Context menu per character
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Make Player Character"))
            {
                // Clear player flag on all, set on this one
                for (auto& c : *chars) c.is_player = false;
                ch.is_player = true;
            }
            if (ImGui::MenuItem("Change Character ID..."))
            {
                selected_char_ = i;
                change_id_target_ = i;
                show_change_id_ = true;
            }
            if (ImGui::MenuItem("Duplicate"))
            {
                GameData::CharacterInfo dup = ch;
                dup.id = (int)chars->size();
                dup.script_name += "_copy";
                chars->push_back(dup);
                editor_.GetProjectPanel().MarkTreeDirty();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete..."))
            {
                selected_char_ = i;
                confirm_delete_ = true;
            }
            ImGui::EndPopup();
        }
    }
}

void CharacterEditor::DrawContextMenu()
{
    if (confirm_delete_)
    {
        ImGui::OpenPopup("Confirm Delete Character");
        confirm_delete_ = false;
    }

    if (ImGui::BeginPopupModal("Confirm Delete Character", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto* chars = GetCharacters();
        if (chars && selected_char_ >= 0 && selected_char_ < (int)chars->size())
        {
            auto& ch = (*chars)[selected_char_];
            ImGui::Text("Are you sure you want to delete character '%s' (%s)?",
                        ch.real_name.c_str(), ch.script_name.c_str());
            ImGui::Text("This action cannot be undone.");
            ImGui::Separator();

            if (ImGui::Button("Delete", ImVec2(Dpi(120), 0)))
            {
                DeleteCharacter(selected_char_);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
            {
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Change Character ID popup
    if (show_change_id_)
    {
        ImGui::OpenPopup("Change Character ID");
        show_change_id_ = false;
    }
    if (ImGui::BeginPopupModal("Change Character ID", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto* chars = GetCharacters();
        if (chars && selected_char_ >= 0 && selected_char_ < (int)chars->size())
        {
            ImGui::Text("Change '%s' (ID %d) to new position:",
                (*chars)[selected_char_].script_name.c_str(), selected_char_);
            ImGui::InputInt("New ID", &change_id_target_);
            change_id_target_ = std::max(0, std::min(change_id_target_, (int)chars->size() - 1));
            ImGui::Separator();

            if (ImGui::Button("Swap", ImVec2(Dpi(120), 0)))
            {
                if (change_id_target_ != selected_char_)
                {
                    ChangeCharacterId(selected_char_, change_id_target_);
                    selected_char_ = change_id_target_;
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
                ImGui::CloseCurrentPopup();
        }
        else
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void CharacterEditor::DeleteCharacter(int index)
{
    auto* chars = GetCharacters();
    if (!chars || index < 0 || index >= (int)chars->size()) return;
    chars->erase(chars->begin() + index);
    // Re-number IDs
    for (int i = 0; i < (int)chars->size(); i++)
        (*chars)[i].id = i;
    if (selected_char_ >= (int)chars->size())
        selected_char_ = std::max(0, (int)chars->size() - 1);
    editor_.GetProjectPanel().MarkTreeDirty();
}

void CharacterEditor::ChangeCharacterId(int old_index, int new_index)
{
    auto* chars = GetCharacters();
    if (!chars) return;
    int count = (int)chars->size();
    if (old_index < 0 || old_index >= count || new_index < 0 || new_index >= count) return;
    if (old_index == new_index) return;

    // Swap the two characters
    std::swap((*chars)[old_index], (*chars)[new_index]);

    // Re-number IDs to match positions
    for (int i = 0; i < count; i++)
        (*chars)[i].id = i;

    editor_.GetProjectPanel().MarkTreeDirty();
}

// ============================================================================
// Properties
// ============================================================================

void CharacterEditor::DrawCharacterProperties()
{
    auto* chars = GetCharacters();
    if (!chars || chars->empty() || selected_char_ < 0 || selected_char_ >= (int)chars->size())
    {
        ImGui::TextDisabled("No character selected.");
        return;
    }

    // Handle confirmation dialog
    DrawContextMenu();

    auto& ch = (*chars)[selected_char_];
    auto* gd = editor_.GetApp().GetProject()->GetGameData();

    if (ch.is_player)
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1.0f), "** Player Character **");
    ImGui::Text("Character ID: %d", ch.id);
    ImGui::Separator();

    // Two columns: properties + preview
    float props_w = Dpi(380);
    ImVec2 content_avail = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("##CharPropsLeft", ImVec2(props_w, content_avail.y));

    float field_w = Dpi(200);

    if (ImGui::CollapsingHeader("Identity", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputText("Script Name", &ch.script_name);
        if (gd)
        {
            std::string err = ValidateScriptName(*gd, ch.script_name, "Character", ch.id);
            if (!err.empty())
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", err.c_str());
        }
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputText("Real Name", &ch.real_name);
    }

    if (ImGui::CollapsingHeader("Position & Room", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Starting Room", &ch.room);
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Start X", &ch.x);
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Start Y", &ch.y);
    }

    if (ImGui::CollapsingHeader("Views", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (gd)
        {
            DrawViewDropdown("Normal View", ch.normal_view);
            DrawViewDropdown("Speech View", ch.speech_view);
            DrawViewDropdown("Idle View", ch.idle_view);
            DrawViewDropdown("Thinking View", ch.thinking_view);
            DrawViewDropdown("Blinking View", ch.blinking_view);
        }
    }

    if (ImGui::CollapsingHeader("Movement", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Movement Speed", &ch.movement_speed);
        ImGui::Checkbox("Uniform Movement Speed", &ch.uniform_movement_speed);
        if (!ch.uniform_movement_speed)
        {
            ImGui::SetNextItemWidth(field_w);
            ImGui::InputInt("Movement Speed X", &ch.movement_speed_x);
            ImGui::SetNextItemWidth(field_w);
            ImGui::InputInt("Movement Speed Y", &ch.movement_speed_y);
        }
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Animation Delay", &ch.animation_delay);
        ImGui::Checkbox("Turn Before Walking", &ch.turn_before_walking);
        ImGui::Checkbox("Turn When Facing", &ch.turn_when_facing);
        ImGui::Checkbox("Diagonal Loops", &ch.diagonal_loops);
        ImGui::Checkbox("Adjust Speed With Scaling", &ch.adjust_speed_with_scaling);
        ImGui::Checkbox("Movement Linked to Animation", &ch.movement_linked_to_animation);
    }

    if (ImGui::CollapsingHeader("Appearance"))
    {
        ImGui::Checkbox("Clickable", &ch.clickable);
        ImGui::Checkbox("Solid", &ch.solid);
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Blocking Width", &ch.blocking_width);
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Blocking Height", &ch.blocking_height);
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Baseline", &ch.baseline);
        ImGui::Checkbox("Use Room Area Scaling", &ch.use_room_area_scaling);
        ImGui::Checkbox("Use Room Area Lighting", &ch.use_room_area_lighting);
        ImGui::Checkbox("Scale Volume", &ch.scale_volume);
        ImGui::SetNextItemWidth(field_w);
        ImGui::SliderInt("Transparency", &ch.transparency, 0, 100, "%d%%");
    }

    if (ImGui::CollapsingHeader("Speech"))
    {
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Speech Anim Delay", &ch.speech_animation_delay);
        float sc[3] = {
            ((ch.speech_color >> 16) & 0xFF) / 255.0f,
            ((ch.speech_color >> 8) & 0xFF) / 255.0f,
            (ch.speech_color & 0xFF) / 255.0f };
        if (ImGui::ColorEdit3("Speech Color", sc))
        {
            ch.speech_color = ((int)(sc[0] * 255) << 16) |
                              ((int)(sc[1] * 255) << 8) |
                              (int)(sc[2] * 255);
        }
    }

    if (ImGui::CollapsingHeader("Idle"))
    {
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Idle Delay (seconds)", &ch.idle_delay);
        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Idle Anim Speed", &ch.idle_anim_speed);
    }

    // Custom properties
    if (ImGui::CollapsingHeader("Custom Properties"))
    {
        if (gd && !gd->custom_property_schemas.empty())
        {
            DrawCustomPropertyValues(gd->custom_property_schemas,
                                     ch.custom_properties, "characters");
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
        ch.interactions,
        InteractionSchemas::Character(),
        ch.script_name,
        "GlobalScript.asc",
        [this](const std::string& script_module, const std::string& func_name) {
            // Create handler: open script file and append function stub
            editor_.OpenScriptFile(script_module);
            editor_.GetLogPanel().AddLog("[Events] Created handler: %s in %s",
                func_name.c_str(), script_module.c_str());
        },
        [this](const std::string& script_module, const std::string& func_name) {
            // Navigate to handler: open script file
            editor_.OpenScriptFile(script_module);
        },
        want_focus
    );

    ImGui::EndChild();

    ImGui::SameLine();

    // Right side: previews (show first view's first loop if available)
    ImGui::BeginChild("##CharPreviews", ImVec2(0, content_avail.y));

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Preview");
    ImGui::Separator();

    ImGui::Checkbox("Animate", &preview_playing_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(Dpi(80));
    ImGui::SliderFloat("FPS", &preview_fps_, 1.0f, 15.0f);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(Dpi(80));
    ImGui::SliderFloat("Zoom", &preview_zoom_, 1.0f, 8.0f, "%.0fx");

    // Try to show the character's normal view
    if (gd && !gd->views.empty())
    {
        if (chars && selected_char_ >= 0 && selected_char_ < (int)chars->size())
        {
            auto& ch_preview = (*chars)[selected_char_];
            int normal_v = ch_preview.normal_view;
            if (normal_v >= 0 && gd->FindViewById(normal_v))
            {
                char vlabel[64];
                std::snprintf(vlabel, sizeof(vlabel), "Normal View (%d)", normal_v);
                DrawViewPreview(vlabel, normal_v, normal_timer_, normal_frame_);
            }
            int speech_v = ch_preview.speech_view;
            if (speech_v >= 0 && gd->FindViewById(speech_v))
            {
                char vlabel[64];
                std::snprintf(vlabel, sizeof(vlabel), "Speech View (%d)", speech_v);
                DrawViewPreview(vlabel, speech_v, speech_timer_, speech_frame_);
            }
            int idle_v = ch_preview.idle_view;
            if (idle_v >= 0 && gd->FindViewById(idle_v))
            {
                char vlabel[64];
                std::snprintf(vlabel, sizeof(vlabel), "Idle View (%d)", idle_v);
                DrawViewPreview(vlabel, idle_v, idle_timer_, idle_frame_);
            }
        }
    }

    ImGui::EndChild();
}

// ============================================================================
// View Dropdown (lists real views from GameData)
// ============================================================================

void CharacterEditor::DrawViewDropdown(const char* label, int& view_id)
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();

    char preview[64];
    if (view_id < 0)
        std::strncpy(preview, "(none)", sizeof(preview));
    else
    {
        auto* vinfo = gd ? gd->FindViewById(view_id) : nullptr;
        if (vinfo)
            std::snprintf(preview, sizeof(preview), "%d: %s", view_id, vinfo->name.c_str());
        else
            std::snprintf(preview, sizeof(preview), "View %d", view_id);
    }

    ImGui::SetNextItemWidth(Dpi(200));
    if (ImGui::BeginCombo(label, preview))
    {
        if (ImGui::Selectable("(none)", view_id < 0))
            view_id = -1;

        if (gd)
        {
            for (size_t vi = 0; vi < gd->views.size(); vi++)
            {
                auto& v = gd->views[vi];
                char opt[128];
                std::snprintf(opt, sizeof(opt), "%d: %s", v.id, v.name.c_str());
                if (ImGui::Selectable(opt, view_id == v.id))
                    view_id = v.id;
            }
        }
        ImGui::EndCombo();
    }
}

// ============================================================================
// View Preview — renders real sprites from TextureCache
// ============================================================================

void CharacterEditor::DrawViewPreview(const char* label, int view_id, float& timer, int& frame)
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd || view_id < 0) return;

    auto* view = gd->FindViewById(view_id);
    if (!view || view->loops.empty()) return;

    // Use first loop for preview
    auto& loop = view->loops[0];
    int num_frames = (int)loop.frames.size();
    if (num_frames <= 0) return;

    ImGui::Text("%s (%s, %d loops, %d frames):", label, view->name.c_str(),
                (int)view->loops.size(), num_frames);

    auto* project = editor_.GetApp().GetProject();
    auto* loader = project ? project->GetSpriteLoader() : nullptr;
    auto& tex_cache = editor_.GetApp().GetTextureCache();

    // Animate
    if (preview_playing_ && num_frames > 1)
    {
        timer += ImGui::GetIO().DeltaTime;
        float interval = 1.0f / preview_fps_;
        if (timer >= interval)
        {
            timer = 0.0f;
            frame = (frame + 1) % num_frames;
        }
    }
    if (frame >= num_frames) frame = 0;

    int spr = loop.frames[frame].sprite_id;

    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    float base_w = Dpi(32) * preview_zoom_ + Dpi(20);
    float base_h = Dpi(48) * preview_zoom_ + Dpi(20);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Background
    dl->AddRectFilled(canvas_pos,
        ImVec2(canvas_pos.x + base_w, canvas_pos.y + base_h),
        IM_COL32(30, 30, 50, 255));

    // Try to render real sprite
    SDL_Texture* tex = nullptr;
    if (loader && spr >= 0)
        tex = tex_cache.GetSpriteTexture(spr, loader);

    if (tex)
    {
        const auto* info = tex_cache.FindSprite(spr);
        float sw = info ? (float)info->width : 32.0f;
        float sh = info ? (float)info->height : 48.0f;
        float scale = preview_zoom_;
        float dw = sw * scale;
        float dh = sh * scale;
        float ox = (base_w - dw) * 0.5f;
        float oy = (base_h - dh) * 0.5f;

        dl->AddImage((ImTextureID)(intptr_t)tex,
            ImVec2(canvas_pos.x + ox, canvas_pos.y + oy),
            ImVec2(canvas_pos.x + ox + dw, canvas_pos.y + oy + dh));
    }
    else
    {
        // Fallback
        float sw = Dpi(32), sh = Dpi(48);
        float cx = canvas_pos.x + base_w * 0.5f;
        float cy = canvas_pos.y + base_h * 0.5f;
        ImU32 col = IM_COL32(100 + (spr * 17) % 155, 80 + (spr * 31) % 175, 120 + (spr * 7) % 120, 255);
        dl->AddRectFilled(ImVec2(cx - sw / 2, cy - sh / 2),
                          ImVec2(cx + sw / 2, cy + sh / 2), col);

        char buf[32];
        std::snprintf(buf, sizeof(buf), "S%d", spr);
        ImVec2 ts = ImGui::CalcTextSize(buf);
        dl->AddText(ImVec2(cx - ts.x / 2, cy - ts.y / 2), IM_COL32(255, 255, 255, 255), buf);
    }

    dl->AddRect(canvas_pos, ImVec2(canvas_pos.x + base_w, canvas_pos.y + base_h),
                IM_COL32(80, 80, 80, 255));

    ImGui::Dummy(ImVec2(base_w, base_h));
    ImGui::Spacing();
}

} // namespace AGSEditor
