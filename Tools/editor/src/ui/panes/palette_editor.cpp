// AGS Editor ImGui - Palette Editor pane implementation
#include "palette_editor.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "ui/file_dialog.h"
#include "core/dpi_helper.h"
#include "project/project.h"
#include "project/game_data.h"
#include "app.h"
#include "imgui.h"
#include <SDL.h>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <algorithm>

namespace AGSEditor
{

PaletteEditor::PaletteEditor(EditorUI& editor)
    : editor_(editor)
{
    // Try to load palette from project GameData
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (gd && !gd->palette.empty())
    {
        int count = std::min((int)gd->palette.size(), 256);
        for (int i = 0; i < count; i++)
        {
            colors_[i].r = gd->palette[i].r;
            colors_[i].g = gd->palette[i].g;
            colors_[i].b = gd->palette[i].b;
            colors_[i].colour_type = gd->palette[i].colour_type;
        }
        // Zero-fill remaining slots
        for (int i = count; i < 256; i++)
        {
            colors_[i].r = 0; colors_[i].g = 0; colors_[i].b = 0;
            colors_[i].colour_type = 2; // Background
        }
    }
    else
    {
        // Fallback: generate a default VGA-like palette
        for (int i = 0; i < 256; i++)
        {
            if (i < 16)
            {
                static const int ega[16][3] = {
                    {0,0,0}, {0,0,170}, {0,170,0}, {0,170,170},
                    {170,0,0}, {170,0,170}, {170,85,0}, {170,170,170},
                    {85,85,85}, {85,85,255}, {85,255,85}, {85,255,255},
                    {255,85,85}, {255,85,255}, {255,255,85}, {255,255,255}
                };
                colors_[i].r = ega[i][0];
                colors_[i].g = ega[i][1];
                colors_[i].b = ega[i][2];
            }
            else if (i < 64)
            {
                int v = (i - 16) * 255 / 47;
                colors_[i].r = v; colors_[i].g = v; colors_[i].b = v;
            }
            else
            {
                int sector = (i - 64) / 32;
                int pos = ((i - 64) % 32) * 8;
                switch (sector % 6)
                {
                case 0: colors_[i].r = 255;     colors_[i].g = pos;     colors_[i].b = 0; break;
                case 1: colors_[i].r = 255-pos;  colors_[i].g = 255;     colors_[i].b = 0; break;
                case 2: colors_[i].r = 0;        colors_[i].g = 255;     colors_[i].b = pos; break;
                case 3: colors_[i].r = 0;        colors_[i].g = 255-pos;  colors_[i].b = 255; break;
                case 4: colors_[i].r = pos;      colors_[i].g = 0;        colors_[i].b = 255; break;
                case 5: colors_[i].r = 255;      colors_[i].g = 0;        colors_[i].b = 255-pos; break;
                }
            }
            colors_[i].colour_type = (i < 64) ? 0 : 2; // Gamewide / Background
        }
    }
}

void PaletteEditor::Draw()
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float props_width = Dpi(260);

    // Color depth awareness header
    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    int color_depth = gd ? gd->color_depth : 32;

    if (color_depth == 8)
    {
        ImGui::TextWrapped("Your game's palette is shown below. "
            "In 8-bit mode, the palette defines all available colors.");
    }
    else
    {
        ImGui::TextWrapped("This palette information will only be used for "
            "drawing any 8-bit graphics that you may have imported. "
            "Your game uses %d-bit color.", color_depth);
    }
    ImGui::Spacing();

    ImGui::BeginChild("PalGrid", ImVec2(avail.x - props_width - Dpi(8), avail.y - ImGui::GetCursorPosY()), ImGuiChildFlags_Borders);
    DrawPaletteTools();
    ImGui::Separator();
    DrawPaletteGrid();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("PalProps", ImVec2(0, avail.y - ImGui::GetCursorPosY()), ImGuiChildFlags_Borders);
    DrawColorProperties();

    // Color Finder section (only for hi/true color modes)
    if (color_depth > 8)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Color Finder");
        ImGui::Separator();

        // RGB sliders for arbitrary color lookup
        static float finder_col[3] = { 1.0f, 1.0f, 1.0f };
        ImGui::ColorPicker3("##colorfinder", finder_col,
            ImGuiColorEditFlags_PickerHueWheel |
            ImGuiColorEditFlags_InputRGB);

        int fr = (int)(finder_col[0] * 255.0f + 0.5f);
        int fg = (int)(finder_col[1] * 255.0f + 0.5f);
        int fb = (int)(finder_col[2] * 255.0f + 0.5f);

        // AGS color number (packed 16-bit R5G6B5)
        int ags_num = ((fr >> 3) << 11) | ((fg >> 2) << 5) | (fb >> 3);
        ImGui::Text("AGS color number: %d", ags_num);

        // Show clamped values for 16-bit mode
        if (color_depth == 16)
        {
            // Clamp to 5-6-5 and show what the engine will actually use
            int cr = ((fr >> 3) * 255 + 15) / 31;
            int cg = ((fg >> 2) * 255 + 31) / 63;
            int cb = ((fb >> 3) * 255 + 15) / 31;
            ImGui::Text("RGB: %d, %d, %d  (clamped: %d, %d, %d)",
                        fr, fg, fb, cr, cg, cb);
            // Show the clamped color swatch
            ImVec2 sp = ImGui::GetCursorScreenPos();
            float sw = Dpi(40);
            ImGui::GetWindowDrawList()->AddRectFilled(sp,
                ImVec2(sp.x + sw, sp.y + sw), IM_COL32(cr, cg, cb, 255));
            ImGui::GetWindowDrawList()->AddRect(sp,
                ImVec2(sp.x + sw, sp.y + sw), IM_COL32(180, 180, 180, 255));
            ImGui::Dummy(ImVec2(sw, sw));
        }
        else
        {
            ImGui::Text("RGB: %d, %d, %d", fr, fg, fb);
        }

        // Hex value
        ImGui::Text("Hex: #%02X%02X%02X", fr, fg, fb);

        // Input AGS color number
        static int input_ags_color = 0;
        ImGui::SetNextItemWidth(Dpi(100));
        if (ImGui::InputInt("AGS Color##finder", &input_ags_color))
        {
            if (input_ags_color < 0) input_ags_color = 0;
            if (input_ags_color > 65535) input_ags_color = 65535;
            finder_col[0] = (float)(((input_ags_color >> 11) & 0x1F) * 255 / 31) / 255.0f;
            finder_col[1] = (float)(((input_ags_color >> 5) & 0x3F) * 255 / 63) / 255.0f;
            finder_col[2] = (float)((input_ags_color & 0x1F) * 255 / 31) / 255.0f;
        }
    }

    ImGui::EndChild();

    // Sync palette changes back to GameData
    if (gd)
    {
        if (gd->palette.size() != 256)
            gd->palette.resize(256);
        for (int i = 0; i < 256; i++)
        {
            gd->palette[i].r = colors_[i].r;
            gd->palette[i].g = colors_[i].g;
            gd->palette[i].b = colors_[i].b;
            gd->palette[i].colour_type = colors_[i].colour_type;
        }
    }
}

void PaletteEditor::DrawPaletteGrid()
{
    ImGui::SliderFloat("Cell Size", &grid_cell_size_, 8.0f, 32.0f);

    // Reserve space for row labels
    float label_w = ImGui::CalcTextSize("240").x + 4.0f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float avail_w = ImGui::GetContentRegionAvail().x - label_w;

    int cols = (int)(avail_w / grid_cell_size_);
    if (cols < 1) cols = 1;
    int rows = (256 + cols - 1) / cols;

    // Grid origin offset for row labels
    float grid_x0 = pos.x + label_w;

    for (int i = 0; i < 256; i++)
    {
        int col = i % cols;
        int row = i / cols;
        float x = grid_x0 + col * grid_cell_size_;
        float y = pos.y + row * grid_cell_size_;

        ImU32 fill = IM_COL32(colors_[i].r, colors_[i].g, colors_[i].b, 255);
        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + grid_cell_size_ - 1, y + grid_cell_size_ - 1), fill);

        // Draw marks for non-gamewide palette entries
        if (colors_[i].colour_type == 2) // Background
        {
            ImU32 xcolor = IM_COL32(255, 255, 255, 180);
            // Small contrasting X in top-left corner
            float m = grid_cell_size_ * 0.15f;
            float s = grid_cell_size_ * 0.35f;
            dl->AddLine(ImVec2(x + m, y + m), ImVec2(x + m + s, y + m + s), xcolor, 1.5f);
            dl->AddLine(ImVec2(x + m + s, y + m), ImVec2(x + m, y + m + s), xcolor, 1.5f);
        }
        else if (colors_[i].colour_type == 1) // Locked
        {
            ImU32 lcolor = IM_COL32(255, 255, 255, 180);
            // Small "L" in top-left corner
            float m = grid_cell_size_ * 0.18f;
            float s = grid_cell_size_ * 0.30f;
            dl->AddLine(ImVec2(x + m, y + m), ImVec2(x + m, y + m + s), lcolor, 1.5f);
            dl->AddLine(ImVec2(x + m, y + m + s), ImVec2(x + m + s * 0.7f, y + m + s), lcolor, 1.5f);
        }

        if (i == selected_color_)
        {
            dl->AddRect(ImVec2(x - 1, y - 1),
                ImVec2(x + grid_cell_size_, y + grid_cell_size_),
                IM_COL32(255, 255, 0, 255), 0, 0, 2.0f);
        }
        else if (IsSelected(i))
        {
            dl->AddRect(ImVec2(x - 1, y - 1),
                ImVec2(x + grid_cell_size_, y + grid_cell_size_),
                IM_COL32(200, 200, 0, 200), 0, 0, 1.5f);
        }
    }

    // Draw row labels (first color index of each row)
    for (int row = 0; row < rows; row++)
    {
        int first_idx = row * cols;
        float y = pos.y + row * grid_cell_size_;
        char lbl[8];
        snprintf(lbl, sizeof(lbl), "%d", first_idx);
        float text_h = ImGui::GetTextLineHeight();
        float label_y = y + (grid_cell_size_ - text_h) * 0.5f;
        dl->AddText(ImVec2(pos.x, label_y), IM_COL32(180, 180, 180, 255), lbl);
    }

    // Handle click
    float total_h = rows * grid_cell_size_;
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton("palette_grid", ImVec2(avail_w + label_w, total_h));
    if (ImGui::IsItemClicked())
    {
        ImVec2 mouse = ImGui::GetMousePos();
        int col = (int)((mouse.x - grid_x0) / grid_cell_size_);
        int row = (int)((mouse.y - pos.y) / grid_cell_size_);
        if (col >= 0 && col < cols)
        {
            int idx = row * cols + col;
            if (idx >= 0 && idx < 256)
            {
                ImGuiIO& io = ImGui::GetIO();
                if (io.KeyCtrl)
                {
                    // Toggle selection
                    if (IsSelected(idx))
                        selected_colors_.erase(idx);
                    else
                        selected_colors_.insert(idx);
                    selected_color_ = idx;
                }
                else if (io.KeyShift && selected_color_ >= 0)
                {
                    // Range select
                    int lo = std::min(selected_color_, idx);
                    int hi = std::max(selected_color_, idx);
                    for (int s = lo; s <= hi; s++)
                        selected_colors_.insert(s);
                    selected_color_ = idx;
                }
                else
                {
                    // Single select
                    selected_colors_.clear();
                    selected_colors_.insert(idx);
                    selected_color_ = idx;
                }
            }
        }
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        ImVec2 mouse = ImGui::GetMousePos();
        int col = (int)((mouse.x - grid_x0) / grid_cell_size_);
        int row = (int)((mouse.y - pos.y) / grid_cell_size_);
        if (col >= 0 && col < cols)
        {
            int idx = row * cols + col;
            if (idx >= 0 && idx < 256)
            {
                if (!IsSelected(idx))
                {
                    selected_colors_.clear();
                    selected_colors_.insert(idx);
                }
                selected_color_ = idx;
            }
        }
        ImGui::OpenPopup("PaletteCtx");
    }
    if (ImGui::BeginPopup("PaletteCtx"))
    {
        int sel_count = (int)selected_colors_.size();
        if (sel_count > 1)
            ImGui::Text("%d colors selected", sel_count);
        else
            ImGui::Text("Color %d", selected_color_);
        ImGui::Separator();

        if (ImGui::MenuItem("Copy Color"))
        {
            copied_color_ = colors_[selected_color_];
            has_copied_color_ = true;
        }
        if (ImGui::MenuItem("Paste Color", nullptr, false, has_copied_color_))
        {
            for (int idx : selected_colors_)
            {
                colors_[idx].r = copied_color_.r;
                colors_[idx].g = copied_color_.g;
                colors_[idx].b = copied_color_.b;
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Set as Gamewide"))
            for (int idx : selected_colors_) colors_[idx].colour_type = 0;
        if (ImGui::MenuItem("Set as Locked"))
            for (int idx : selected_colors_) colors_[idx].colour_type = 1;
        if (ImGui::MenuItem("Set as Background"))
            for (int idx : selected_colors_) colors_[idx].colour_type = 2;
        ImGui::Separator();
        if (ImGui::MenuItem("Set to Black"))
            for (int idx : selected_colors_)
            { colors_[idx].r = 0; colors_[idx].g = 0; colors_[idx].b = 0; }
        if (ImGui::MenuItem("Set to White"))
            for (int idx : selected_colors_)
            { colors_[idx].r = 255; colors_[idx].g = 255; colors_[idx].b = 255; }

        ImGui::Separator();
        if (ImGui::MenuItem("Replace Selected from File..."))
        {
            replace_selected_pending_ = true;
        }

        ImGui::EndPopup();
    }

    // Handle replace-selected-from-file dialog (must be outside popup)
    if (replace_selected_pending_)
    {
        replace_selected_pending_ = false;
        FileDialog::Open(FileDialogType::OpenFile, "Replace Selected Palette Slots",
            ".pal,.bmp,.gpl,.txt", ".",
            [this](const std::string& path) { ReplaceSelectedFromFile(path); });
    }
}

void PaletteEditor::DrawColorProperties()
{
    ImGui::Text("Color %d", selected_color_);
    ImGui::Separator();

    auto& c = colors_[selected_color_];

    // Color preview swatch
    ImVec2 swatch_pos = ImGui::GetCursorScreenPos();
    float swatch_size = Dpi(60);
    ImGui::GetWindowDrawList()->AddRectFilled(swatch_pos,
        ImVec2(swatch_pos.x + swatch_size, swatch_pos.y + swatch_size),
        IM_COL32(c.r, c.g, c.b, 255));
    ImGui::GetWindowDrawList()->AddRect(swatch_pos,
        ImVec2(swatch_pos.x + swatch_size, swatch_pos.y + swatch_size),
        IM_COL32(200, 200, 200, 255));
    ImGui::Dummy(ImVec2(swatch_size, swatch_size));

    ImGui::Spacing();

    // RGB sliders
    ImGui::SliderInt("Red", &c.r, 0, 255);
    ImGui::SliderInt("Green", &c.g, 0, 255);
    ImGui::SliderInt("Blue", &c.b, 0, 255);

    // Hex input (editable)
    char hex[8];
    std::snprintf(hex, sizeof(hex), "#%02X%02X%02X", c.r, c.g, c.b);
    ImGui::SetNextItemWidth(Dpi(90));
    if (ImGui::InputText("Hex", hex, sizeof(hex), ImGuiInputTextFlags_CharsUppercase))
    {
        // Parse hex input (accept #RRGGBB or RRGGBB)
        const char* p = hex;
        if (*p == '#') p++;
        unsigned int val = 0;
        if (std::sscanf(p, "%06X", &val) == 1)
        {
            c.r = (val >> 16) & 0xFF;
            c.g = (val >> 8) & 0xFF;
            c.b = val & 0xFF;
        }
    }

    // AGS color number (packed 16-bit: RRRRRGGGGGGBBBBB, 5-6-5)
    int ags_color = ((c.r >> 3) << 11) | ((c.g >> 2) << 5) | (c.b >> 3);
    ImGui::SetNextItemWidth(Dpi(90));
    if (ImGui::InputInt("AGS Color", &ags_color))
    {
        if (ags_color < 0) ags_color = 0;
        if (ags_color > 65535) ags_color = 65535;
        c.r = ((ags_color >> 11) & 0x1F) * 255 / 31;
        c.g = ((ags_color >> 5) & 0x3F) * 255 / 63;
        c.b = (ags_color & 0x1F) * 255 / 31;
    }

    // Full color picker widget
    ImGui::Spacing();
    float col3[3] = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f };
    if (ImGui::ColorPicker3("##picker", col3,
        ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview))
    {
        c.r = (int)(col3[0] * 255.0f + 0.5f);
        c.g = (int)(col3[1] * 255.0f + 0.5f);
        c.b = (int)(col3[2] * 255.0f + 0.5f);
    }

    ImGui::Separator();

    // Colour type selector
    const char* colour_types[] = { "Gamewide", "Locked", "Background" };
    ImGui::Combo("Colour Type", &c.colour_type, colour_types, 3);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Gamewide: stays the same across all rooms.\n"
                    "Locked: same as gamewide, cannot be changed at runtime.\n"
                    "Background: replaced per room from the room's palette.");
        ImGui::EndTooltip();
    }

    ImGui::Separator();

    // Batch operations
    ImGui::Text("Batch:");
    if (ImGui::Button("Set Range Game-Wide"))
    {
        for (int i = 0; i < 64; i++)
            colors_[i].colour_type = 0;
        editor_.GetLogPanel().AddLog("[Info] Set colors 0-63 as game-wide.");
    }
    if (ImGui::Button("Set Range Background"))
    {
        for (int i = 64; i < 256; i++)
            colors_[i].colour_type = 2;
        editor_.GetLogPanel().AddLog("[Info] Set colors 64-255 as background.");
    }
}

void PaletteEditor::DrawPaletteTools()
{
    if (ImGui::Button("Import..."))
    {
        auto* proj = editor_.GetApp().GetProject();
        std::string default_dir = (proj && proj->IsLoaded()) ? proj->GetProjectDir() : ".";
        FileDialog::Open(FileDialogType::OpenFile, "Import Palette",
            ".pal,.bmp,.gpl,.txt{Palette Files}",
            default_dir,
            [this](const std::string& path) {
                // Detect format from extension
                std::string ext;
                auto dot = path.rfind('.');
                if (dot != std::string::npos)
                {
                    ext = path.substr(dot);
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                }

                if (ext == ".pal")
                {
                    // RIFF PAL or raw 768-byte palette
                    std::ifstream file(path, std::ios::binary);
                    if (!file.is_open())
                    {
                        editor_.GetLogPanel().AddLog("[Palette] Cannot open file: %s", path.c_str());
                        return;
                    }

                    // Check file size
                    file.seekg(0, std::ios::end);
                    auto size = file.tellg();
                    file.seekg(0, std::ios::beg);

                    if (size == 768)
                    {
                        // Raw RGB palette (256 * 3 bytes)
                        for (int i = 0; i < 256; i++)
                        {
                            uint8_t r, g, b;
                            file.read(reinterpret_cast<char*>(&r), 1);
                            file.read(reinterpret_cast<char*>(&g), 1);
                            file.read(reinterpret_cast<char*>(&b), 1);
                            // AGS palettes are often stored as 6-bit (0-63), scale to 8-bit
                            colors_[i].r = (r <= 63) ? r * 4 : r;
                            colors_[i].g = (g <= 63) ? g * 4 : g;
                            colors_[i].b = (b <= 63) ? b * 4 : b;
                        }
                    }
                    else
                    {
                        // Try RIFF PAL format
                        char riff_hdr[4];
                        file.read(riff_hdr, 4);
                        if (std::string(riff_hdr, 4) == "RIFF")
                        {
                            file.seekg(20); // Skip RIFF header to palette data
                            for (int i = 0; i < 256 && file.good(); i++)
                            {
                                uint8_t r, g, b, flags;
                                file.read(reinterpret_cast<char*>(&r), 1);
                                file.read(reinterpret_cast<char*>(&g), 1);
                                file.read(reinterpret_cast<char*>(&b), 1);
                                file.read(reinterpret_cast<char*>(&flags), 1);
                                colors_[i].r = r;
                                colors_[i].g = g;
                                colors_[i].b = b;
                            }
                        }
                        else
                        {
                            editor_.GetLogPanel().AddLog("[Palette] Unknown .pal format");
                            return;
                        }
                    }
                    editor_.GetLogPanel().AddLog("[Palette] Imported palette from: %s", path.c_str());
                }
                else if (ext == ".bmp")
                {
                    // Extract palette from 8-bit indexed BMP
                    SDL_Surface* surf = SDL_LoadBMP(path.c_str());
                    if (!surf)
                    {
                        editor_.GetLogPanel().AddLog("[Palette] Cannot load BMP: %s", SDL_GetError());
                        return;
                    }
                    if (!surf->format->palette || surf->format->palette->ncolors < 256)
                    {
                        editor_.GetLogPanel().AddLog(
                            "[Palette] BMP is not 256-color indexed. Only 8-bit BMPs have a palette.");
                        SDL_FreeSurface(surf);
                        return;
                    }
                    for (int i = 0; i < 256; i++)
                    {
                        colors_[i].r = surf->format->palette->colors[i].r;
                        colors_[i].g = surf->format->palette->colors[i].g;
                        colors_[i].b = surf->format->palette->colors[i].b;
                    }
                    SDL_FreeSurface(surf);
                    editor_.GetLogPanel().AddLog("[Palette] Imported palette from BMP: %s", path.c_str());
                }
                else if (ext == ".gpl" || ext == ".txt")
                {
                    // GIMP palette format (text-based)
                    std::ifstream file(path);
                    if (!file.is_open())
                    {
                        editor_.GetLogPanel().AddLog("[Palette] Cannot open file: %s", path.c_str());
                        return;
                    }

                    std::string line;
                    int idx = 0;
                    while (std::getline(file, line) && idx < 256)
                    {
                        // Skip comments and header lines
                        if (line.empty() || line[0] == '#' || line[0] == '\n')
                            continue;
                        if (line.find("GIMP Palette") != std::string::npos)
                            continue;
                        if (line.find("Name:") != std::string::npos)
                            continue;
                        if (line.find("Columns:") != std::string::npos)
                            continue;

                        // Parse "R G B [name]"
                        int r, g, b;
                        if (sscanf(line.c_str(), "%d %d %d", &r, &g, &b) == 3)
                        {
                            colors_[idx].r = std::clamp(r, 0, 255);
                            colors_[idx].g = std::clamp(g, 0, 255);
                            colors_[idx].b = std::clamp(b, 0, 255);
                            idx++;
                        }
                    }
                    editor_.GetLogPanel().AddLog("[Palette] Imported %d colors from: %s", idx, path.c_str());
                }
                else
                {
                    editor_.GetLogPanel().AddLog("[Palette] Unsupported palette format: %s", ext.c_str());
                }
            });
    }
    ImGui::SameLine();
    if (ImGui::Button("Export..."))
    {
        auto* proj = editor_.GetApp().GetProject();
        std::string default_dir = (proj && proj->IsLoaded()) ? proj->GetProjectDir() : ".";
        FileDialog::Open(FileDialogType::SaveFile, "Export Palette",
            ".pal,.bmp,.gpl{Palette Files}",
            default_dir,
            [this](const std::string& path) {
                std::string ext;
                auto dot = path.rfind('.');
                if (dot != std::string::npos)
                {
                    ext = path.substr(dot);
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                }

                if (ext == ".bmp")
                {
                    // Export as 8-bit indexed BMP (10x10)
                    SDL_Surface* surf = SDL_CreateRGBSurface(0, 10, 10, 8, 0, 0, 0, 0);
                    if (!surf)
                    {
                        editor_.GetLogPanel().AddLog("[Palette] Cannot create BMP surface: %s", SDL_GetError());
                        return;
                    }
                    SDL_Color sdl_pal[256];
                    for (int i = 0; i < 256; i++)
                    {
                        sdl_pal[i].r = (uint8_t)colors_[i].r;
                        sdl_pal[i].g = (uint8_t)colors_[i].g;
                        sdl_pal[i].b = (uint8_t)colors_[i].b;
                        sdl_pal[i].a = 255;
                    }
                    SDL_SetPaletteColors(surf->format->palette, sdl_pal, 0, 256);
                    if (SDL_SaveBMP(surf, path.c_str()) != 0)
                        editor_.GetLogPanel().AddLog("[Palette] Cannot save BMP: %s", SDL_GetError());
                    else
                        editor_.GetLogPanel().AddLog("[Palette] Exported palette BMP to: %s", path.c_str());
                    SDL_FreeSurface(surf);
                }
                else if (ext == ".gpl")
                {
                    // Export as GIMP palette
                    std::ofstream file(path);
                    if (!file.is_open())
                    {
                        editor_.GetLogPanel().AddLog("[Palette] Cannot create file: %s", path.c_str());
                        return;
                    }
                    file << "GIMP Palette\n";
                    file << "Name: AGS Game Palette\n";
                    file << "Columns: 16\n";
                    file << "#\n";
                    for (int i = 0; i < 256; i++)
                    {
                        char line[64];
                        snprintf(line, sizeof(line), "%3d %3d %3d\tColor %d",
                                 colors_[i].r, colors_[i].g, colors_[i].b, i);
                        file << line << "\n";
                    }
                    file.close();
                    editor_.GetLogPanel().AddLog("[Palette] Exported GIMP palette to: %s", path.c_str());
                }
                else
                {
                    // Export as raw .pal (768 bytes, 8-bit RGB)
                    std::string final_path = path;
                    if (ext.empty()) final_path += ".pal";

                    std::ofstream file(final_path, std::ios::binary);
                    if (!file.is_open())
                    {
                        editor_.GetLogPanel().AddLog("[Palette] Cannot create file: %s", final_path.c_str());
                        return;
                    }
                    for (int i = 0; i < 256; i++)
                    {
                        uint8_t r = (uint8_t)colors_[i].r;
                        uint8_t g = (uint8_t)colors_[i].g;
                        uint8_t b = (uint8_t)colors_[i].b;
                        file.write(reinterpret_cast<char*>(&r), 1);
                        file.write(reinterpret_cast<char*>(&g), 1);
                        file.write(reinterpret_cast<char*>(&b), 1);
                    }
                    file.close();
                    editor_.GetLogPanel().AddLog("[Palette] Exported raw palette to: %s", final_path.c_str());
                }
            });
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset to Default"))
    {
        // Re-init palette
        for (int i = 0; i < 256; i++)
        {
            if (i < 16)
            {
                static const int ega[16][3] = {
                    {0,0,0}, {0,0,170}, {0,170,0}, {0,170,170},
                    {170,0,0}, {170,0,170}, {170,85,0}, {170,170,170},
                    {85,85,85}, {85,85,255}, {85,255,85}, {85,255,255},
                    {255,85,85}, {255,85,255}, {255,255,85}, {255,255,255}
                };
                colors_[i].r = ega[i][0];
                colors_[i].g = ega[i][1];
                colors_[i].b = ega[i][2];
            }
            else
            {
                int v = i;
                colors_[i].r = v; colors_[i].g = v; colors_[i].b = v;
            }
            colors_[i].colour_type = (i < 64) ? 0 : 2;
        }
        editor_.GetLogPanel().AddLog("[Info] Palette reset to defaults.");
    }
}

bool PaletteEditor::LoadPaletteFromFile(const std::string& path, PaletteColor out[256])
{
    std::string ext;
    auto dot = path.rfind('.');
    if (dot != std::string::npos)
    {
        ext = path.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }

    if (ext == ".pal")
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return false;
        file.seekg(0, std::ios::end);
        auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size == 768)
        {
            for (int i = 0; i < 256; i++)
            {
                uint8_t r, g, b;
                file.read(reinterpret_cast<char*>(&r), 1);
                file.read(reinterpret_cast<char*>(&g), 1);
                file.read(reinterpret_cast<char*>(&b), 1);
                out[i].r = (r <= 63) ? r * 4 : r;
                out[i].g = (g <= 63) ? g * 4 : g;
                out[i].b = (b <= 63) ? b * 4 : b;
            }
        }
        else
        {
            char riff_hdr[4];
            file.read(riff_hdr, 4);
            if (std::string(riff_hdr, 4) != "RIFF") return false;
            file.seekg(20);
            for (int i = 0; i < 256 && file.good(); i++)
            {
                uint8_t r, g, b, flags;
                file.read(reinterpret_cast<char*>(&r), 1);
                file.read(reinterpret_cast<char*>(&g), 1);
                file.read(reinterpret_cast<char*>(&b), 1);
                file.read(reinterpret_cast<char*>(&flags), 1);
                out[i].r = r; out[i].g = g; out[i].b = b;
            }
        }
        return true;
    }
    else if (ext == ".bmp")
    {
        SDL_Surface* surf = SDL_LoadBMP(path.c_str());
        if (!surf) return false;
        if (!surf->format->palette || surf->format->palette->ncolors < 256)
        {
            SDL_FreeSurface(surf);
            return false;
        }
        for (int i = 0; i < 256; i++)
        {
            out[i].r = surf->format->palette->colors[i].r;
            out[i].g = surf->format->palette->colors[i].g;
            out[i].b = surf->format->palette->colors[i].b;
        }
        SDL_FreeSurface(surf);
        return true;
    }
    else if (ext == ".gpl" || ext == ".txt")
    {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        std::string line;
        int idx = 0;
        while (std::getline(file, line) && idx < 256)
        {
            if (line.empty() || line[0] == '#') continue;
            if (line.find("GIMP Palette") != std::string::npos) continue;
            if (line.find("Name:") != std::string::npos) continue;
            if (line.find("Columns:") != std::string::npos) continue;
            int r, g, b;
            if (sscanf(line.c_str(), "%d %d %d", &r, &g, &b) == 3)
            {
                out[idx].r = std::clamp(r, 0, 255);
                out[idx].g = std::clamp(g, 0, 255);
                out[idx].b = std::clamp(b, 0, 255);
                idx++;
            }
        }
        return idx > 0;
    }
    return false;
}

void PaletteEditor::ReplaceSelectedFromFile(const std::string& path)
{
    PaletteColor temp[256] = {};
    if (!LoadPaletteFromFile(path, temp))
    {
        editor_.GetLogPanel().AddLog("[Palette] Failed to load palette from: %s", path.c_str());
        return;
    }

    int count = 0;
    for (int idx : selected_colors_)
    {
        if (idx < 0 || idx >= 256) continue;
        // Replace color but preserve colour_type
        colors_[idx].r = temp[idx].r;
        colors_[idx].g = temp[idx].g;
        colors_[idx].b = temp[idx].b;
        count++;
    }

    editor_.GetLogPanel().AddLog("[Palette] Replaced %d selected slot(s) from: %s",
        count, path.c_str());
}

} // namespace AGSEditor
