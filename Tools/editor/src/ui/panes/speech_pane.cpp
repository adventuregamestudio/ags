// AGS Editor ImGui - Speech Pane
// Shows information about the Speech directory and voice-over files.

#include "speech_pane.h"
#include "app.h"
#include "project/project.h"
#include "core/dpi_helper.h"
#include "imgui.h"

#include <filesystem>

namespace AGSEditor
{

SpeechPane::SpeechPane(EditorUI& editor)
    : editor_(editor)
{
}

void SpeechPane::RefreshFileList()
{
    files_.clear();
    subdirs_.clear();
    total_size_ = 0;
    ogg_count_ = 0;
    mp3_count_ = 0;
    wav_count_ = 0;
    other_count_ = 0;

    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded()) return;

    std::string speech_dir = project->GetProjectDir() + "/Speech";

    auto scanDir = [&](const std::string& dir) {
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
        {
            std::string fname = entry.path().filename().string();
            if (entry.is_directory())
            {
                if (dir == speech_dir)
                    subdirs_.push_back(fname);
                continue;
            }
            if (!entry.is_regular_file()) continue;

            SpeechFileInfo info;
            info.filename = fname;
            info.size = (size_t)entry.file_size(ec);

            size_t dot = fname.rfind('.');
            if (dot != std::string::npos)
            {
                info.extension = fname.substr(dot);
                std::string ext_lower = info.extension;
                for (auto& c : ext_lower) c = (char)tolower(c);
                if (ext_lower == ".ogg") ogg_count_++;
                else if (ext_lower == ".mp3") mp3_count_++;
                else if (ext_lower == ".wav") wav_count_++;
                else other_count_++;
            }
            else
            {
                other_count_++;
            }

            total_size_ += info.size;
            files_.push_back(info);
        }
    };

    scanDir(speech_dir);
    needs_refresh_ = false;
}

static std::string FormatFileSize(size_t bytes)
{
    if (bytes < 1024)
        return std::to_string(bytes) + " B";
    else if (bytes < 1024 * 1024)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
        return buf;
    }
    else
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f MB", bytes / (1024.0 * 1024.0));
        return buf;
    }
}

void SpeechPane::Draw()
{
    if (needs_refresh_)
        RefreshFileList();

    auto* project = editor_.GetApp().GetProject();
    if (!project || !project->IsLoaded())
    {
        ImGui::TextDisabled("No project loaded.");
        return;
    }

    std::string speech_dir = project->GetProjectDir() + "/Speech";

    ImGui::TextWrapped("The Speech folder contains voice-over clips for your game. "
        "These are packed into speech.vox during the build.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Directory info
    ImGui::Text("Speech Directory:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), "%s", speech_dir.c_str());

    ImGui::Spacing();

    // Summary
    ImGui::Text("Total voice files: %d", (int)files_.size());
    if (!files_.empty())
    {
        ImGui::Text("  OGG: %d  |  MP3: %d  |  WAV: %d  |  Other: %d",
            ogg_count_, mp3_count_, wav_count_, other_count_);
        ImGui::Text("Total size: %s", FormatFileSize(total_size_).c_str());
    }

    if (!subdirs_.empty())
    {
        ImGui::Spacing();
        ImGui::Text("Language subfolders: %d", (int)subdirs_.size());
        for (const auto& sd : subdirs_)
        {
            ImGui::BulletText("%s (will create sp_%s.vox)", sd.c_str(), sd.c_str());
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // File naming info
    auto* gd = project->GetGameData();
    if (gd)
    {
        // Check game speech naming setting
        ImGui::Text("Voice clip naming style:");
        ImGui::SameLine();
        // Check for UseOldVoiceClipNaming in general settings
        // New style: scriptname.linenum.ext  (e.g. cEgo.1.ogg)
        // Old style: ego1.ogg (4-char + number)
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "New style (ScriptName.LineNumber.ext)");
    }

    ImGui::Spacing();

    // Buttons
    if (ImGui::Button("Refresh"))
    {
        needs_refresh_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Open Speech Folder"))
    {
#if defined(__linux__)
        std::string cmd = "xdg-open \"" + speech_dir + "\" &";
        system(cmd.c_str());
#elif defined(__APPLE__)
        std::string cmd = "open \"" + speech_dir + "\"";
        system(cmd.c_str());
#elif defined(_WIN32)
        std::string cmd = "explorer \"" + speech_dir + "\"";
        system(cmd.c_str());
#endif
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // File list table
    if (files_.empty())
    {
        ImGui::TextDisabled("No voice files found in the Speech directory.");
        ImGui::TextDisabled("Add speech files to 'Speech/' in your project folder.");
    }
    else
    {
        ImGui::Text("Voice Files:");
        if (ImGui::BeginTable("SpeechFiles", 3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable,
            ImVec2(0, ImGui::GetContentRegionAvail().y)))
        {
            ImGui::TableSetupColumn("Filename", ImGuiTableColumnFlags_DefaultSort);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, Dpi(60));
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, Dpi(80));
            ImGui::TableHeadersRow();

            for (const auto& file : files_)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(file.filename.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(file.extension.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%s", FormatFileSize(file.size).c_str());
            }
            ImGui::EndTable();
        }
    }
}

} // namespace AGSEditor
