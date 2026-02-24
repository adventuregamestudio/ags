// AGS Editor ImGui - Debug Log Pane implementation
#include "debug_log_pane.h"
#include "ui/editor_ui.h"
#include "core/dpi_helper.h"
#include "app.h"

#include "imgui.h"

#include <chrono>
#include <cstring>
#include <ctime>
#include <algorithm>

namespace AGSEditor
{

DebugLogPane::DebugLogPane(EditorUI& editor)
    : editor_(editor)
{
    // Add some initial entries to show the log is working
    AddLog(LogLevel::Info, "Editor", "AGS Editor ImGui started");
    AddLog(LogLevel::Info, "Editor", "Debug log pane initialized");
}

void DebugLogPane::AddLog(LogLevel level, const std::string& category,
    const std::string& message)
{
    LogEntry entry;
    entry.level = level;
    entry.category = category;
    entry.message = message;
    entry.timestamp = GetTimestamp();

    entries_.push_back(entry);

    // Trim if too many entries
    while (entries_.size() > kMaxEntries)
        entries_.pop_front();

    if (auto_scroll_)
        scroll_to_bottom_ = true;
}

void DebugLogPane::Clear()
{
    entries_.clear();
}

std::string DebugLogPane::GetTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    struct tm tm_info;
    localtime_r(&time, &tm_info);

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d",
        tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec,
        (int)ms.count());
    return buf;
}

ImVec4 DebugLogPane::GetLevelColor(LogLevel level) const
{
    switch (level)
    {
    case LogLevel::Debug:   return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    case LogLevel::Info:    return ImVec4(0.8f, 0.8f, 1.0f, 1.0f);
    case LogLevel::Warning: return ImVec4(1.0f, 0.85f, 0.3f, 1.0f);
    case LogLevel::Error:   return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
    }
    return ImVec4(1, 1, 1, 1);
}

const char* DebugLogPane::GetLevelLabel(LogLevel level) const
{
    switch (level)
    {
    case LogLevel::Debug:   return "DBG";
    case LogLevel::Info:    return "INF";
    case LogLevel::Warning: return "WRN";
    case LogLevel::Error:   return "ERR";
    }
    return "???";
}

void DebugLogPane::Draw()
{
    // Toolbar row
    if (ImGui::Button("Clear"))
        Clear();

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &auto_scroll_);

    ImGui::SameLine();
    ImGui::Separator();

    ImGui::SameLine();
    ImGui::Checkbox("Debug", &show_debug_);
    ImGui::SameLine();
    ImGui::Checkbox("Info", &show_info_);
    ImGui::SameLine();
    ImGui::Checkbox("Warnings", &show_warnings_);
    ImGui::SameLine();
    ImGui::Checkbox("Errors", &show_errors_);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(Dpi(200));
    ImGui::InputTextWithHint("##filter", "Filter...", filter_text_, sizeof(filter_text_));

    ImGui::SameLine();
    ImGui::Text("(%d entries)", (int)entries_.size());

    ImGui::Separator();

    // Log table
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (ImGui::BeginChild("LogScroll", ImVec2(0, avail.y), ImGuiChildFlags_None))
    {
        if (ImGui::BeginTable("LogTable", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable |
            ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, Dpi(100));
            ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, Dpi(40));
            ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, Dpi(80));
            ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            bool has_filter = (filter_text_[0] != '\0');

            for (const auto& entry : entries_)
            {
                // Level filter
                switch (entry.level)
                {
                case LogLevel::Debug:   if (!show_debug_) continue; break;
                case LogLevel::Info:    if (!show_info_) continue; break;
                case LogLevel::Warning: if (!show_warnings_) continue; break;
                case LogLevel::Error:   if (!show_errors_) continue; break;
                }

                // Text filter (case-insensitive)
                if (has_filter)
                {
                    std::string lower_msg = entry.message;
                    std::string lower_cat = entry.category;
                    std::string lower_filter = filter_text_;
                    std::transform(lower_msg.begin(), lower_msg.end(),
                        lower_msg.begin(), ::tolower);
                    std::transform(lower_cat.begin(), lower_cat.end(),
                        lower_cat.begin(), ::tolower);
                    std::transform(lower_filter.begin(), lower_filter.end(),
                        lower_filter.begin(), ::tolower);

                    if (lower_msg.find(lower_filter) == std::string::npos &&
                        lower_cat.find(lower_filter) == std::string::npos)
                        continue;
                }

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(entry.timestamp.c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(GetLevelColor(entry.level),
                    "%s", GetLevelLabel(entry.level));

                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(entry.category.c_str());

                ImGui::TableSetColumnIndex(3);
                ImGui::TextWrapped("%s", entry.message.c_str());
            }

            if (scroll_to_bottom_)
            {
                ImGui::SetScrollHereY(1.0f);
                scroll_to_bottom_ = false;
            }

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}

} // namespace AGSEditor
