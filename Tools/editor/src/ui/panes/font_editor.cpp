// AGS Editor ImGui - Font Editor
// Uses real font data from GameData. Loads TTF fonts for preview via ImGui.
// WFN bitmap fonts rendered natively using AGS Common WFNFont.
#include "font_editor.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "ui/file_dialog.h"
#include "core/dpi_helper.h"
#include "project/project.h"
#include "project/game_data.h"
#include "app.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "util/file.h"
#include <cstring>
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

namespace AGSEditor
{

FontEditor::FontEditor(EditorUI& editor, int font_id)
    : editor_(editor)
    , title_("Fonts")
{
    if (font_id >= 0)
        selected_font_ = font_id;
}

WFNFont* FontEditor::GetWFNFont(int font_id)
{
    auto it = wfn_cache_.find(font_id);
    if (it != wfn_cache_.end())
        return &it->second;

    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
        return nullptr;

    char wfn_name[64];
    snprintf(wfn_name, sizeof(wfn_name), "agsfnt%d.wfn", font_id);
    std::string wfn_path = (fs::path(project->GetProjectDir()) / wfn_name).string();
    if (!fs::exists(wfn_path))
        return nullptr;

    auto in = AGS::Common::File::OpenFileRead(wfn_path.c_str());
    if (!in)
        return nullptr;

    WFNFont font;
    WFNError err = font.ReadFromFile(in.get());
    if (err != kWFNErr_NoError && err != kWFNErr_HasBadCharacters)
        return nullptr;

    auto [pos, ok] = wfn_cache_.emplace(font_id, std::move(font));
    return &pos->second;
}

void FontEditor::DrawWFNChar(ImDrawList* dl, const WFNChar& ch,
    float x, float y, float scale, ImU32 color) const
{
    if (!ch.IsValid())
        return;
    size_t row_bytes = ch.GetRowByteCount();
    for (int py = 0; py < ch.Height; py++)
    {
        const uint8_t* row = ch.Data + py * row_bytes;
        for (int px = 0; px < ch.Width; px++)
        {
            if (row[px / 8] & (0x80 >> (px % 8)))
            {
                dl->AddRectFilled(
                    ImVec2(x + px * scale, y + py * scale),
                    ImVec2(x + (px + 1) * scale, y + (py + 1) * scale),
                    color);
            }
        }
    }
}

float FontEditor::DrawWFNText(ImDrawList* dl, const WFNFont& font,
    const char* text, float x, float y, float scale, ImU32 color,
    float max_width) const
{
    float cursor_x = x;
    for (const char* p = text; *p; p++)
    {
        unsigned char c = (unsigned char)*p;
        if (c >= font.GetCharCount())
            continue;
        const WFNChar& ch = font.GetChar(c);
        if (max_width > 0 && cursor_x + ch.Width * scale > x + max_width)
            break;
        DrawWFNChar(dl, ch, cursor_x, y, scale, color);
        cursor_x += ch.Width * scale;
    }
    return cursor_x - x;
}

void FontEditor::Draw()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
    {
        ImGui::TextDisabled("No project loaded. Open a game to view fonts.");
        return;
    }

    auto* gd = project->GetGameData();
    if (!gd || gd->fonts.empty())
    {
        ImGui::TextDisabled("No fonts in this game.");
        return;
    }

    float list_width = Dpi(180);
    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("FontList", ImVec2(list_width, avail.y), ImGuiChildFlags_Borders);
    DrawFontList();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("FontProps", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
    DrawFontProperties();
    ImGui::EndChild();
}

void FontEditor::DrawFontList()
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd) return;

    // New Font button
    if (ImGui::Button("New Font", ImVec2(-1, 0)))
    {
        GameData::FontInfo new_font;
        new_font.id = (int)gd->fonts.size();
        new_font.name = "Font " + std::to_string(new_font.id);
        new_font.size = 12;
        new_font.size_multiplier = 1;
        gd->fonts.push_back(new_font);
        selected_font_ = (int)gd->fonts.size() - 1;
        editor_.GetLogPanel().AddLog("[Font] Created font %d: %s", new_font.id, new_font.name.c_str());
    }

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%d fonts", (int)gd->fonts.size());
    ImGui::Separator();

    for (int i = 0; i < (int)gd->fonts.size(); i++)
    {
        auto& fnt = gd->fonts[i];
        char label[128];
        std::snprintf(label, sizeof(label), "%d: %s", fnt.id, fnt.name.c_str());
        if (ImGui::Selectable(label, selected_font_ == i))
            selected_font_ = i;

        // Context menu
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Duplicate"))
            {
                GameData::FontInfo dup = fnt;
                dup.id = (int)gd->fonts.size();
                dup.name = fnt.name + " (copy)";
                gd->fonts.push_back(dup);
                selected_font_ = (int)gd->fonts.size() - 1;
                editor_.GetLogPanel().AddLog("[Font] Duplicated font %d", fnt.id);
            }
            if (ImGui::MenuItem("Delete..."))
            {
                selected_font_ = i;
                confirm_delete_ = true;
            }
            ImGui::EndPopup();
        }
    }

    DrawContextMenu();
}

void FontEditor::DrawFontProperties()
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd || gd->fonts.empty() || selected_font_ < 0 || selected_font_ >= (int)gd->fonts.size())
    {
        ImGui::TextDisabled("No font selected.");
        return;
    }

    auto& fnt = gd->fonts[selected_font_];

    ImGui::Text("Font ID: %d", fnt.id);
    ImGui::Separator();

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SetNextItemWidth(Dpi(200));
        ImGui::InputText("Name", &fnt.name);

        ImGui::SetNextItemWidth(Dpi(200));
        ImGui::InputText("Source File", &fnt.source_file);
        ImGui::SameLine();
        if (ImGui::Button("Import..."))
        {
            FileDialog::Open(FileDialogType::OpenFile,
                "Import Font",
                "Font files{.ttf,.wfn}",
                ".",
                [this](const std::string& path) {
                    ImportFont(path);
                });
        }

        // TTF point size dialog
        if (show_ttf_size_dialog_)
        {
            ImGui::OpenPopup("TTF Point Size");
            show_ttf_size_dialog_ = false;
        }
        if (ImGui::BeginPopupModal("TTF Point Size", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Select the point size for this TTF font:");
            ImGui::SetNextItemWidth(Dpi(120));
            ImGui::InputInt("Point Size", &import_ttf_size_);
            if (import_ttf_size_ < 1) import_ttf_size_ = 1;
            if (import_ttf_size_ > 200) import_ttf_size_ = 200;
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(Dpi(80), 0)))
            {
                auto* gd2 = editor_.GetApp().GetProject()->GetGameData();
                if (gd2 && selected_font_ >= 0 && selected_font_ < (int)gd2->fonts.size())
                {
                    auto& f = gd2->fonts[selected_font_];
                    auto* proj = editor_.GetApp().GetProject();
                    std::string proj_dir = proj ? proj->GetProjectDir() : ".";

                    // Copy TTF to project dir
                    char dest_name[64];
                    snprintf(dest_name, sizeof(dest_name), "agsfnt%d.ttf", f.id);
                    fs::path dest = fs::path(proj_dir) / dest_name;
                    try {
                        fs::copy_file(import_ttf_path_, dest,
                            fs::copy_options::overwrite_existing);
                    } catch (const std::exception& e) {
                        fprintf(stderr, "[Font] Failed to copy TTF: %s\n", e.what());
                    }

                    // Delete WFN counterpart if it exists
                    char wfn_name[64];
                    snprintf(wfn_name, sizeof(wfn_name), "agsfnt%d.wfn", f.id);
                    fs::path wfn_path = fs::path(proj_dir) / wfn_name;
                    if (fs::exists(wfn_path))
                        fs::remove(wfn_path);

                    f.source_file = import_ttf_path_;
                    f.size = import_ttf_size_;
                    f.size_multiplier = 1;

                    // Reload fonts for preview
                    editor_.GetApp().LoadGameFonts();

                    editor_.GetLogPanel().AddLog("[Font] Imported TTF font %d: %s (size %d)",
                        f.id, import_ttf_path_.c_str(), import_ttf_size_);
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(Dpi(80), 0)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::SetNextItemWidth(Dpi(120));
        if (ImGui::InputInt("Point Size", &fnt.size))
        {
            if (fnt.size < 1) fnt.size = 1;
        }
        if (fnt.size < 1) fnt.size = 1;
        ImGui::SameLine();
        // Reimport with current point size - useful for TTF fonts
        {
            auto* proj = editor_.GetApp().GetProject();
            std::string proj_dir = proj ? proj->GetProjectDir() : ".";
            char ttf_name[64];
            snprintf(ttf_name, sizeof(ttf_name), "agsfnt%d.ttf", fnt.id);
            fs::path ttf_path = fs::path(proj_dir) / ttf_name;
            bool is_ttf = fs::exists(ttf_path);
            if (!is_ttf) ImGui::BeginDisabled();
            if (ImGui::Button("Reimport"))
            {
                // Reload the font atlas with the new point size
                editor_.GetApp().LoadGameFonts();
                editor_.GetLogPanel().AddLog("[Font] Reimported font %d with point size %d",
                    fnt.id, fnt.size);
            }
            if (!is_ttf) ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Reimport the TTF font with the current point size");
        }

        ImGui::SetNextItemWidth(Dpi(120));
        ImGui::InputInt("Size Multiplier", &fnt.size_multiplier);
        if (fnt.size_multiplier < 1) fnt.size_multiplier = 1;

        ImGui::SetNextItemWidth(Dpi(120));
        ImGui::InputInt("Line Spacing", &fnt.line_spacing);
        if (fnt.line_spacing < 0) fnt.line_spacing = 0;
    }

    if (ImGui::CollapsingHeader("Outline"))
    {
        const char* outline_names[] = {"None", "Automatic", "Use Font"};
        ImGui::SetNextItemWidth(Dpi(150));
        ImGui::Combo("Outline Type", &fnt.outline_type, outline_names, 3);
        if (fnt.outline_type == 2)
        {
            ImGui::SetNextItemWidth(Dpi(120));
            ImGui::InputInt("Outline Font", &fnt.outline_font);
            if (fnt.outline_font < 0) fnt.outline_font = 0;
            if (fnt.outline_font >= (int)gd->fonts.size())
                fnt.outline_font = (int)gd->fonts.size() - 1;
        }
    }

    DrawFontPreview();
    DrawCharacterGrid();
}

void FontEditor::DrawFontPreview()
{
    if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::InputText("Preview Text", preview_text_, sizeof(preview_text_));

        auto* project = editor_.GetApp().GetProject();
        auto* gd = project ? project->GetGameData() : nullptr;
        if (!gd || selected_font_ < 0 || selected_font_ >= (int)gd->fonts.size()) return;
        auto& fnt = gd->fonts[selected_font_];

        // Scaling factor for low-res games
        int scale = preview_scale_;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(Dpi(60));
        ImGui::InputInt("Scale", &preview_scale_);
        if (preview_scale_ < 1) preview_scale_ = 1;
        if (preview_scale_ > 8) preview_scale_ = 8;
        ImGui::SameLine();
        // Auto-detect from game resolution
        if (ImGui::SmallButton("Auto"))
        {
            // If the game is low-res (<=320 width), suggest 2x or 3x
            if (gd->resolution_width <= 320)
                preview_scale_ = 3;
            else if (gd->resolution_width <= 640)
                preview_scale_ = 2;
            else
                preview_scale_ = 1;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Auto-detect scale from game resolution (%dx%d)",
                gd->resolution_width, gd->resolution_height);
        scale = preview_scale_;

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float preview_h = (avail.y > Dpi(120)) ? Dpi(120) : avail.y;
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size(avail.x, preview_h);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        dl->AddRectFilled(canvas_pos,
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(30, 30, 45, 255));
        dl->AddRect(canvas_pos,
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(100, 100, 100, 255));

        // Check if this is a WFN bitmap font
        WFNFont* wfn_font = nullptr;
        bool is_wfn = false;
        if (project && project->IsLoaded())
        {
            char wfn_name[64];
            snprintf(wfn_name, sizeof(wfn_name), "agsfnt%d.wfn", fnt.id);
            if (fs::exists(fs::path(project->GetProjectDir()) / wfn_name))
            {
                is_wfn = true;
                wfn_font = GetWFNFont(fnt.id);
            }
        }

        // Try to get native TTF font
        int ttf_id = -1;
        if (!is_wfn && project && project->IsLoaded())
        {
            int pixel_size = fnt.size * fnt.size_multiplier;
            if (pixel_size > 0)
                ttf_id = GetTTFFont(fnt.id, pixel_size);
        }

        // Try to get ImGui fallback font
        ImFont* game_font = (ttf_id < 0) ? editor_.GetApp().GetGameFont(fnt.id) : nullptr;

        // Render preview
        float text_x = canvas_pos.x + Dpi(5);
        float text_y = canvas_pos.y + Dpi(5);

        if (wfn_font)
        {
            // Native WFN bitmap rendering
            bool draw_outline = (fnt.outline_type == 1);
            if (draw_outline)
            {
                ImU32 outline_col = IM_COL32(0, 0, 0, 255);
                float offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}};
                for (auto& off : offsets)
                {
                    DrawWFNText(dl, *wfn_font, preview_text_,
                        text_x + off[0] * scale, text_y + off[1] * scale,
                        (float)scale, outline_col, canvas_size.x - Dpi(10));
                }
            }
            DrawWFNText(dl, *wfn_font, preview_text_, text_x, text_y,
                (float)scale, IM_COL32(255, 255, 255, 255), canvas_size.x - Dpi(10));
        }
        else if (ttf_id >= 0)
        {
            // Native FreeType rendering
            bool draw_outline = (fnt.outline_type == 1);
            DrawTTFPreview(dl, ttf_id, preview_text_, text_x, text_y,
                           (float)scale, draw_outline, canvas_size.x - Dpi(10));
        }
        else if (game_font)
        {
            ImGui::PushFont(game_font);

            // Outline rendering simulation
            bool draw_outline = (fnt.outline_type == 1); // automatic outline
            if (draw_outline)
            {
                ImU32 outline_col = IM_COL32(0, 0, 0, 255);
                // Draw text at 8 surrounding offsets to simulate outline
                float offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}};
                for (auto& off : offsets)
                {
                    float ox = off[0] * (float)scale;
                    float oy = off[1] * (float)scale;
                    dl->AddText(game_font, game_font->LegacySize * scale,
                        ImVec2(text_x + ox, text_y + oy), outline_col, preview_text_);
                }
            }
            // Main text on top
            dl->AddText(game_font, game_font->LegacySize * scale,
                ImVec2(text_x, text_y), IM_COL32(255, 255, 255, 255), preview_text_);

            ImGui::PopFont();
        }
        else
        {
            ImGui::SetCursorScreenPos(ImVec2(text_x, text_y));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
            ImGui::TextWrapped("%s", preview_text_);
            ImGui::PopStyleColor();
        }

        // Info footer
        ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x + Dpi(5), canvas_pos.y + canvas_size.y - Dpi(20)));
        const char* outline_label = (fnt.outline_type == 0) ? "None"
            : (fnt.outline_type == 1) ? "Auto" : "Font";
        if (is_wfn)
            ImGui::TextDisabled("Font: %s | WFN bitmap%s | Height: %d | Outline: %s | Scale: %dx",
                fnt.name.c_str(),
                wfn_font ? "" : " (load failed)",
                wfn_font ? wfn_font->GetHeight() : fnt.size,
                outline_label, scale);
        else if (ttf_id >= 0)
            ImGui::TextDisabled("Font: %s | TTF (native) | Size: %d | Mul: %d | Outline: %s | Scale: %dx | Height: %dpx",
                fnt.name.c_str(), fnt.size, fnt.size_multiplier, outline_label, scale,
                ttf_renderer_.GetLineHeight(ttf_id) * scale);
        else if (game_font)
            ImGui::TextDisabled("Font: %s | TTF | Size: %d | Mul: %d | Outline: %s | Scale: %dx | Rendered: %.0fpx",
                fnt.name.c_str(), fnt.size, fnt.size_multiplier, outline_label, scale,
                game_font->LegacySize * scale);
        else
            ImGui::TextDisabled("Font: %s | Source: %s | Size: %d (not loaded)",
                fnt.name.c_str(), fnt.source_file.c_str(), fnt.size);
        ImGui::Dummy(ImVec2(0, Dpi(5)));
    }
}

void FontEditor::DrawCharacterGrid()
{
    if (!ImGui::CollapsingHeader("Character Grid"))
        return;

    auto* project = editor_.GetApp().GetProject();
    auto* gd = project ? project->GetGameData() : nullptr;
    if (!gd || selected_font_ < 0 || selected_font_ >= (int)gd->fonts.size()) return;
    auto& fnt = gd->fonts[selected_font_];

    ImFont* game_font = editor_.GetApp().GetGameFont(fnt.id);
    WFNFont* wfn_font = GetWFNFont(fnt.id);

    // Try native TTF rendering
    int ttf_id = -1;
    if (!wfn_font)
    {
        int pixel_size = fnt.size * fnt.size_multiplier;
        if (pixel_size > 0)
            ttf_id = GetTTFFont(fnt.id, pixel_size);
    }

    // Cell size slider
    ImGui::SetNextItemWidth(Dpi(150));
    ImGui::SliderInt("Cell Size", &grid_cell_size_, 20, 64);
    ImGui::SameLine();
    ImGui::Checkbox("Show Codes", &show_char_codes_);
    if (show_char_codes_)
    {
        ImGui::SameLine();
        ImGui::Checkbox("Hex", &show_hex_codes_);
    }

    // ANSI / Unicode mode toggle
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    if (ImGui::RadioButton("ANSI", !unicode_mode_))
    {
        unicode_mode_ = false;
        unicode_range_start_ = 0;
        unicode_range_end_ = 0xFF;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Unicode", unicode_mode_))
    {
        unicode_mode_ = true;
        show_hex_codes_ = true;
        if (unicode_range_end_ <= 0xFF)
        {
            unicode_range_start_ = 0;
            unicode_range_end_ = 0x7FF;
        }
    }

    // Unicode range inputs
    if (unicode_mode_)
    {
        ImGui::SetNextItemWidth(Dpi(80));
        ImGui::InputInt("Range Start (hex)##RangeStart", &unicode_range_start_, 0x10, 0x100);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(Dpi(80));
        ImGui::InputInt("Range End##RangeEnd", &unicode_range_end_, 0x10, 0x100);
        if (unicode_range_start_ < 0) unicode_range_start_ = 0;
        if (unicode_range_end_ < unicode_range_start_) unicode_range_end_ = unicode_range_start_;
        if (unicode_range_end_ > 0xFFFF) unicode_range_end_ = 0xFFFF;

        // Quick range presets
        ImGui::SameLine();
        if (ImGui::SmallButton("Latin")) { unicode_range_start_ = 0; unicode_range_end_ = 0x24F; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Cyrillic")) { unicode_range_start_ = 0x400; unicode_range_end_ = 0x4FF; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Greek")) { unicode_range_start_ = 0x370; unicode_range_end_ = 0x3FF; }
        ImGui::SameLine();
        if (ImGui::SmallButton("CJK")) { unicode_range_start_ = 0x4E00; unicode_range_end_ = 0x9FFF; }
    }

    int range_start = unicode_mode_ ? unicode_range_start_ : 0;
    int range_end = unicode_mode_ ? unicode_range_end_ : 255;
    int char_count = range_end - range_start + 1;

    // Character code input
    ImGui::SetNextItemWidth(Dpi(80));
    if (show_hex_codes_ || unicode_mode_)
    {
        if (ImGui::InputText("Code (hex)##CharCode", char_code_input_,
            sizeof(char_code_input_), ImGuiInputTextFlags_CharsHexadecimal
            | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            int val = (int)strtol(char_code_input_, nullptr, 16);
            if (val >= range_start && val <= range_end) selected_char_ = val;
        }
    }
    else
    {
        if (ImGui::InputText("Code##CharCode", char_code_input_,
            sizeof(char_code_input_), ImGuiInputTextFlags_CharsDecimal
            | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            int val = atoi(char_code_input_);
            if (val >= range_start && val <= range_end) selected_char_ = val;
        }
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(Dpi(40));
    if (ImGui::InputText("Char##CharText", char_text_input_,
        sizeof(char_text_input_), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (char_text_input_[0] != '\0')
        {
            // Decode UTF-8 to get codepoint
            unsigned char c0 = (unsigned char)char_text_input_[0];
            int cp = c0;
            if (c0 >= 0xC0 && c0 < 0xE0 && char_text_input_[1])
                cp = ((c0 & 0x1F) << 6) | ((unsigned char)char_text_input_[1] & 0x3F);
            else if (c0 >= 0xE0 && c0 < 0xF0 && char_text_input_[1] && char_text_input_[2])
                cp = ((c0 & 0x0F) << 12) | (((unsigned char)char_text_input_[1] & 0x3F) << 6)
                    | ((unsigned char)char_text_input_[2] & 0x3F);
            selected_char_ = cp;
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Go To") && selected_char_ >= range_start && selected_char_ <= range_end)
        scroll_to_char_ = true;

    // Sync code input fields with selected char
    if (selected_char_ >= range_start && selected_char_ <= range_end)
    {
        if (show_hex_codes_ || unicode_mode_)
            snprintf(char_code_input_, sizeof(char_code_input_), "%04X", selected_char_);
        else
            snprintf(char_code_input_, sizeof(char_code_input_), "%d", selected_char_);
        if (selected_char_ >= 32 && selected_char_ < 127)
        {
            char_text_input_[0] = (char)selected_char_;
            char_text_input_[1] = '\0';
        }
        else
        {
            char_text_input_[0] = '\0';
        }
    }

    // Show selected character info
    if (selected_char_ >= range_start && selected_char_ <= range_end)
    {
        char info[128];
        if (unicode_mode_)
            snprintf(info, sizeof(info), "Selected: U+%04X | Dec: %d", selected_char_, selected_char_);
        else if (selected_char_ >= 32 && selected_char_ < 127)
            snprintf(info, sizeof(info), "Selected: '%c' | Dec: %d | Hex: 0x%02X",
                (char)selected_char_, selected_char_, selected_char_);
        else
            snprintf(info, sizeof(info), "Selected: (0x%02X) | Dec: %d | Hex: 0x%02X",
                selected_char_, selected_char_, selected_char_);
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", info);
    }
    else
    {
        ImGui::TextDisabled("Click a cell to select a character");
    }

    // Draw the grid
    float cell = Dpi((float)grid_cell_size_);
    ImVec2 avail = ImGui::GetContentRegionAvail();
    int cols = std::max(1, (int)(avail.x / cell));
    int rows = (char_count + cols - 1) / cols;

    ImGui::BeginChild("##CharGrid", ImVec2(0, 0), ImGuiChildFlags_Borders);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Handle scroll-to-char request
    if (scroll_to_char_ && selected_char_ >= range_start && selected_char_ <= range_end)
    {
        int char_idx = selected_char_ - range_start;
        int target_row = char_idx / cols;
        float scroll_y = target_row * cell;
        ImGui::SetScrollY(scroll_y);
        scroll_to_char_ = false;
    }

    if (game_font && !wfn_font && ttf_id < 0) ImGui::PushFont(game_font);

    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < cols; col++)
        {
            int idx = row * cols + col;
            if (idx >= char_count) break;
            int ch = range_start + idx;
            if (col > 0) ImGui::SameLine(0, 0);

            ImVec2 pos = ImGui::GetCursorScreenPos();
            bool selected = (selected_char_ == ch);

            // Background
            ImU32 bg_col = selected ? IM_COL32(60, 100, 180, 255) : IM_COL32(40, 40, 55, 255);
            dl->AddRectFilled(pos, ImVec2(pos.x + cell - 1, pos.y + cell - 1), bg_col);

            // Border
            ImU32 border_col = selected ? IM_COL32(100, 160, 255, 255) : IM_COL32(70, 70, 90, 255);
            dl->AddRect(pos, ImVec2(pos.x + cell - 1, pos.y + cell - 1), border_col);

            // Character glyph
            if (wfn_font && ch < wfn_font->GetCharCount())
            {
                // Native WFN bitmap glyph rendering
                const WFNChar& wch = wfn_font->GetChar((uint16_t)ch);
                if (wch.IsValid())
                {
                    // Center glyph in cell
                    float gx = pos.x + (cell - 1 - wch.Width) * 0.5f;
                    float gy = pos.y + (cell - 1 - wch.Height) * 0.5f;
                    DrawWFNChar(dl, wch, gx, gy, 1.0f, IM_COL32(255, 255, 255, 255));
                }
            }
            else if (ttf_id >= 0 && ch >= 32)
            {
                // Native FreeType glyph rendering
                DrawTTFGlyph(dl, ttf_id, ch, pos.x, pos.y, cell);
            }
            else if (ch >= 32)
            {
                char glyph[5] = {};
                if (ch < 0x80)
                {
                    glyph[0] = (char)ch;
                }
                else if (ch < 0x800)
                {
                    glyph[0] = (char)(0xC0 | (ch >> 6));
                    glyph[1] = (char)(0x80 | (ch & 0x3F));
                }
                else
                {
                    glyph[0] = (char)(0xE0 | (ch >> 12));
                    glyph[1] = (char)(0x80 | ((ch >> 6) & 0x3F));
                    glyph[2] = (char)(0x80 | (ch & 0x3F));
                }
                ImVec2 text_size = ImGui::CalcTextSize(glyph);
                float tx = pos.x + (cell - 1 - text_size.x) * 0.5f;
                float ty = pos.y + (cell - 1 - text_size.y) * 0.5f;
                dl->AddText(ImVec2(tx, ty), IM_COL32(255, 255, 255, 255), glyph);
            }

            // Character code overlay
            if (show_char_codes_)
            {
                char code[16];
                if (show_hex_codes_ || unicode_mode_)
                    snprintf(code, sizeof(code), "%02X", ch);
                else
                    snprintf(code, sizeof(code), "%d", ch);
                // Use default font for the code label
                if (game_font && !wfn_font && ttf_id < 0) ImGui::PopFont();
                ImVec2 code_size = ImGui::CalcTextSize(code);
                float cx = pos.x + cell - 2 - code_size.x;
                float cy = pos.y + cell - 2 - code_size.y;
                dl->AddRectFilled(ImVec2(cx - 1, cy),
                    ImVec2(cx + code_size.x + 1, cy + code_size.y),
                    IM_COL32(0, 0, 0, 180));
                dl->AddText(ImVec2(cx, cy), IM_COL32(200, 200, 200, 255), code);
                if (game_font && !wfn_font && ttf_id < 0) ImGui::PushFont(game_font);
            }

            // Invisible button for click handling
            char btn_id[32];
            snprintf(btn_id, sizeof(btn_id), "##ch%d", ch);
            ImGui::InvisibleButton(btn_id, ImVec2(cell, cell));
            if (ImGui::IsItemClicked())
                selected_char_ = ch;

            // Tooltip
            if (ImGui::IsItemHovered())
            {
                if (unicode_mode_)
                    ImGui::SetTooltip("U+%04X (Dec: %d)", ch, ch);
                else if (ch >= 32 && ch < 127)
                    ImGui::SetTooltip("'%c' (Dec: %d, Hex: 0x%02X)", (char)ch, ch, ch);
                else
                    ImGui::SetTooltip("Dec: %d, Hex: 0x%02X", ch, ch);
            }
        }
    }

    if (game_font && !wfn_font && ttf_id < 0) ImGui::PopFont();
    ImGui::EndChild();
}

void FontEditor::DrawContextMenu()
{
    if (confirm_delete_)
    {
        ImGui::OpenPopup("Delete Font?");
        confirm_delete_ = false;
    }

    if (ImGui::BeginPopupModal("Delete Font?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto* gd = editor_.GetApp().GetProject()->GetGameData();
        if (gd && selected_font_ >= 0 && selected_font_ < (int)gd->fonts.size())
        {
            auto& fnt = gd->fonts[selected_font_];
            ImGui::Text("Delete font %d: '%s'?", fnt.id, fnt.name.c_str());
            ImGui::TextWrapped("Characters and GUIs using this font may display incorrectly.");
            ImGui::Separator();
            if (ImGui::Button("Delete", ImVec2(Dpi(100), 0)))
            {
                DeleteFont(selected_font_);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(Dpi(100), 0)))
                ImGui::CloseCurrentPopup();
        }
        else
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void FontEditor::DeleteFont(int index)
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd || index < 0 || index >= (int)gd->fonts.size()) return;

    editor_.GetLogPanel().AddLog("[Font] Deleted font %d: %s", gd->fonts[index].id, gd->fonts[index].name.c_str());
    gd->fonts.erase(gd->fonts.begin() + index);
    // Re-number IDs
    for (int i = 0; i < (int)gd->fonts.size(); i++)
        gd->fonts[i].id = i;
    if (selected_font_ >= (int)gd->fonts.size())
        selected_font_ = (int)gd->fonts.size() - 1;
}

void FontEditor::ImportFont(const std::string& path)
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd || selected_font_ < 0 || selected_font_ >= (int)gd->fonts.size()) return;
    auto& fnt = gd->fonts[selected_font_];
    auto* project = editor_.GetApp().GetProject();
    std::string proj_dir = project ? project->GetProjectDir() : ".";

    fs::path fpath(path);
    std::string ext = fpath.extension().string();
    // Lowercase the extension
    for (auto& c : ext) c = (char)std::tolower((unsigned char)c);

    if (ext == ".ttf")
    {
        // Show size dialog for TTF
        import_ttf_path_ = path;
        import_ttf_size_ = fnt.size > 0 ? fnt.size : 10;
        show_ttf_size_dialog_ = true;
    }
    else if (ext == ".wfn")
    {
        // WFN bitmap font — copy directly
        char dest_name[64];
        snprintf(dest_name, sizeof(dest_name), "agsfnt%d.wfn", fnt.id);
        fs::path dest = fs::path(proj_dir) / dest_name;
        try {
            fs::copy_file(path, dest, fs::copy_options::overwrite_existing);
        } catch (const std::exception& e) {
            fprintf(stderr, "[Font] Failed to copy WFN: %s\n", e.what());
            return;
        }

        // Delete TTF counterpart if it exists
        char ttf_name[64];
        snprintf(ttf_name, sizeof(ttf_name), "agsfnt%d.ttf", fnt.id);
        fs::path ttf_path = fs::path(proj_dir) / ttf_name;
        if (fs::exists(ttf_path))
            fs::remove(ttf_path);

        fnt.source_file = path;
        fnt.size = 0;
        fnt.size_multiplier = 1;

        // Reload fonts for preview
        editor_.GetApp().LoadGameFonts();

        editor_.GetLogPanel().AddLog("[Font] Imported WFN font %d: %s", fnt.id, path.c_str());
    }
    else
    {
        fprintf(stderr, "[Font] Unsupported font format: %s\n", ext.c_str());
    }
}

// =========================================================================
// Native TTF rendering helpers
// =========================================================================

int FontEditor::GetTTFFont(int font_id, int pixel_size)
{
    if (pixel_size <= 0)
        return -1;

    int cache_key = font_id * 1000 + pixel_size;
    auto it = ttf_cache_.find(cache_key);
    if (it != ttf_cache_.end())
        return it->second;

    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
        return -1;

    char ttf_name[64];
    snprintf(ttf_name, sizeof(ttf_name), "agsfnt%d.ttf", font_id);
    std::string ttf_path = (fs::path(project->GetProjectDir()) / ttf_name).string();
    if (!fs::exists(ttf_path))
        return -1;

    int id = ttf_renderer_.LoadFont(ttf_path, pixel_size);
    if (id >= 0)
        ttf_cache_[cache_key] = id;
    return id;
}

void FontEditor::DrawTTFPreview(ImDrawList* dl, int ttf_id, const char* text,
                                 float x, float y, float scale,
                                 bool outline, float max_width)
{
    SDL_Renderer* renderer = editor_.GetApp().GetRenderer();
    if (!renderer)
        return;

    RenderedText rt;
    int max_w = max_width > 0 ? (int)(max_width / scale) : 0;

    if (outline)
    {
        rt = ttf_renderer_.RenderTextOutlined(ttf_id, text,
            0xFFFFFFFF, 0xFF000000, 1, max_w);
    }
    else
    {
        rt = ttf_renderer_.RenderText(ttf_id, text, 0xFFFFFFFF, max_w);
    }

    if (rt.pixels.empty())
        return;

    SDL_Texture* tex = ttf_renderer_.UploadToTexture(renderer, rt);
    if (!tex)
        return;

    float dw = rt.width * scale;
    float dh = rt.height * scale;
    dl->AddImage((ImTextureID)(intptr_t)tex,
        ImVec2(x, y), ImVec2(x + dw, y + dh));

    // Schedule texture for cleanup next frame (simple approach:
    // use a small list of recently created textures)
    // For now, destroy immediately after draw list submission won't work —
    // we need to keep it alive until ImGui renders. Use a static ring buffer.
    static SDL_Texture* prev_textures[32] = {};
    static int prev_idx = 0;
    if (prev_textures[prev_idx])
        SDL_DestroyTexture(prev_textures[prev_idx]);
    prev_textures[prev_idx] = tex;
    prev_idx = (prev_idx + 1) % 32;
}

void FontEditor::DrawTTFGlyph(ImDrawList* dl, int ttf_id, int codepoint,
                                float cell_x, float cell_y, float cell_size)
{
    SDL_Renderer* renderer = editor_.GetApp().GetRenderer();
    if (!renderer)
        return;

    RenderedText rt = ttf_renderer_.RenderGlyph(ttf_id, codepoint, 0xFFFFFFFF);
    if (rt.pixels.empty() || rt.width <= 0 || rt.height <= 0)
        return;

    SDL_Texture* tex = ttf_renderer_.UploadToTexture(renderer, rt);
    if (!tex)
        return;

    // Scale glyph to fit in cell
    float avail = cell_size - 2;
    float scale = std::min(avail / (float)rt.width, avail / (float)rt.height);
    if (scale > 1.0f) scale = 1.0f; // don't upscale
    float dw = rt.width * scale;
    float dh = rt.height * scale;
    float gx = cell_x + (cell_size - 1 - dw) * 0.5f;
    float gy = cell_y + (cell_size - 1 - dh) * 0.5f;

    dl->AddImage((ImTextureID)(intptr_t)tex,
        ImVec2(gx, gy), ImVec2(gx + dw, gy + dh));

    // Ring buffer cleanup
    static SDL_Texture* glyph_textures[64] = {};
    static int glyph_idx = 0;
    if (glyph_textures[glyph_idx])
        SDL_DestroyTexture(glyph_textures[glyph_idx]);
    glyph_textures[glyph_idx] = tex;
    glyph_idx = (glyph_idx + 1) % 64;
}

} // namespace AGSEditor
