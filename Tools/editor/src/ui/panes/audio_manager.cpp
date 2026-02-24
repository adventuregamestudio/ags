// AGS Editor ImGui - Audio Manager pane implementation
// Uses real audio clip data from GameData. Plays WAV and OGG files via AudioPlayer.
#include "audio_manager.h"
#include "ui/editor_ui.h"
#include "ui/log_panel.h"
#include "ui/file_dialog.h"
#include "ui/folder_tree_widget.h"
#include "project/project.h"
#include "project/game_data.h"
#include "core/script_name_validator.h"
#include "core/audio_player.h"
#include "core/path_utils.h"
#include "app.h"
#include "core/dpi_helper.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "../IconsLucide.h"
#include "ac/dynobj/scriptaudioclip.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <ctime>
#include <sys/stat.h>

namespace fs = std::filesystem;

namespace AGSEditor
{

// Convert a file extension to an AudioFileType enum value
static int AudioFileTypeFromExtension(const std::string& ext)
{
    if (ext == ".ogg") return eAudioFileOGG;
    if (ext == ".mp3") return eAudioFileMP3;
    if (ext == ".wav") return eAudioFileWAV;
    if (ext == ".voc") return eAudioFileVOC;
    if (ext == ".mid" || ext == ".midi") return eAudioFileMIDI;
    if (ext == ".mod" || ext == ".xm" || ext == ".s3m" || ext == ".it") return eAudioFileMOD;
    return eAudioFileUndefined;
}

// Get a display name for an AudioFileType enum value
static const char* AudioFileTypeName(int file_type)
{
    switch (file_type)
    {
    case eAudioFileOGG:  return "OGG";
    case eAudioFileMP3:  return "MP3";
    case eAudioFileWAV:  return "WAV";
    case eAudioFileVOC:  return "VOC";
    case eAudioFileMIDI: return "MIDI";
    case eAudioFileMOD:  return "MOD";
    default:             return "Unknown";
    }
}

// Get a file's last write time as a UTC string (yyyy-MM-dd HH:mm:ssZ)
static std::string GetFileLastModifiedUTC(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return "";
    std::tm utc{};
    gmtime_r(&st.st_mtime, &utc);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%SZ", &utc);
    return buf;
}

AudioManager::AudioManager(EditorUI& editor)
    : editor_(editor)
{
}

void AudioManager::RefreshFromProject()
{
    needs_refresh_ = true;
    selected_clip_ = -1;
}

void AudioManager::Draw()
{
    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
    {
        ImGui::TextDisabled("No project loaded. Open a game to view audio clips.");
        return;
    }

    float tree_width = Dpi(240);
    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("AudioTree", ImVec2(tree_width, avail.y), ImGuiChildFlags_Borders);
    DrawAudioTree();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("AudioProps", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
    auto* gd = project->GetGameData();
    if (show_type_manager_)
    {
        DrawAudioTypeManager();
    }
    else if (gd && selected_clip_ >= 0 && selected_clip_ < (int)gd->audio_clips.size())
    {
        DrawAudioProperties();
        ImGui::Separator();
        DrawAudioPreview();
    }
    else
    {
        ImGui::TextDisabled("Select an audio clip to view its properties.");
    }
    ImGui::EndChild();
}

void AudioManager::DrawAudioTree()
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = project ? project->GetGameData() : nullptr;
    if (!gd)
    {
        ImGui::TextDisabled("No game data.");
        return;
    }

    const auto& clips = gd->audio_clips;

    // Toolbar
    if (ImGui::Button("Import..."))
    {
        auto* proj = editor_.GetApp().GetProject();
        std::string default_dir = (proj && proj->IsLoaded()) ? proj->GetProjectDir() : ".";
        FileDialog::OpenMulti("Import Audio Clips",
            ".wav,.ogg,.mp3,.mid,.flac{Audio Files}",
            default_dir,
            [this](const std::vector<std::string>& paths) {
                auto* import_project = editor_.GetApp().GetProject();
                auto* game_data = import_project ? import_project->GetGameData() : nullptr;
                if (!game_data || paths.empty()) return;

                int imported = 0;
                for (const auto& path : paths)
                {
                    // Determine audio type from extension
                    std::string ext;
                    auto dot = path.rfind('.');
                    if (dot != std::string::npos)
                    {
                        ext = path.substr(dot);
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    }

                    // Determine type from extension, using actual type IDs if available
                    int audio_type = 3; // Default to Sound (ID 3)
                    if (!game_data->audio_clip_types.empty())
                    {
                        // Try to find a "Sound" type as default
                        for (const auto& ct : game_data->audio_clip_types)
                        {
                            std::string lower = ct.name;
                            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                            if (lower.find("sound") != std::string::npos)
                            {
                                audio_type = ct.id;
                                break;
                            }
                        }
                        // For MIDI, try to find "Music" type
                        if (ext == ".mid" || ext == ".midi")
                        {
                            for (const auto& ct : game_data->audio_clip_types)
                            {
                                std::string lower = ct.name;
                                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                                if (lower.find("music") != std::string::npos)
                                {
                                    audio_type = ct.id;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        audio_type = (ext == ".mid" || ext == ".midi") ? 2 : 3;
                    }

                    // Extract filename from path
                    std::string filename;
                    auto last_sep = path.rfind('/');
                    if (last_sep == std::string::npos)
                        last_sep = path.rfind('\\');
                    filename = (last_sep != std::string::npos) ? path.substr(last_sep + 1) : path;

                    // Generate script name from filename (without extension)
                    std::string script_name = filename;
                    auto ext_dot = script_name.rfind('.');
                    if (ext_dot != std::string::npos)
                        script_name = script_name.substr(0, ext_dot);
                    // Sanitize: only letters, digits, underscores
                    for (auto& ch : script_name)
                    {
                        if (!isalnum(ch) && ch != '_')
                            ch = '_';
                    }
                    if (!script_name.empty() && isdigit(script_name[0]))
                        script_name = "a" + script_name;

                    // Find next available ID
                    int new_id = 0;
                    for (const auto& c : game_data->audio_clips)
                        if (c.id >= new_id) new_id = c.id + 1;

                    // Generate cache filename: au{ID:X6}.{ext}
                    char cache_name[32];
                    snprintf(cache_name, sizeof(cache_name), "au%06X%s", new_id, ext.c_str());

                    // Copy file to project's AudioCache directory
                    std::string dest_dir = import_project->GetProjectDir() + "/AudioCache";
                    fs::create_directories(dest_dir);
                    std::string dest_path = dest_dir + "/" + cache_name;
                    try {
                        fs::copy_file(path, dest_path, fs::copy_options::overwrite_existing);
                    } catch (const std::exception& e) {
                        editor_.GetLogPanel().AddLog("[Audio] Failed to copy file: %s", e.what());
                        continue;
                    }

                    // Add to game data
                    GameData::AudioClipInfo clip;
                    clip.id = new_id;
                    clip.name = script_name;
                    clip.filename = std::string("AudioCache/") + cache_name;
                    clip.source_filename = MakeRelativePath(path, import_project->GetProjectDir());
                    clip.type = audio_type;
                    game_data->audio_clips.push_back(clip);
                    imported++;

                    editor_.GetLogPanel().AddLog("[Audio] Imported: %s (ID %d)",
                        filename.c_str(), new_id);
                }

                if (imported > 0)
                {
                    editor_.GetLogPanel().AddLog("[Audio] Imported %d file(s)", imported);
                    needs_refresh_ = true;
                }
            });
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete") && selected_clip_ >= 0)
    {
        confirm_delete_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Types..."))
    {
        show_type_manager_ = !show_type_manager_;
        if (show_type_manager_) selected_clip_ = -1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Replace Paths..."))
    {
        show_replace_paths_ = true;
        // Pre-fill old_path from first clip with a source filename
        replace_old_path_[0] = '\0';
        replace_new_path_[0] = '\0';
        for (const auto& c : clips)
        {
            if (!c.source_filename.empty())
            {
                // Extract directory part
                auto slash = c.source_filename.rfind('/');
                if (slash == std::string::npos) slash = c.source_filename.rfind('\\');
                if (slash != std::string::npos)
                    strncpy(replace_old_path_, c.source_filename.substr(0, slash).c_str(),
                        sizeof(replace_old_path_) - 1);
                else
                    strncpy(replace_old_path_, c.source_filename.c_str(),
                        sizeof(replace_old_path_) - 1);
                strncpy(replace_new_path_, replace_old_path_, sizeof(replace_new_path_) - 1);
                break;
            }
        }
    }
    // Replace Source Paths popup
    if (show_replace_paths_)
    {
        ImGui::OpenPopup("Replace Source Paths");
        show_replace_paths_ = false;
    }
    if (ImGui::BeginPopupModal("Replace Source Paths", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Replace part of the source path for all audio clips.");
        ImGui::Text("Relative paths are assumed relative to the project folder.");
        ImGui::Spacing();
        ImGui::SetNextItemWidth(Dpi(400));
        ImGui::InputText("Old path", replace_old_path_, sizeof(replace_old_path_));
        ImGui::SetNextItemWidth(Dpi(400));
        ImGui::InputText("New path", replace_new_path_, sizeof(replace_new_path_));
        ImGui::Spacing();

        bool same = (strcmp(replace_old_path_, replace_new_path_) == 0);
        bool empty = (strlen(replace_old_path_) == 0);
        if (same || empty) ImGui::BeginDisabled();
        if (ImGui::Button("Replace", ImVec2(Dpi(120), 0)))
        {
            int count = 0;
            std::string old_str(replace_old_path_);
            std::string new_str(replace_new_path_);
            for (auto& c : gd->audio_clips)
            {
                if (c.source_filename.empty()) continue;
                size_t pos = c.source_filename.find(old_str);
                if (pos != std::string::npos)
                {
                    c.source_filename.replace(pos, old_str.length(), new_str);
                    count++;
                }
            }
            editor_.GetLogPanel().AddLog("[Audio] Updated source paths for %d clip(s).", count);
            ImGui::CloseCurrentPopup();
        }
        if (same || empty) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(Dpi(120), 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::Separator();

    // Deletion confirmation dialog
    if (confirm_delete_)
    {
        ImGui::OpenPopup("Confirm Delete Audio");
        confirm_delete_ = false;
    }
    if (ImGui::BeginPopupModal("Confirm Delete Audio", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto* game_data = project->GetGameData();
        if (game_data && selected_clip_ >= 0 && selected_clip_ < (int)game_data->audio_clips.size())
        {
            const auto& clip = game_data->audio_clips[selected_clip_];
            ImGui::Text("Delete audio clip '%s' (%s)?", clip.name.c_str(), clip.filename.c_str());
            ImGui::Text("The file will also be removed from disk.");
            ImGui::Separator();
            if (ImGui::Button("Delete", ImVec2(Dpi(120), 0)))
            {
                editor_.GetLogPanel().AddLog("[Audio] Deleted audio clip %d: %s",
                    clip.id, clip.name.c_str());

                if (!clip.filename.empty())
                {
                    std::string file_path = ResolvePath(clip.filename, project->GetProjectDir());
                    try {
                        if (fs::exists(file_path))
                            fs::remove(file_path);
                    } catch (const std::exception& e) {
                        editor_.GetLogPanel().AddLog("[Audio] Failed to delete file: %s", e.what());
                    }
                }

                game_data->audio_clips.erase(game_data->audio_clips.begin() + selected_clip_);
                selected_clip_ = -1;
                needs_refresh_ = true;
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

    // Filter buttons - use actual audio clip types if available
    ImGui::Checkbox("Folders", &show_folder_tree_);
    ImGui::SameLine();
    ImGui::RadioButton("All", &selected_folder_, 0);
    if (!gd->audio_clip_types.empty())
    {
        for (int t = 0; t < (int)gd->audio_clip_types.size(); t++)
        {
            ImGui::SameLine();
            // Use type index + 1 as filter ID (0 = All)
            ImGui::RadioButton(gd->audio_clip_types[t].name.c_str(), &selected_folder_, t + 1);
        }
    }
    else
    {
        ImGui::SameLine();
        ImGui::RadioButton("Music", &selected_folder_, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Sounds", &selected_folder_, 2);
    }
    ImGui::Separator();

    // Counts per type
    if (!gd->audio_clip_types.empty())
    {
        int total = (int)clips.size();
        ImGui::TextDisabled("%d clips total", total);
        for (const auto& ct : gd->audio_clip_types)
        {
            int count = 0;
            for (const auto& c : clips)
                if (c.type == ct.id) count++;
            if (count > 0)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("| %s: %d", ct.name.c_str(), count);
            }
        }
    }
    else
    {
        int music_count = 0, sound_count = 0;
        for (const auto& c : clips)
        {
            if (c.type == 1) music_count++;
            else sound_count++;
        }
        ImGui::TextDisabled("%d clips (%d music, %d sound)", (int)clips.size(), music_count, sound_count);
    }
    ImGui::Separator();

    // Folder tree (hierarchical)
    if (show_folder_tree_)
    {
        const auto* sel = static_cast<const FolderInfo*>(selected_clip_folder_);
        const auto* new_sel = DrawFolderTreeWidget(sel, gd->audio_clip_folders, "All Clips", &gd->audio_clip_folders);
        if (new_sel != sel)
            selected_clip_folder_ = new_sel;
        ImGui::Separator();
    }

    // Build folder filter set
    std::set<int> clip_folder_ids;
    if (selected_clip_folder_)
        CollectFolderItemIds(*static_cast<const FolderInfo*>(selected_clip_folder_), clip_folder_ids);

    // Clip list
    for (int i = 0; i < (int)clips.size(); i++)
    {
        const auto& clip = clips[i];

        // Filter by folder tree
        if (selected_clip_folder_ && clip_folder_ids.find(clip.id) == clip_folder_ids.end())
            continue;

        // Filter by type
        if (selected_folder_ > 0)
        {
            if (!gd->audio_clip_types.empty())
            {
                // Filter by actual type ID
                int filter_idx = selected_folder_ - 1;
                if (filter_idx >= 0 && filter_idx < (int)gd->audio_clip_types.size())
                {
                    if (clip.type != gd->audio_clip_types[filter_idx].id) continue;
                }
            }
            else
            {
                // Legacy fallback
                if (selected_folder_ == 1 && clip.type != 1) continue;
                if (selected_folder_ == 2 && clip.type != 0) continue;
            }
        }

        // Determine icon based on type name
        bool is_music = false;
        for (const auto& ct : gd->audio_clip_types)
        {
            if (ct.id == clip.type)
            {
                std::string lower_name = ct.name;
                std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
                is_music = (lower_name.find("music") != std::string::npos);
                break;
            }
        }
        const char* type_icon = is_music
            ? (g_icons_loaded ? ICON_LC_MUSIC : "[M]")
            : (g_icons_loaded ? ICON_LC_VOLUME_2 : "[S]");
        char label[256];
        if (!clip.name.empty())
            snprintf(label, sizeof(label), "%s %d: %s", type_icon, clip.id, clip.name.c_str());
        else
            snprintf(label, sizeof(label), "%s %d: %s", type_icon, clip.id, clip.filename.c_str());

        if (ImGui::Selectable(label, selected_clip_ == i))
            selected_clip_ = i;
        BeginItemDragSource(clip.id, label);

        // Context menu per clip
        if (ImGui::BeginPopupContextItem())
        {
            selected_clip_ = i;
            if (ImGui::MenuItem("Rename..."))
            {
                // Select the clip so properties panel shows it for renaming
            }
            if (ImGui::MenuItem("Duplicate"))
            {
                GameData::AudioClipInfo dup = clip;
                dup.id = 0;
                for (const auto& c : clips)
                    if (c.id >= dup.id) dup.id = c.id + 1;
                dup.name += "_copy";
                gd->audio_clips.push_back(dup);
                editor_.GetLogPanel().AddLog("[Audio] Duplicated clip %d as %d: %s",
                    clip.id, dup.id, dup.name.c_str());
                needs_refresh_ = true;
            }
            if (ImGui::MenuItem("Replace Source File..."))
            {
                replace_clip_idx_ = i;
                auto* proj = editor_.GetApp().GetProject();
                std::string dir = (proj && proj->IsLoaded()) ? proj->GetProjectDir() : ".";
                FileDialog::Open(FileDialogType::OpenFile, "Replace Audio Source",
                    ".wav,.ogg,.mp3,.mid,.flac{Audio Files}",
                    dir,
                    [this](const std::string& path) { ReplaceSourceFile(path); });
            }
            if (ImGui::MenuItem("Force Reimport", nullptr, false, !clip.source_filename.empty()))
            {
                ForceReimportClip(i);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete..."))
            {
                confirm_delete_ = true;
            }
            ImGui::EndPopup();
        }
    }
}

void AudioManager::DrawAudioProperties()
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd || selected_clip_ < 0 || selected_clip_ >= (int)gd->audio_clips.size())
        return;

    auto& clip = gd->audio_clips[selected_clip_];

    ImGui::Text("Audio Clip ID: %d", clip.id);
    ImGui::Separator();

    float field_w = Dpi(200);

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // --- Prominent file info display ---
        // Get file type display name from stored enum value or extension
        const char* file_type_name = AudioFileTypeName(clip.file_type);

        // Highlighted file info box
        ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg);
        ImGui::BeginChild("##fileinfobox", ImVec2(0, Dpi(90)), ImGuiChildFlags_Borders);
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", clip.filename.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), " [%s]", file_type_name);
        if (!clip.source_filename.empty())
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Source: %s", clip.source_filename.c_str());
        }
        if (!clip.file_last_modified.empty())
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Modified: %s", clip.file_last_modified.c_str());
        }
        if (ImGui::SmallButton("Replace Source File..."))
        {
            replace_clip_idx_ = selected_clip_;
            auto* proj = editor_.GetApp().GetProject();
            std::string dir = (proj && proj->IsLoaded()) ? proj->GetProjectDir() : ".";
            FileDialog::Open(FileDialogType::OpenFile, "Replace Audio Source",
                ".wav,.ogg,.mp3,.mid,.flac{Audio Files}",
                dir,
                [this](const std::string& path) { ReplaceSourceFile(path); });
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Spacing();

        ImGui::SetNextItemWidth(field_w);
        ImGui::InputText("Script Name", &clip.name);
        {
            std::string err = ValidateScriptName(*gd, clip.name, "Audio", clip.id);
            if (!err.empty())
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", err.c_str());
        }

        // Audio type combo - use dynamically loaded clip types
        ImGui::SetNextItemWidth(field_w);
        if (!gd->audio_clip_types.empty())
        {
            // Find current selection index
            int current_idx = 0;
            for (int i = 0; i < (int)gd->audio_clip_types.size(); i++)
            {
                if (gd->audio_clip_types[i].id == clip.type)
                {
                    current_idx = i;
                    break;
                }
            }
            if (ImGui::BeginCombo("Audio Type", gd->audio_clip_types[current_idx].name.c_str()))
            {
                for (int i = 0; i < (int)gd->audio_clip_types.size(); i++)
                {
                    bool selected = (clip.type == gd->audio_clip_types[i].id);
                    if (ImGui::Selectable(gd->audio_clip_types[i].name.c_str(), selected))
                        clip.type = gd->audio_clip_types[i].id;
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            // Fallback if no clip types loaded
            const char* type_items[] = { "Sound", "Music" };
            int type_idx = (clip.type == 2) ? 1 : 0;
            if (ImGui::Combo("Audio Type", &type_idx, type_items, 2))
                clip.type = (type_idx == 1) ? 2 : 3;
        }
    }

    if (ImGui::CollapsingHeader("Defaults"))
    {
        ImGui::SetNextItemWidth(field_w);
        ImGui::SliderInt("Default Volume", &clip.default_volume, 0, 100, "%d%%");
        ImGui::SetNextItemWidth(field_w);
        ImGui::SliderInt("Default Priority", &clip.default_priority, 0, 100);

        const char* repeat_items[] = { "Once", "Loop" };
        ImGui::SetNextItemWidth(field_w);
        ImGui::Combo("Default Repeat", &clip.default_repeat, repeat_items, 2);

        const char* bundle_items[] = { "In main game file", "In audio VOX" };
        ImGui::SetNextItemWidth(field_w);
        ImGui::Combo("Bundling Type", &clip.bundling_type, bundle_items, 2);
    }
}

void AudioManager::DrawAudioPreview()
{
    auto* project = editor_.GetApp().GetProject();
    auto* gd = project ? project->GetGameData() : nullptr;
    if (!gd || selected_clip_ < 0 || selected_clip_ >= (int)gd->audio_clips.size())
        return;

    if (!ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    const auto& clip = gd->audio_clips[selected_clip_];
    auto& player = editor_.GetApp().GetAudioPlayer();

    // Update player state (detect playback finished, pause device)
    player.Update();

    // Build full path to the audio file
    std::string audio_path;
    std::vector<std::string> tried_paths;
    if (!clip.filename.empty())
    {
        fs::path project_dir = project->GetProjectDir();
        // Normalize backslashes in filename - AGF stores Windows-style paths
        // (e.g. "AudioCache\au000001.ogg") which don't work on Linux/macOS
        std::string norm_filename = NormalizePath(clip.filename);
        // Also extract just the bare filename without any directory prefix
        std::string bare_name = fs::path(norm_filename).filename().string();
        // clip.filename may already include "AudioCache/" prefix
        // or be a bare filename. Try direct path first, then common subdirectories.
        fs::path candidates[] = {
            project_dir / norm_filename,
            project_dir / "AudioCache" / bare_name,
            project_dir / "Sound" / bare_name,
            project_dir / "Music" / bare_name,
        };
        for (const auto& p : candidates)
        {
            tried_paths.push_back(p.string());
            if (fs::exists(p))
            {
                audio_path = p.string();
                break;
            }
        }
    }

    // Check if this is a playable format (WAV or OGG)
    bool is_playable = false;
    if (!clip.filename.empty())
    {
        std::string lower = clip.filename;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        is_playable = (lower.size() >= 4 &&
            (lower.substr(lower.size() - 4) == ".wav" ||
             lower.substr(lower.size() - 4) == ".ogg"));
    }

    // Detect if this clip is currently loaded in the player
    bool is_loaded = (!audio_path.empty() && player.GetLoadedFile() == audio_path);

    // Playback controls
    bool can_play = !audio_path.empty() && is_playable;

    if (can_play)
    {
        if (player.IsPlaying() && is_loaded)
        {
            if (ImGui::Button("Pause"))
                player.Pause();
        }
        else
        {
            if (ImGui::Button("Play"))
            {
                if (!is_loaded)
                {
                    if (player.LoadFile(audio_path))
                    {
                        player.Play();
                        editor_.GetLogPanel().AddLog("[Audio] Playing: %s", clip.filename.c_str());
                    }
                    else
                    {
                        editor_.GetLogPanel().AddLog("[Audio] Failed to load: %s", audio_path.c_str());
                    }
                }
                else
                {
                    player.Play();
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Stop"))
        {
            player.Stop();
        }

        // Progress bar
        if (is_loaded)
        {
            float progress = player.GetProgress();
            float duration = player.GetDuration();
            float position = player.GetPosition();

            char overlay[64];
            snprintf(overlay, sizeof(overlay), "%.1f / %.1f sec", position, duration);
            ImGui::ProgressBar(progress, ImVec2(-1, Dpi(16)), overlay);

            // State indicator
            const char* state_str = "Stopped";
            if (player.GetState() == AudioPlayerState::Playing) state_str = "Playing";
            else if (player.GetState() == AudioPlayerState::Paused) state_str = "Paused";
            ImGui::TextDisabled("State: %s", state_str);
        }
    }
    else if (!audio_path.empty() && !is_playable)
    {
        ImGui::TextDisabled("Preview: only WAV and OGG files can be played.");
        ImGui::TextDisabled("This is a %s file. MP3/MIDI/MOD playback is not yet supported.",
                           clip.filename.substr(clip.filename.rfind('.') + 1).c_str());
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Audio file not found: %s", clip.filename.c_str());
        if (!tried_paths.empty())
        {
            ImGui::TextDisabled("Searched paths:");
            for (const auto& tp : tried_paths)
                ImGui::TextDisabled("  %s", tp.c_str());
        }
    }

    // Waveform visualization
    if (is_loaded && !player.GetWaveformData().empty())
    {
        ImGui::Spacing();
        ImGui::Text("Waveform:");

        const auto& waveform = player.GetWaveformData();
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float wave_h = std::max(Dpi(60), std::min(avail.y - Dpi(10), Dpi(120)));
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size(avail.x, wave_h);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Background
        dl->AddRectFilled(canvas_pos,
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(20, 20, 35, 255));
        dl->AddRect(canvas_pos,
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(60, 60, 80, 255));

        // Center line
        float cy = canvas_pos.y + canvas_size.y * 0.5f;
        dl->AddLine(ImVec2(canvas_pos.x, cy),
                    ImVec2(canvas_pos.x + canvas_size.x, cy),
                    IM_COL32(50, 50, 70, 255));

        // Draw waveform
        int num_points = (int)waveform.size();
        if (num_points > 1)
        {
            float x_step = canvas_size.x / (float)(num_points - 1);
            ImU32 wave_color = IM_COL32(80, 180, 255, 200);

            for (int i = 1; i < num_points; i++)
            {
                float x0 = canvas_pos.x + (float)(i - 1) * x_step;
                float x1 = canvas_pos.x + (float)i * x_step;
                float y0 = cy - waveform[i - 1] * (canvas_size.y * 0.45f);
                float y1 = cy - waveform[i] * (canvas_size.y * 0.45f);
                dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), wave_color);
            }
        }

        // Playback position indicator
        float progress = player.GetProgress();
        float px = canvas_pos.x + progress * canvas_size.x;
        dl->AddLine(ImVec2(px, canvas_pos.y),
                    ImVec2(px, canvas_pos.y + canvas_size.y),
                    IM_COL32(255, 100, 100, 200), 2.0f);

        ImGui::Dummy(canvas_size);
    }
}

void AudioManager::DrawAudioTypeManager()
{
    auto* gd = editor_.GetApp().GetProject()->GetGameData();
    if (!gd) return;

    ImGui::Text("Audio Clip Types");
    ImGui::Separator();

    float field_w = Dpi(200);

    // Add new type button
    if (ImGui::Button("New Type"))
    {
        GameData::AudioClipTypeInfo new_type;
        // Find next available ID
        new_type.id = 1;
        for (const auto& t : gd->audio_clip_types)
            if (t.id >= new_type.id) new_type.id = t.id + 1;
        new_type.name = "New Type";
        new_type.max_channels = 1;
        gd->audio_clip_types.push_back(new_type);
        selected_type_ = (int)gd->audio_clip_types.size() - 1;
    }
    ImGui::SameLine();
    bool can_delete_type = selected_type_ >= 0 &&
        selected_type_ < (int)gd->audio_clip_types.size();
    if (!can_delete_type) ImGui::BeginDisabled();
    if (ImGui::Button("Delete Type"))
        confirm_delete_type_ = true;
    if (!can_delete_type) ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Back to Clips"))
        show_type_manager_ = false;

    ImGui::Separator();

    // Delete type confirmation
    if (confirm_delete_type_)
    {
        ImGui::OpenPopup("Confirm Delete Type");
        confirm_delete_type_ = false;
    }
    if (ImGui::BeginPopupModal("Confirm Delete Type", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (selected_type_ >= 0 && selected_type_ < (int)gd->audio_clip_types.size())
        {
            auto& dt = gd->audio_clip_types[selected_type_];
            // Count clips using this type
            int usage_count = 0;
            for (const auto& c : gd->audio_clips)
                if (c.type == dt.id) usage_count++;

            ImGui::Text("Delete audio clip type '%s' (ID %d)?", dt.name.c_str(), dt.id);
            if (usage_count > 0)
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                    "Warning: %d audio clip(s) are using this type!", usage_count);

            ImGui::Separator();
            if (ImGui::Button("Delete", ImVec2(Dpi(120), 0)))
            {
                gd->audio_clip_types.erase(gd->audio_clip_types.begin() + selected_type_);
                selected_type_ = -1;
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

    // Type list
    ImGui::BeginChild("##TypeList", ImVec2(0, Dpi(120)), ImGuiChildFlags_Borders);
    for (int i = 0; i < (int)gd->audio_clip_types.size(); i++)
    {
        auto& ct = gd->audio_clip_types[i];
        char label[128];
        snprintf(label, sizeof(label), "%d: %s", ct.id, ct.name.c_str());
        if (ImGui::Selectable(label, selected_type_ == i))
            selected_type_ = i;
    }
    ImGui::EndChild();

    ImGui::Separator();

    // Properties of selected type
    if (selected_type_ >= 0 && selected_type_ < (int)gd->audio_clip_types.size())
    {
        auto& ct = gd->audio_clip_types[selected_type_];

        ImGui::Text("Type ID: %d", ct.id);

        ImGui::SetNextItemWidth(field_w);
        ImGui::InputText("Name", &ct.name);

        ImGui::SetNextItemWidth(field_w);
        ImGui::InputInt("Max Channels", &ct.max_channels);
        if (ct.max_channels < 0) ct.max_channels = 0;

        ImGui::SetNextItemWidth(field_w);
        ImGui::SliderInt("Volume Reduction (Speech)", &ct.volume_reduction_while_speech, 0, 100, "%d%%");

        const char* crossfade_items[] = { "No", "Slow", "Fast" };
        ImGui::SetNextItemWidth(field_w);
        ImGui::Combo("Crossfade Clips", &ct.crossfade_speed, crossfade_items, 3);

        ImGui::Checkbox("Backwards Compatibility Type", &ct.backwards_compat_type);

        // Show usage count
        int usage = 0;
        for (const auto& c : gd->audio_clips)
            if (c.type == ct.id) usage++;
        ImGui::TextDisabled("Used by %d clip(s)", usage);
    }
    else
    {
        ImGui::TextDisabled("Select an audio clip type to edit its properties.");
    }
}

void AudioManager::ForceReimportClip(int clip_idx)
{
    auto* proj = editor_.GetApp().GetProject();
    auto* gd = proj ? proj->GetGameData() : nullptr;
    if (!gd || clip_idx < 0 || clip_idx >= (int)gd->audio_clips.size())
        return;

    auto& clip = gd->audio_clips[clip_idx];
    if (clip.source_filename.empty())
    {
        editor_.GetLogPanel().AddLog("[Audio] Clip %d (%s): no source file recorded.",
            clip.id, clip.name.c_str());
        return;
    }

    namespace fs = std::filesystem;
    std::string proj_dir = proj->GetProjectDir();
    std::string source_path = proj_dir + "/" + clip.source_filename;

    // Normalize path separators
    std::replace(source_path.begin(), source_path.end(), '\\', '/');

    if (!fs::exists(source_path))
    {
        editor_.GetLogPanel().AddLog("[Audio] Source file not found: %s", source_path.c_str());
        return;
    }

    std::string cache_path = proj_dir + "/" + clip.filename;
    std::replace(cache_path.begin(), cache_path.end(), '\\', '/');

    // Ensure AudioCache directory exists
    std::error_code ec;
    auto cache_dir = fs::path(cache_path).parent_path();
    fs::create_directories(cache_dir, ec);

    fs::copy_file(source_path, cache_path, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        editor_.GetLogPanel().AddLog("[Audio] Reimport error for clip %d: %s",
            clip.id, ec.message().c_str());
        return;
    }

    editor_.GetLogPanel().AddLog("[Audio] Reimported clip %d (%s) from: %s",
        clip.id, clip.name.c_str(), clip.source_filename.c_str());
    needs_refresh_ = true;
}

void AudioManager::ReplaceSourceFile(const std::string& new_path)
{
    auto* proj = editor_.GetApp().GetProject();
    auto* gd = proj ? proj->GetGameData() : nullptr;
    if (!gd || replace_clip_idx_ < 0 || replace_clip_idx_ >= (int)gd->audio_clips.size())
        return;

    auto& clip = gd->audio_clips[replace_clip_idx_];
    namespace fs = std::filesystem;

    // Determine extension
    std::string ext;
    {
        auto dot = new_path.rfind('.');
        if (dot != std::string::npos)
        {
            ext = new_path.substr(dot);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        }
    }

    // Generate proper cache filename: au{ID:X6}.{ext}
    char cache_name[32];
    snprintf(cache_name, sizeof(cache_name), "au%06X%s", clip.id, ext.c_str());

    std::string proj_dir = proj->GetProjectDir();
    std::string dest = proj_dir + "/AudioCache/" + cache_name;

    // Ensure AudioCache directory exists
    std::error_code ec;
    fs::create_directories(proj_dir + "/AudioCache", ec);

    // Copy new file to AudioCache
    fs::copy_file(new_path, dest, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        editor_.GetLogPanel().AddLog("[Audio] Error copying file: %s", ec.message().c_str());
        return;
    }

    // Remove old cache file if name differs
    std::string old_full = proj_dir + "/" + clip.filename;
    std::string new_cache = std::string("AudioCache/") + cache_name;
    if (clip.filename != new_cache && fs::exists(old_full))
        fs::remove(old_full, ec);

    // Update clip metadata
    clip.filename = new_cache;
    clip.source_filename = MakeRelativePath(new_path, proj_dir);

    // Recalculate file_type from extension using AudioFileType enum
    clip.file_type = AudioFileTypeFromExtension(ext);

    // Track file modification date
    std::string cache_full = proj_dir + "/" + new_cache;
    clip.file_last_modified = GetFileLastModifiedUTC(cache_full);

    editor_.GetLogPanel().AddLog("[Audio] Replaced source for clip %d (%s): %s",
        clip.id, clip.name.c_str(), cache_name);
    needs_refresh_ = true;
}

} // namespace AGSEditor
