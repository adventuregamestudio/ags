// AGS Editor ImGui - Sprite Manager
// Uses real sprite data from SpriteLoader + TextureCache for rendering.
#include "sprite_manager.h"
#include "ui/log_panel.h"
#include "ui/file_dialog.h"
#include "ui/folder_tree_widget.h"
#include "core/dpi_helper.h"
#include "core/preferences.h"
#include "project/project.h"
#include "project/sprite_loader.h"
#include "project/texture_cache.h"
#include "project/game_data.h"
#include "app.h"
#include "imgui.h"
#include <SDL.h>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <functional>
#include <set>

#include "ac/spritefile.h"
#include "gfx/bitmap.h"
#include "gfx/bitmapdata.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace AGSEditor
{

SpriteManager::SpriteManager(EditorUI& editor)
    : editor_(editor)
{
    // Load persistent settings from preferences
    auto& prefs = editor_.GetApp().GetPreferences();
    icon_size_ = prefs.sprite_icon_size;
    show_properties_ = prefs.sprite_show_properties;
}

void SpriteManager::RefreshFromProject()
{
    needs_refresh_ = true;
}

// ============================================================================
// Main Draw
// ============================================================================

void SpriteManager::Draw()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
    {
        ImGui::TextDisabled("No project loaded. Open a game to view sprites.");
        return;
    }

    DrawToolbar();
    ImGui::Separator();

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float props_width = show_properties_ ? Dpi(220) : 0.0f;
    float tree_width = show_folder_tree_ ? Dpi(180) : 0.0f;

    // Folder tree panel
    if (show_folder_tree_)
    {
        ImGui::BeginChild("##SpriteFolders", ImVec2(tree_width, avail.y), ImGuiChildFlags_Borders);
        DrawFolderTree();
        ImGui::EndChild();
        ImGui::SameLine();
    }

    // Sprite grid
    float grid_w = avail.x - props_width - tree_width
                 - (show_properties_ ? Dpi(4) : 0.0f)
                 - (show_folder_tree_ ? Dpi(4) : 0.0f);
    ImGui::BeginChild("##SpriteGrid", ImVec2(grid_w, avail.y), ImGuiChildFlags_Borders);
    DrawSpriteGrid();
    ImGui::EndChild();

    // Properties panel
    if (show_properties_)
    {
        ImGui::SameLine();
        ImGui::BeginChild("##SpriteProps", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
        DrawSpriteProperties();
        ImGui::EndChild();
    }

    DrawContextMenu();

    // Undo/Redo keyboard shortcuts
    auto& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift)
        Undo();
    if (io.KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))))
        Redo();

    // Assign-to-view popup (must be rendered every frame when open)
    if (ImGui::BeginPopupModal("AssignToViewPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Assign %d sprite(s) as sequential frames:", (int)selected_sprites_.size());
        ImGui::Separator();
        ImGui::InputInt("View ID", &assign_view_id_);
        ImGui::InputInt("Loop", &assign_loop_id_);
        ImGui::Separator();

        if (ImGui::Button("Assign", ImVec2(120, 0)))
        {
            auto* assign_project = editor_.GetApp().GetProject();
            auto* gd = assign_project ? assign_project->GetGameData() : nullptr;
            if (gd)
            {
                // Find or validate the target view
                GameData::ViewInfo* target_view = nullptr;
                for (auto& v : gd->views)
                {
                    if (v.id == assign_view_id_)
                    {
                        target_view = &v;
                        break;
                    }
                }

                if (!target_view)
                {
                    editor_.GetLogPanel().AddLog("[Sprite] View %d not found.", assign_view_id_);
                }
                else if (assign_loop_id_ < 0 || assign_loop_id_ >= (int)target_view->loops.size())
                {
                    editor_.GetLogPanel().AddLog("[Sprite] Loop %d out of range for view %d (has %d loops).",
                        assign_loop_id_, assign_view_id_, (int)target_view->loops.size());
                }
                else
                {
                    auto& loop = target_view->loops[assign_loop_id_];
                    loop.frames.clear();
                    for (int spr_id : selected_sprites_)
                    {
                        GameData::FrameData frame;
                        frame.sprite_id = spr_id;
                        frame.delay = 0;
                        frame.flipped = false;
                        frame.sound = -1;
                        loop.frames.push_back(frame);
                    }
                    editor_.GetLogPanel().AddLog("[Sprite] Assigned %d sprite(s) to View %d, Loop %d.",
                        (int)selected_sprites_.size(), assign_view_id_, assign_loop_id_);
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

// ============================================================================
// Toolbar
// ============================================================================

void SpriteManager::DrawToolbar()
{
    if (ImGui::Button("Import..."))
        ImportSprite();
    ImGui::SameLine();
    if (ImGui::Button("Batch Import..."))
        BatchImportSprites();
    ImGui::SameLine();
    if (ImGui::Button("Export..."))
        ExportSprite();
    ImGui::SameLine();
    if (ImGui::Button("Replace..."))
        ReplaceSprite();
    ImGui::SameLine();
    if (ImGui::Button("Delete"))
        DeleteSprite();

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(Dpi(80));
    if (ImGui::SliderFloat("Size", &icon_size_, 32.0f, 256.0f, "%.0f"))
    {
        editor_.GetApp().GetPreferences().sprite_icon_size = icon_size_;
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Props", &show_properties_))
    {
        editor_.GetApp().GetPreferences().sprite_show_properties = show_properties_;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Folders", &show_folder_tree_);

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(Dpi(120));
    ImGui::InputTextWithHint("##filter", "Filter...", filter_text_, sizeof(filter_text_));

    auto* loader = editor_.GetApp().GetProject()->GetSpriteLoader();
    if (loader && loader->IsOpen())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("| %d sprites", loader->GetSpriteCount());
    }

    if (!selected_sprites_.empty())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("| %d selected", (int)selected_sprites_.size());
    }
}

// ============================================================================
// Sprite Grid
// ============================================================================

void SpriteManager::DrawSpriteGrid()
{
    auto* project = editor_.GetApp().GetProject();
    auto* loader = project ? project->GetSpriteLoader() : nullptr;
    auto& tex_cache = editor_.GetApp().GetTextureCache();

    if (!loader || !loader->IsOpen())
    {
        ImGui::TextDisabled("No sprite file loaded.");
        return;
    }

    const auto& all_metrics = loader->GetAllMetrics();
    int total = (int)all_metrics.size();

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Sprites (%d slots)", total);
    ImGui::Separator();

    float avail_width = ImGui::GetContentRegionAvail().x;
    float cell_size = icon_size_ + Dpi(4);
    int cols = std::max(1, (int)(avail_width / cell_size));

    // Collect visible sprite indices
    bool has_filter = (filter_text_[0] != '\0');
    std::vector<int> visible_ids;
    visible_ids.reserve(total);

    // Build folder filter set if a folder is selected
    std::set<int> folder_sprite_ids;
    bool has_folder_filter = (selected_folder_ != nullptr);
    if (has_folder_filter)
    {
        const auto* folder = static_cast<const FolderInfo*>(selected_folder_);
        CollectFolderItemIds(*folder, folder_sprite_ids);
    }

    for (int i = 0; i < total; i++)
    {
        if (!all_metrics[i].exists)
            continue;
        if (has_folder_filter && folder_sprite_ids.find(i) == folder_sprite_ids.end())
            continue;
        if (has_filter)
        {
            char id_str[16];
            snprintf(id_str, sizeof(id_str), "%d", i);
            if (!strstr(id_str, filter_text_))
                continue;
        }
        visible_ids.push_back(i);
    }

    int rows = ((int)visible_ids.size() + cols - 1) / cols;

    ImGuiListClipper clipper;
    clipper.Begin(rows, cell_size);
    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
        {
            for (int c = 0; c < cols; c++)
            {
                int idx = row * cols + c;
                if (idx >= (int)visible_ids.size())
                    break;

                int sprite_id = visible_ids[idx];
                const auto& met = all_metrics[sprite_id];

                if (c > 0)
                    ImGui::SameLine();

                ImGui::PushID(sprite_id);

                bool selected = IsSpriteSelected(sprite_id);

                // Try to get the sprite texture
                SDL_Texture* tex = tex_cache.GetSpriteTexture(sprite_id, loader);

                ImVec2 btn_size(icon_size_, icon_size_);
                bool clicked = false;

                ImVec2 cursor = ImGui::GetCursorScreenPos();
                clicked = ImGui::InvisibleButton("##spr", btn_size);
                ImDrawList* dl = ImGui::GetWindowDrawList();

                // Background
                ImU32 bg_col32 = selected ? IM_COL32(77, 102, 153, 255) : IM_COL32(38, 38, 51, 255);
                if (ImGui::IsItemHovered())
                    bg_col32 = IM_COL32(64, 89, 128, 255);
                dl->AddRectFilled(cursor, ImVec2(cursor.x + icon_size_, cursor.y + icon_size_), bg_col32);

                if (tex)
                {
                    // Calculate aspect-correct size within the cell
                    float scale = std::min(icon_size_ / (float)met.width,
                                           icon_size_ / (float)met.height);
                    float dw = met.width * scale;
                    float dh = met.height * scale;

                    // Centered sprite image
                    float ox = (icon_size_ - dw) * 0.5f;
                    float oy = (icon_size_ - dh) * 0.5f;
                    dl->AddImage((ImTextureID)(intptr_t)tex,
                        ImVec2(cursor.x + ox, cursor.y + oy),
                        ImVec2(cursor.x + ox + dw, cursor.y + oy + dh));
                }
                else
                {
                    // Fallback: show dimensions text
                    char label[48];
                    snprintf(label, sizeof(label), "%d\n%dx%d", sprite_id, met.width, met.height);
                    ImVec2 ts = ImGui::CalcTextSize(label);
                    dl->AddText(ImVec2(cursor.x + (icon_size_ - ts.x) * 0.5f,
                                       cursor.y + (icon_size_ - ts.y) * 0.5f),
                        IM_COL32(180, 180, 180, 255), label);
                }

                // Sprite ID label in corner
                char id_label[16];
                snprintf(id_label, sizeof(id_label), "%d", sprite_id);
                dl->AddText(ImVec2(cursor.x + 2, cursor.y + 1),
                    IM_COL32(200, 200, 200, 180), id_label);

                // Selection border
                if (selected)
                    dl->AddRect(cursor, ImVec2(cursor.x + icon_size_, cursor.y + icon_size_),
                        IM_COL32(100, 180, 255, 255), 0.0f, 0, 2.0f);

                if (clicked)
                {
                    bool ctrl = ImGui::GetIO().KeyCtrl;
                    bool shift = ImGui::GetIO().KeyShift;
                    if (ctrl)
                    {
                        ToggleSpriteSelection(sprite_id);
                    }
                    else if (shift && primary_sprite_ >= 0)
                    {
                        selected_sprites_.clear();
                        int lo = std::min(primary_sprite_, sprite_id);
                        int hi = std::max(primary_sprite_, sprite_id);
                        for (int s = lo; s <= hi; s++)
                            if (s < total && all_metrics[s].exists)
                                selected_sprites_.push_back(s);
                    }
                    else
                    {
                        selected_sprites_.clear();
                        selected_sprites_.push_back(sprite_id);
                        primary_sprite_ = sprite_id;
                    }
                }

                // Right-click context menu
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                {
                    if (!IsSpriteSelected(sprite_id))
                    {
                        selected_sprites_.clear();
                        selected_sprites_.push_back(sprite_id);
                        primary_sprite_ = sprite_id;
                    }
                    show_context_menu_ = true;
                }

                // Tooltip
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Sprite %d\nSize: %dx%d\nDepth: %d bpp",
                        sprite_id, met.width, met.height, met.color_depth);
                }

                // Drag source for folder DnD
                {
                    char dnd_label[32];
                    snprintf(dnd_label, sizeof(dnd_label), "Sprite %d", sprite_id);
                    BeginItemDragSource(sprite_id, dnd_label);
                }

                ImGui::PopID();
            }
        }
    }
}

// ============================================================================
// Sprite Folder Tree
// ============================================================================

void SpriteManager::DrawFolderTree()
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = project ? project->GetGameData() : nullptr;
    if (!gd)
    {
        ImGui::TextDisabled("No project data.");
        return;
    }

    const auto* sel = static_cast<const FolderInfo*>(selected_folder_);
    const auto* new_sel = DrawFolderTreeWidget(sel, gd->root_sprite_folder, "All Sprites",
                                                &gd->root_sprite_folder);
    if (new_sel != sel)
        selected_folder_ = new_sel;
}

// ============================================================================
// Sprite Properties
// ============================================================================

void SpriteManager::DrawSpriteProperties()
{
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Properties");
    ImGui::Separator();

    if (selected_sprites_.empty() || primary_sprite_ < 0)
    {
        ImGui::TextDisabled("No sprite selected.");
        return;
    }

    auto* loader = editor_.GetApp().GetProject()->GetSpriteLoader();
    if (!loader || !loader->IsOpen())
        return;

    const auto* met = loader->GetMetrics(primary_sprite_);
    if (!met || !met->exists)
    {
        ImGui::TextDisabled("Sprite slot %d is empty.", primary_sprite_);
        return;
    }

    bool multi = (selected_sprites_.size() > 1);
    if (multi)
        ImGui::Text("%d sprites selected", (int)selected_sprites_.size());

    ImGui::Text("ID: %d", primary_sprite_);
    ImGui::Text("Size: %d x %d", met->width, met->height);
    ImGui::Text("Color depth: %d bpp", met->color_depth);

    // Source info from AGF metadata
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (gd)
    {
        for (const auto& spr : gd->sprites)
        {
            if (spr.id == primary_sprite_)
            {
                if (spr.source.HasSource())
                {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1.0f), "Source");
                    ImGui::TextWrapped("File: %s", spr.source.source_file.c_str());
                    if (spr.source.frame > 0)
                        ImGui::Text("Frame: %d", spr.source.frame);
                    if (spr.source.offset_x > 0 || spr.source.offset_y > 0)
                        ImGui::Text("Offset: %d, %d", spr.source.offset_x, spr.source.offset_y);
                    if (spr.source.import_width > 0 || spr.source.import_height > 0)
                        ImGui::Text("Import size: %dx%d", spr.source.import_width, spr.source.import_height);
                }
                if (!spr.resolution.empty() && spr.resolution != "Real")
                    ImGui::Text("Resolution: %s", spr.resolution.c_str());
                if (spr.alpha_channel)
                    ImGui::Text("Alpha channel: Yes");
                break;
            }
        }
    }

    ImGui::Separator();

    // Real sprite preview
    auto& tex_cache = editor_.GetApp().GetTextureCache();
    SDL_Texture* tex = tex_cache.GetSpriteTexture(primary_sprite_, loader);
    if (tex)
    {
        float max_w = ImGui::GetContentRegionAvail().x;
        float max_h = Dpi(200);
        float scale = std::min(max_w / (float)met->width, max_h / (float)met->height);
        scale = std::min(scale, 4.0f); // Don't upscale too much
        float dw = met->width * scale;
        float dh = met->height * scale;

        // Checkerboard background for transparency
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float check = 8.0f;
        for (float cy = 0; cy < dh; cy += check)
        {
            for (float cx = 0; cx < dw; cx += check)
            {
                int ix = (int)(cx / check);
                int iy = (int)(cy / check);
                ImU32 col = ((ix + iy) & 1) ? IM_COL32(60, 60, 60, 255) : IM_COL32(40, 40, 40, 255);
                float x0 = pos.x + cx, y0 = pos.y + cy;
                float x1 = std::min(x0 + check, pos.x + dw);
                float y1 = std::min(y0 + check, pos.y + dh);
                dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), col);
            }
        }

        ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(dw, dh));
    }
    else
    {
        // Fallback colored rectangle
        ImVec2 preview_pos = ImGui::GetCursorScreenPos();
        float pw = std::min(Dpi(180), (float)met->width * 2.0f);
        float ph = pw * met->height / std::max(1, met->width);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImU32 spr_col = IM_COL32(100 + (primary_sprite_ * 17) % 155,
                                  80 + (primary_sprite_ * 31) % 175,
                                  120 + (primary_sprite_ * 7) % 120, 255);
        dl->AddRectFilled(preview_pos, ImVec2(preview_pos.x + pw, preview_pos.y + ph), spr_col);
        dl->AddRect(preview_pos, ImVec2(preview_pos.x + pw, preview_pos.y + ph),
                    IM_COL32(100, 100, 100, 255));
        ImGui::Dummy(ImVec2(pw, ph));
    }

    ImGui::Separator();

    // Actions
    if (ImGui::Button("Replace...", ImVec2(-1, 0))) ReplaceSprite();
    if (ImGui::Button("Export...", ImVec2(-1, 0))) ExportSprite();
    if (ImGui::Button("Assign to View", ImVec2(-1, 0))) AssignToView();
    if (ImGui::Button("Show Usage", ImVec2(-1, 0))) ShowUsage();
    if (ImGui::Button("Delete", ImVec2(-1, 0))) DeleteSprite();
}

// ============================================================================
// Context Menu
// ============================================================================

void SpriteManager::DrawContextMenu()
{
    if (show_context_menu_)
    {
        ImGui::OpenPopup("##SpriteCtxMenu");
        show_context_menu_ = false;
    }

    if (ImGui::BeginPopup("##SpriteCtxMenu"))
    {
        bool has_sel = !selected_sprites_.empty();
        if (ImGui::MenuItem("Import...", nullptr, false, true)) ImportSprite();
        if (ImGui::MenuItem("Export...", nullptr, false, has_sel)) ExportSprite();
        if (ImGui::MenuItem("Replace...", nullptr, false, has_sel)) ReplaceSprite();
        if (ImGui::MenuItem("Replace from Source", nullptr, false, has_sel)) ReplaceFromSource();
        ImGui::Separator();
        if (ImGui::MenuItem("Duplicate", nullptr, false, has_sel)) DuplicateSprite();
        if (ImGui::MenuItem("Delete", "Del", false, has_sel)) DeleteSprite();
        ImGui::Separator();
        if (ImGui::MenuItem("Assign to View", nullptr, false, has_sel)) AssignToView();
        if (ImGui::MenuItem("Show Usage", nullptr, false, has_sel)) ShowUsage();
        ImGui::Separator();
        if (ImGui::MenuItem("Open Project Folder"))
        {
            auto* project = editor_.GetApp().GetProject();
            if (project && project->IsLoaded())
            {
                std::string dir = project->GetProjectDir();
#if defined(__linux__)
                std::string cmd = "xdg-open \"" + dir + "\" &";
                system(cmd.c_str());
#elif defined(__APPLE__)
                std::string cmd = "open \"" + dir + "\"";
                system(cmd.c_str());
#elif defined(_WIN32)
                std::string cmd = "explorer \"" + dir + "\"";
                system(cmd.c_str());
#endif
            }
        }
        ImGui::EndPopup();
    }
}

// ============================================================================
// Operations
// ============================================================================

void SpriteManager::ImportSprite()
{
    auto* project = editor_.GetApp().GetProject();
    std::string default_dir = (project && project->IsLoaded()) ? project->GetProjectDir() : ".";

    // Use OpenFile for single import
    FileDialog::Open(FileDialogType::OpenFile, "Import Sprite",
        ".png,.bmp,.tga,.jpg,.jpeg{Image Files}",
        default_dir,
        [this](const std::string& path) {
            auto* project2 = editor_.GetApp().GetProject();
            auto* loader = project2 ? project2->GetSpriteLoader() : nullptr;
            if (!loader || !loader->IsOpen() || !loader->GetSpriteFile())
            {
                editor_.GetLogPanel().AddLog("[Sprite] No sprite file open.");
                return;
            }

            std::vector<std::string> paths = { path };

            auto* sprite_file = loader->GetSpriteFile();
            int old_max = loader->GetTopmostSprite();

            // Load all images
            struct ImportEntry {
                AGS::Common::Bitmap* bmp = nullptr;
                std::string source;
            };
            std::vector<ImportEntry> to_import;

            for (const auto& img_path : paths)
            {
                auto* bmp = AGS::Common::BitmapHelper::LoadFromFile(img_path.c_str());
                if (bmp)
                    to_import.push_back({bmp, img_path});
                else
                    editor_.GetLogPanel().AddLog("[Sprite] Failed to load: %s", img_path.c_str());
            }

            if (to_import.empty()) return;

            // Build sprites vector: existing slots empty, new slots at end
            int new_max = old_max + (int)to_import.size();
            std::vector<std::pair<bool, AGS::Common::BitmapData>> sprites(new_max + 1);

            std::vector<int> new_ids;
            for (int i = 0; i < (int)to_import.size(); i++)
            {
                int new_slot = old_max + 1 + i;
                sprites[new_slot] = { true, to_import[i].bmp->GetBitmapData() };
                new_ids.push_back(new_slot);
            }

            // Rewrite sprite file
            std::string spr_path = project2->GetProjectDir() + "/acsprset.spr";
            AGS::Common::SpriteFileIndex index;
            auto save_err = AGS::Common::SaveSpriteFile(
                AGS::Common::String(spr_path.c_str()),
                sprites, sprite_file,
                sprite_file->GetStoreFlags(),
                sprite_file->GetSpriteCompression(),
                index);

            if (save_err)
            {
                std::string idx_path = project2->GetProjectDir() + "/sprindex.dat";
                AGS::Common::SaveSpriteIndex(
                    AGS::Common::String(idx_path.c_str()), index);

                // Reload sprite loader
                loader->Close();
                loader->Open(project2->GetProjectDir());

                // Update game data
                auto* gd = project2->GetGameData();
                if (gd)
                {
                    for (int i = 0; i < (int)to_import.size(); i++)
                    {
                        GameData::SpriteInfo si;
                        si.id = new_ids[i];
                        si.width = to_import[i].bmp->GetWidth();
                        si.height = to_import[i].bmp->GetHeight();
                        si.color_depth = to_import[i].bmp->GetColorDepth();
                        gd->sprites.push_back(si);
                    }
                }

                editor_.GetLogPanel().AddLog("[Sprite] Imported %d sprite(s) (slots %d-%d)",
                    (int)to_import.size(), new_ids.front(), new_ids.back());

                // Record undo action
                SpriteUndoAction undo_action;
                undo_action.type = SpriteUndoAction::Import;
                undo_action.sprite_ids = new_ids;
                PushUndo(std::move(undo_action));

                needs_refresh_ = true;
            }
            else
            {
                editor_.GetLogPanel().AddLog("[Sprite] Failed to save sprite file after import.");
            }

            // Free loaded bitmaps
            for (auto& ie : to_import)
                delete ie.bmp;
        });
}

void SpriteManager::BatchImportSprites()
{
    auto* project = editor_.GetApp().GetProject();
    std::string default_dir = (project && project->IsLoaded()) ? project->GetProjectDir() : ".";

    FileDialog::Open(FileDialogType::SelectFolder, "Select Folder for Batch Import",
        "",
        default_dir,
        [this](const std::string& folder_path) {
            auto* project2 = editor_.GetApp().GetProject();
            auto* loader = project2 ? project2->GetSpriteLoader() : nullptr;
            if (!loader || !loader->IsOpen() || !loader->GetSpriteFile())
            {
                editor_.GetLogPanel().AddLog("[Sprite] No sprite file open.");
                return;
            }

            // Collect all image files from the folder, sorted alphabetically
            std::vector<std::string> image_paths;
            try {
                for (const auto& entry : fs::directory_iterator(folder_path))
                {
                    if (!entry.is_regular_file()) continue;
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".png" || ext == ".bmp" || ext == ".tga" ||
                        ext == ".jpg" || ext == ".jpeg" || ext == ".pcx")
                    {
                        image_paths.push_back(entry.path().string());
                    }
                }
            } catch (...) {
                editor_.GetLogPanel().AddLog("[Sprite] Failed to read folder: %s", folder_path.c_str());
                return;
            }

            std::sort(image_paths.begin(), image_paths.end());

            if (image_paths.empty())
            {
                editor_.GetLogPanel().AddLog("[Sprite] No image files found in: %s", folder_path.c_str());
                return;
            }

            auto* sprite_file = loader->GetSpriteFile();
            int old_max = loader->GetTopmostSprite();

            // Load all images
            struct ImportEntry {
                AGS::Common::Bitmap* bmp = nullptr;
                std::string source;
            };
            std::vector<ImportEntry> to_import;

            for (const auto& path : image_paths)
            {
                auto* bmp = AGS::Common::BitmapHelper::LoadFromFile(path.c_str());
                if (bmp)
                    to_import.push_back({bmp, path});
                else
                    editor_.GetLogPanel().AddLog("[Sprite] Failed to load: %s", path.c_str());
            }

            if (to_import.empty()) return;

            // Build sprites vector
            int new_max = old_max + (int)to_import.size();
            std::vector<std::pair<bool, AGS::Common::BitmapData>> sprites(new_max + 1);

            std::vector<int> new_ids;
            for (int i = 0; i < (int)to_import.size(); i++)
            {
                int new_slot = old_max + 1 + i;
                sprites[new_slot] = { true, to_import[i].bmp->GetBitmapData() };
                new_ids.push_back(new_slot);
            }

            // Rewrite sprite file
            std::string spr_path = project2->GetProjectDir() + "/acsprset.spr";
            AGS::Common::SpriteFileIndex index;
            auto save_err = AGS::Common::SaveSpriteFile(
                AGS::Common::String(spr_path.c_str()),
                sprites, sprite_file,
                sprite_file->GetStoreFlags(),
                sprite_file->GetSpriteCompression(),
                index);

            if (save_err)
            {
                std::string idx_path = project2->GetProjectDir() + "/sprindex.dat";
                AGS::Common::SaveSpriteIndex(
                    AGS::Common::String(idx_path.c_str()), index);

                loader->Close();
                loader->Open(project2->GetProjectDir());

                auto* gd = project2->GetGameData();
                if (gd)
                {
                    for (int i = 0; i < (int)to_import.size(); i++)
                    {
                        GameData::SpriteInfo si;
                        si.id = new_ids[i];
                        si.width = to_import[i].bmp->GetWidth();
                        si.height = to_import[i].bmp->GetHeight();
                        si.color_depth = to_import[i].bmp->GetColorDepth();
                        gd->sprites.push_back(si);
                    }
                }

                editor_.GetLogPanel().AddLog("[Sprite] Batch imported %d sprite(s) from %s (slots %d-%d)",
                    (int)to_import.size(), folder_path.c_str(), new_ids.front(), new_ids.back());

                // Record undo action
                SpriteUndoAction undo_action;
                undo_action.type = SpriteUndoAction::Import;
                undo_action.sprite_ids = new_ids;
                PushUndo(std::move(undo_action));

                needs_refresh_ = true;
            }
            else
            {
                editor_.GetLogPanel().AddLog("[Sprite] Failed to save sprite file after batch import.");
            }

            for (auto& ie : to_import)
                delete ie.bmp;
        });
}

void SpriteManager::ExportSprite()
{
    if (selected_sprites_.empty()) return;

    auto* project = editor_.GetApp().GetProject();
    auto* loader = project ? project->GetSpriteLoader() : nullptr;
    if (!loader || !loader->IsOpen() || !loader->GetSpriteFile()) return;

    std::string default_dir = project->GetProjectDir();

    if (selected_sprites_.size() == 1)
    {
        // Single sprite: let user pick save path
        int spr_id = selected_sprites_[0];
        FileDialog::Open(FileDialogType::SaveFile, "Export Sprite",
            ".bmp,.pcx{Image Files}",
            default_dir,
            [this, spr_id](const std::string& path) {
                auto* project2 = editor_.GetApp().GetProject();
                auto* loader2 = project2 ? project2->GetSpriteLoader() : nullptr;
                if (!loader2 || !loader2->GetSpriteFile()) return;

                AGS::Common::PixelBuffer pixbuf;
                auto err = loader2->GetSpriteFile()->LoadSprite(spr_id, pixbuf);
                if (!err)
                {
                    editor_.GetLogPanel().AddLog("[Sprite] Failed to load sprite %d: %s",
                        spr_id, err->FullMessage().GetCStr());
                    return;
                }

                // Convert PixelBuffer to Bitmap
                auto* bmp = AGS::Common::BitmapHelper::CreateBitmap(
                    pixbuf.GetWidth(), pixbuf.GetHeight(), pixbuf.GetColorDepth());
                if (bmp)
                {
                    // Copy pixel data
                    const uint8_t* src_data = pixbuf.GetData();
                    if (src_data)
                    {
                        for (int y = 0; y < pixbuf.GetHeight(); y++)
                        {
                            memcpy(bmp->GetScanLineForWriting(y),
                                   src_data + y * pixbuf.GetStride(),
                                   std::min((size_t)bmp->GetLineLength(),
                                            (size_t)pixbuf.GetStride()));
                        }
                    }

                    if (AGS::Common::BitmapHelper::SaveToFile(bmp, path.c_str()))
                        editor_.GetLogPanel().AddLog("[Sprite] Exported sprite %d to: %s", spr_id, path.c_str());
                    else
                        editor_.GetLogPanel().AddLog("[Sprite] Failed to save sprite %d to: %s", spr_id, path.c_str());
                    delete bmp;
                }
            });
    }
    else
    {
        // Multiple sprites: export to a directory using sprite IDs as filenames
        FileDialog::Open(FileDialogType::SelectFolder, "Export Sprites To Folder",
            "", default_dir,
            [this](const std::string& dir) {
                auto* project2 = editor_.GetApp().GetProject();
                auto* loader2 = project2 ? project2->GetSpriteLoader() : nullptr;
                if (!loader2 || !loader2->GetSpriteFile()) return;

                int exported = 0;
                for (int spr_id : selected_sprites_)
                {
                    AGS::Common::PixelBuffer pixbuf;
                    auto err = loader2->GetSpriteFile()->LoadSprite(spr_id, pixbuf);
                    if (!err) continue;

                    auto* bmp = AGS::Common::BitmapHelper::CreateBitmap(
                        pixbuf.GetWidth(), pixbuf.GetHeight(), pixbuf.GetColorDepth());
                    if (!bmp) continue;

                    const uint8_t* src_data = pixbuf.GetData();
                    if (src_data)
                    {
                        for (int y = 0; y < pixbuf.GetHeight(); y++)
                        {
                            memcpy(bmp->GetScanLineForWriting(y),
                                   src_data + y * pixbuf.GetStride(),
                                   std::min((size_t)bmp->GetLineLength(),
                                            (size_t)pixbuf.GetStride()));
                        }
                    }

                    char fname[64];
                    snprintf(fname, sizeof(fname), "%s/sprite_%04d.bmp", dir.c_str(), spr_id);
                    if (AGS::Common::BitmapHelper::SaveToFile(bmp, fname))
                        exported++;
                    delete bmp;
                }
                editor_.GetLogPanel().AddLog("[Sprite] Exported %d sprite(s) to: %s",
                    exported, dir.c_str());
            });
    }
}

void SpriteManager::DeleteSprite()
{
    if (selected_sprites_.empty()) return;

    auto* project = editor_.GetApp().GetProject();
    auto* gd = project ? project->GetGameData() : nullptr;

    // Check usage first — warn if any sprite is in use
    if (gd)
    {
        for (int spr_id : selected_sprites_)
        {
            bool in_use = false;
            // Check views
            for (const auto& view : gd->views)
            {
                for (const auto& loop : view.loops)
                    for (const auto& frame : loop.frames)
                        if (frame.sprite_id == spr_id) { in_use = true; break; }
                if (in_use) break;
            }
            // Check inventory items
            if (!in_use)
            {
                for (const auto& inv : gd->inventory_items)
                    if (inv.image == spr_id || inv.cursor_image == spr_id) { in_use = true; break; }
            }
            // Check cursors
            if (!in_use)
            {
                for (const auto& cur : gd->cursors)
                    if (cur.image == spr_id) { in_use = true; break; }
            }

            if (in_use)
            {
                editor_.GetLogPanel().AddLog("[Sprite] WARNING: Sprite %d is still in use. "
                    "References will become invalid.", spr_id);
            }
        }
    }

    // Mark sprites as deleted in game data
    if (gd)
    {
        SpriteUndoAction undo_action;
        undo_action.type = SpriteUndoAction::Delete;
        undo_action.sprite_ids = selected_sprites_;

        for (int spr_id : selected_sprites_)
        {
            // Remove from sprite info list (mark as non-existent)
            for (auto& s : gd->sprites)
            {
                if (s.id == spr_id)
                {
                    undo_action.saved_info.push_back(s); // save for undo
                    s.width = 0;
                    s.height = 0;
                    s.color_depth = 0;
                    break;
                }
            }
            editor_.GetLogPanel().AddLog("[Sprite] Deleted sprite %d.", spr_id);
        }

        PushUndo(std::move(undo_action));
    }

    // Clear texture cache for deleted sprites
    auto& tex_cache = editor_.GetApp().GetTextureCache();
    for (int spr_id : selected_sprites_)
        tex_cache.EvictSprite(spr_id);

    int count = (int)selected_sprites_.size();
    selected_sprites_.clear();
    primary_sprite_ = -1;
    needs_refresh_ = true;
    editor_.GetLogPanel().AddLog("[Sprite] Deleted %d sprite(s).", count);
}

void SpriteManager::ReplaceSprite()
{
    if (primary_sprite_ < 0) return;

    auto* project = editor_.GetApp().GetProject();
    std::string default_dir = (project && project->IsLoaded()) ? project->GetProjectDir() : ".";
    int spr_id = primary_sprite_;

    FileDialog::Open(FileDialogType::OpenFile, "Replace Sprite",
        ".bmp,.pcx,.tga{Image Files}",
        default_dir,
        [this, spr_id](const std::string& path) {
            auto* project2 = editor_.GetApp().GetProject();
            auto* loader = project2 ? project2->GetSpriteLoader() : nullptr;
            if (!loader || !loader->IsOpen() || !loader->GetSpriteFile()) return;

            // Load the replacement image
            auto* bmp = AGS::Common::BitmapHelper::LoadFromFile(path.c_str());
            if (!bmp)
            {
                editor_.GetLogPanel().AddLog("[Sprite] Failed to load image: %s", path.c_str());
                return;
            }

            // Build a sparse sprites vector — only the replaced slot has data
            int max_slot = loader->GetTopmostSprite();
            std::vector<std::pair<bool, AGS::Common::BitmapData>> sprites(max_slot + 1);
            sprites[spr_id] = { true, bmp->GetBitmapData() };

            // Rewrite the entire sprite file with the replacement
            std::string spr_path = project2->GetProjectDir() + "/acsprset.spr";
            AGS::Common::SpriteFileIndex index;
            auto err = AGS::Common::SaveSpriteFile(
                AGS::Common::String(spr_path.c_str()),
                sprites, loader->GetSpriteFile(),
                loader->GetSpriteFile()->GetStoreFlags(),
                loader->GetSpriteFile()->GetSpriteCompression(),
                index);

            if (err)
            {
                // Save the updated index file
                std::string idx_path = project2->GetProjectDir() + "/sprindex.dat";
                AGS::Common::SaveSpriteIndex(
                    AGS::Common::String(idx_path.c_str()), index);

                // Evict old texture from cache
                auto& tex_cache = editor_.GetApp().GetTextureCache();
                tex_cache.EvictSprite(spr_id);

                // Reload the sprite file so loader picks up changes
                loader->Close();
                loader->Open(project2->GetProjectDir());

                // Update game data metrics
                auto* gd = project2->GetGameData();
                if (gd)
                {
                    for (auto& s : gd->sprites)
                    {
                        if (s.id == spr_id)
                        {
                            s.width = bmp->GetWidth();
                            s.height = bmp->GetHeight();
                            s.color_depth = bmp->GetColorDepth();
                            break;
                        }
                    }
                }

                needs_refresh_ = true;
                editor_.GetLogPanel().AddLog("[Sprite] Replaced sprite %d from: %s",
                    spr_id, path.c_str());
            }
            else
            {
                editor_.GetLogPanel().AddLog("[Sprite] Failed to save sprite file after replace.");
            }

            delete bmp;
        });
}

void SpriteManager::ReplaceFromSource()
{
    if (selected_sprites_.empty()) return;

    auto* project = editor_.GetApp().GetProject();
    auto* loader = project ? project->GetSpriteLoader() : nullptr;
    if (!loader || !loader->IsOpen() || !loader->GetSpriteFile()) return;

    auto* gd = project->GetGameData();
    if (!gd) return;

    std::string project_dir = project->GetProjectDir();
    int max_slot = loader->GetTopmostSprite();
    std::vector<std::pair<bool, AGS::Common::BitmapData>> sprite_data(max_slot + 1);
    std::vector<AGS::Common::Bitmap*> loaded_bitmaps;
    int replaced = 0;
    int failed = 0;

    for (int sel_id : selected_sprites_)
    {
        // Find the sprite info
        GameData::SpriteInfo* spr_info = nullptr;
        for (auto& s : gd->sprites)
        {
            if (s.id == sel_id) { spr_info = &s; break; }
        }
        if (!spr_info || !spr_info->source.HasSource())
        {
            editor_.GetLogPanel().AddLog("[Sprite] Sprite %d has no source file.", sel_id);
            failed++;
            continue;
        }

        std::string src_path = spr_info->source.source_file;
        if (!std::filesystem::path(src_path).is_absolute())
            src_path = project_dir + "/" + src_path;

        if (!std::filesystem::exists(src_path))
        {
            editor_.GetLogPanel().AddLog("[Sprite] Source not found for sprite %d: %s",
                sel_id, spr_info->source.source_file.c_str());
            failed++;
            continue;
        }

        auto* bmp = AGS::Common::BitmapHelper::LoadFromFile(src_path.c_str());
        if (!bmp)
        {
            editor_.GetLogPanel().AddLog("[Sprite] Failed to load source for sprite %d: %s",
                sel_id, src_path.c_str());
            failed++;
            continue;
        }

        // Apply import cropping
        if (spr_info->source.import_width > 0 && spr_info->source.import_height > 0 &&
            (spr_info->source.offset_x > 0 || spr_info->source.offset_y > 0 ||
             spr_info->source.import_width < bmp->GetWidth() ||
             spr_info->source.import_height < bmp->GetHeight()))
        {
            int crop_w = std::min(spr_info->source.import_width, bmp->GetWidth() - spr_info->source.offset_x);
            int crop_h = std::min(spr_info->source.import_height, bmp->GetHeight() - spr_info->source.offset_y);
            if (crop_w > 0 && crop_h > 0)
            {
                auto* cropped = AGS::Common::BitmapHelper::CreateBitmap(
                    crop_w, crop_h, bmp->GetColorDepth());
                cropped->Blit(bmp, spr_info->source.offset_x, spr_info->source.offset_y,
                             0, 0, crop_w, crop_h);
                delete bmp;
                bmp = cropped;
            }
        }

        sprite_data[sel_id] = { true, bmp->GetBitmapData() };
        loaded_bitmaps.push_back(bmp);

        spr_info->width = bmp->GetWidth();
        spr_info->height = bmp->GetHeight();
        spr_info->color_depth = bmp->GetColorDepth();
        replaced++;
    }

    if (replaced > 0)
    {
        std::string spr_path = project_dir + "/acsprset.spr";
        AGS::Common::SpriteFileIndex index;
        auto err = AGS::Common::SaveSpriteFile(
            AGS::Common::String(spr_path.c_str()),
            sprite_data, loader->GetSpriteFile(),
            loader->GetSpriteFile()->GetStoreFlags(),
            loader->GetSpriteFile()->GetSpriteCompression(),
            index);

        if (err)
        {
            std::string idx_path = project_dir + "/sprindex.dat";
            AGS::Common::SaveSpriteIndex(
                AGS::Common::String(idx_path.c_str()), index);

            auto& tex_cache = editor_.GetApp().GetTextureCache();
            for (int sel_id : selected_sprites_)
                tex_cache.EvictSprite(sel_id);

            loader->Close();
            loader->Open(project_dir);
            needs_refresh_ = true;

            editor_.GetLogPanel().AddLog("[Sprite] Replaced %d sprite(s) from source (%d failed).",
                replaced, failed);
        }
        else
        {
            editor_.GetLogPanel().AddLog("[Sprite] Failed to save sprite file after replace from source.");
        }
    }
    else
    {
        editor_.GetLogPanel().AddLog("[Sprite] No sprites could be replaced from source (%d failed).", failed);
    }

    for (auto* b : loaded_bitmaps) delete b;
}

void SpriteManager::DuplicateSprite()
{
    if (selected_sprites_.empty()) return;

    auto* project = editor_.GetApp().GetProject();
    auto* loader = project ? project->GetSpriteLoader() : nullptr;
    if (!loader || !loader->IsOpen() || !loader->GetSpriteFile()) return;

    auto* sprite_file = loader->GetSpriteFile();
    int old_max = loader->GetTopmostSprite();

    // Load pixel data for each selected sprite and append as new slots
    std::vector<std::pair<int, AGS::Common::PixelBuffer>> to_duplicate;
    for (int sel_id : selected_sprites_)
    {
        AGS::Common::PixelBuffer pixbuf;
        auto err = sprite_file->LoadSprite(sel_id, pixbuf);
        if (err && pixbuf.GetData())
            to_duplicate.push_back({ sel_id, std::move(pixbuf) });
        else
            editor_.GetLogPanel().AddLog("[Sprite] Failed to load sprite %d for duplication.", sel_id);
    }

    if (to_duplicate.empty()) return;

    // Build sprites vector: empty for existing slots (read from file),
    // new duplicates appended at end
    int new_max = old_max + (int)to_duplicate.size();
    std::vector<std::pair<bool, AGS::Common::BitmapData>> sprites(new_max + 1);

    // Create BitmapData wrappers for the duplicated sprites at new slots
    std::vector<int> new_ids;
    for (int i = 0; i < (int)to_duplicate.size(); i++)
    {
        int new_slot = old_max + 1 + i;
        auto& pb = to_duplicate[i].second;
        sprites[new_slot] = { true,
            AGS::Common::BitmapData(
                const_cast<uint8_t*>(pb.GetData()), pb.GetDataSize(),
                pb.GetStride(), pb.GetWidth(), pb.GetHeight(), pb.GetFormat()) };
        new_ids.push_back(new_slot);
    }

    // Rewrite sprite file
    std::string spr_path = project->GetProjectDir() + "/acsprset.spr";
    AGS::Common::SpriteFileIndex index;
    auto save_err = AGS::Common::SaveSpriteFile(
        AGS::Common::String(spr_path.c_str()),
        sprites, sprite_file,
        sprite_file->GetStoreFlags(),
        sprite_file->GetSpriteCompression(),
        index);

    if (save_err)
    {
        std::string idx_path = project->GetProjectDir() + "/sprindex.dat";
        AGS::Common::SaveSpriteIndex(
            AGS::Common::String(idx_path.c_str()), index);

        // Reload sprite loader
        loader->Close();
        loader->Open(project->GetProjectDir());

        // Update game data — add new sprite entries
        auto* gd = project->GetGameData();
        if (gd)
        {
            for (int i = 0; i < (int)to_duplicate.size(); i++)
            {
                int src_id = to_duplicate[i].first;
                int new_id = new_ids[i];
                GameData::SpriteInfo si;
                si.id = new_id;
                si.width = to_duplicate[i].second.GetWidth();
                si.height = to_duplicate[i].second.GetHeight();
                si.color_depth = to_duplicate[i].second.GetColorDepth();
                gd->sprites.push_back(si);
                editor_.GetLogPanel().AddLog("[Sprite] Duplicated sprite %d → %d", src_id, new_id);
            }
        }

        needs_refresh_ = true;

        // Record undo action
        SpriteUndoAction undo_action;
        undo_action.type = SpriteUndoAction::Duplicate;
        undo_action.sprite_ids = new_ids;
        PushUndo(std::move(undo_action));
    }
    else
    {
        editor_.GetLogPanel().AddLog("[Sprite] Failed to save sprite file after duplication.");
    }
}

void SpriteManager::AssignToView()
{
    if (selected_sprites_.empty()) return;

    // Open the assignment popup — actual assignment happens in Draw loop
    ImGui::OpenPopup("AssignToViewPopup");
}

// Drawn inline after AssignToView sets the popup open (called from DrawContextMenu area)

void SpriteManager::ShowUsage()
{
    if (primary_sprite_ < 0) return;

    auto* project = editor_.GetApp().GetProject();
    auto* gd = (project && project->IsLoaded()) ? project->GetGameData() : nullptr;
    if (!gd)
    {
        editor_.GetLogPanel().AddLog("[Sprite] No project loaded — cannot scan usage.");
        return;
    }

    int spr = primary_sprite_;
    int usages = 0;

    editor_.GetLogPanel().AddLog("[Sprite] === Usage of sprite %d ===", spr);

    // Check views (all frames)
    for (const auto& view : gd->views)
    {
        for (int li = 0; li < (int)view.loops.size(); li++)
        {
            const auto& loop = view.loops[li];
            for (int fi = 0; fi < (int)loop.frames.size(); fi++)
            {
                if (loop.frames[fi].sprite_id == spr)
                {
                    editor_.GetLogPanel().AddLog("  View %d (%s), Loop %d, Frame %d",
                                                  view.id, view.name.c_str(), li, fi);
                    usages++;
                }
            }
        }
    }

    // Check GUI controls (button images)
    for (const auto& gui : gd->guis)
    {
        for (const auto& ctrl : gui.controls)
        {
            if (ctrl.image == spr)
            {
                editor_.GetLogPanel().AddLog("  GUI %d (%s), Control %s (image)",
                                              gui.id, gui.name.c_str(), ctrl.name.c_str());
                usages++;
            }
        }
    }

    // Check inventory items
    for (const auto& inv : gd->inventory_items)
    {
        if (inv.image == spr)
        {
            editor_.GetLogPanel().AddLog("  Inventory item %d (%s) - image",
                                          inv.id, inv.script_name.c_str());
            usages++;
        }
        if (inv.cursor_image == spr)
        {
            editor_.GetLogPanel().AddLog("  Inventory item %d (%s) - cursor image",
                                          inv.id, inv.script_name.c_str());
            usages++;
        }
    }

    // Check cursors
    for (const auto& cur : gd->cursors)
    {
        if (cur.image == spr)
        {
            editor_.GetLogPanel().AddLog("  Cursor %d (%s) - image",
                                          cur.id, cur.name.c_str());
            usages++;
        }
    }

    if (usages == 0)
        editor_.GetLogPanel().AddLog("  No usages found.");
    else
        editor_.GetLogPanel().AddLog("  Total: %d usage(s).", usages);
}

// ============================================================================
// Multi-select helpers
// ============================================================================

bool SpriteManager::IsSpriteSelected(int id) const
{
    return std::find(selected_sprites_.begin(), selected_sprites_.end(), id) != selected_sprites_.end();
}

void SpriteManager::ToggleSpriteSelection(int id)
{
    auto it = std::find(selected_sprites_.begin(), selected_sprites_.end(), id);
    if (it != selected_sprites_.end())
        selected_sprites_.erase(it);
    else
    {
        selected_sprites_.push_back(id);
        primary_sprite_ = id;
    }
}

// ============================================================================
// Undo / Redo
// ============================================================================

void SpriteManager::PushUndo(SpriteUndoAction action)
{
    undo_stack_.push_back(std::move(action));
    redo_stack_.clear();
    // Cap undo stack size
    if (undo_stack_.size() > 100)
        undo_stack_.erase(undo_stack_.begin());
}

void SpriteManager::Undo()
{
    if (undo_stack_.empty()) return;

    auto action = std::move(undo_stack_.back());
    undo_stack_.pop_back();

    auto* project = editor_.GetApp().GetProject();
    auto* gd = project ? project->GetGameData() : nullptr;
    if (!gd) return;

    SpriteUndoAction redo_action;

    switch (action.type)
    {
    case SpriteUndoAction::Delete:
        // Restore deleted sprites — put saved metadata back
        redo_action.type = SpriteUndoAction::Delete;
        redo_action.sprite_ids = action.sprite_ids;
        for (const auto& saved : action.saved_info)
        {
            bool found = false;
            for (auto& s : gd->sprites)
            {
                if (s.id == saved.id)
                {
                    // Save current (zeroed) state for redo
                    redo_action.saved_info.push_back(s);
                    // Restore original metadata
                    s = saved;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                // Sprite info was removed entirely, re-add it
                GameData::SpriteInfo spriteInfo;
                spriteInfo.id = saved.id;
                redo_action.saved_info.push_back(spriteInfo);
                gd->sprites.push_back(saved);
            }
        }
        needs_refresh_ = true;
        editor_.GetLogPanel().AddLog("[Sprite] Undo: restored %d deleted sprite(s).",
            (int)action.sprite_ids.size());
        break;

    case SpriteUndoAction::Import:
    case SpriteUndoAction::Duplicate:
        // Undo import/duplicate: delete the newly added sprites
        redo_action.type = action.type;
        redo_action.sprite_ids = action.sprite_ids;
        for (int id : action.sprite_ids)
        {
            for (auto& s : gd->sprites)
            {
                if (s.id == id)
                {
                    redo_action.saved_info.push_back(s);
                    s.width = 0;
                    s.height = 0;
                    s.color_depth = 0;
                    break;
                }
            }
            editor_.GetApp().GetTextureCache().EvictSprite(id);
        }
        needs_refresh_ = true;
        editor_.GetLogPanel().AddLog("[Sprite] Undo: removed %d imported/duplicated sprite(s).",
            (int)action.sprite_ids.size());
        break;
    }

    redo_stack_.push_back(std::move(redo_action));
}

void SpriteManager::Redo()
{
    if (redo_stack_.empty()) return;

    auto action = std::move(redo_stack_.back());
    redo_stack_.pop_back();

    auto* project = editor_.GetApp().GetProject();
    auto* gd = project ? project->GetGameData() : nullptr;
    if (!gd) return;

    SpriteUndoAction undo_action;

    switch (action.type)
    {
    case SpriteUndoAction::Delete:
        // Redo delete — zero out sprite metadata again
        undo_action.type = SpriteUndoAction::Delete;
        undo_action.sprite_ids = action.sprite_ids;
        for (int id : action.sprite_ids)
        {
            for (auto& s : gd->sprites)
            {
                if (s.id == id)
                {
                    undo_action.saved_info.push_back(s);
                    s.width = 0;
                    s.height = 0;
                    s.color_depth = 0;
                    break;
                }
            }
            editor_.GetApp().GetTextureCache().EvictSprite(id);
        }
        needs_refresh_ = true;
        editor_.GetLogPanel().AddLog("[Sprite] Redo: deleted %d sprite(s).",
            (int)action.sprite_ids.size());
        break;

    case SpriteUndoAction::Import:
    case SpriteUndoAction::Duplicate:
        // Redo import/duplicate — restore saved metadata
        undo_action.type = action.type;
        undo_action.sprite_ids = action.sprite_ids;
        for (const auto& saved : action.saved_info)
        {
            bool found = false;
            for (auto& s : gd->sprites)
            {
                if (s.id == saved.id)
                {
                    s = saved;
                    found = true;
                    break;
                }
            }
            if (!found)
                gd->sprites.push_back(saved);
        }
        needs_refresh_ = true;
        editor_.GetLogPanel().AddLog("[Sprite] Redo: restored %d imported/duplicated sprite(s).",
            (int)action.sprite_ids.size());
        break;
    }

    undo_stack_.push_back(std::move(undo_action));
}

} // namespace AGSEditor
