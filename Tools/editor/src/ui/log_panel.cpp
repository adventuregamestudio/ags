// AGS Editor ImGui - Log Panel implementation
// Logs are written to both the in-editor pane and stderr/stdout console.
#include "log_panel.h"
#include "editor_ui.h"
#include "core/logger.h"

namespace AGSEditor
{

LogPanel::LogPanel(EditorUI& editor)
    : editor_(editor)
{
    Clear();

    // Register this LogPanel as the Logger callback so that all components
    // (compiler, build system, etc.) can log through Logger::Log() and
    // have their messages appear in the editor pane.
    Logger::SetLogCallback([this](const char* message) {
        AddLogDirect(message);
    });
}

LogPanel::~LogPanel()
{
    // Unregister callback to avoid dangling pointer
    Logger::SetLogCallback(nullptr);
}

void LogPanel::Clear()
{
    buf_.clear();
    line_offsets_.clear();
    line_offsets_.push_back(0);
}

void LogPanel::AddLog(const char* fmt, ...)
{
    // Format the message
    char formatted[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(formatted, sizeof(formatted), fmt, args);
    va_end(args);

    // Also write to stderr for console visibility
    fprintf(stderr, "%s\n", formatted);

    // Append to the internal buffer
    int old_size = buf_.size();
    buf_.append(formatted);
    buf_.append("\n");

    for (int new_size = buf_.size(); old_size < new_size; old_size++)
    {
        if (buf_[old_size] == '\n')
            line_offsets_.push_back(old_size + 1);
    }
}

void LogPanel::AddLogDirect(const char* message)
{
    // Append pre-formatted message (no stderr write -- Logger already did that)
    int old_size = buf_.size();
    buf_.append(message);
    buf_.append("\n");

    for (int new_size = buf_.size(); old_size < new_size; old_size++)
    {
        if (buf_[old_size] == '\n')
            line_offsets_.push_back(old_size + 1);
    }
}

void LogPanel::Draw()
{
    ImGui::Begin("Log", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);

    // Options
    if (ImGui::BeginPopup("LogOptions"))
    {
        ImGui::Checkbox("Auto-scroll", &auto_scroll_);
        ImGui::Separator();
        ImGui::Checkbox("Show Info", &show_info_);
        ImGui::Checkbox("Show Warnings", &show_warnings_);
        ImGui::Checkbox("Show Errors", &show_errors_);
        ImGui::Separator();
        if (ImGui::MenuItem("Copy All to Clipboard"))
        {
            ImGui::SetClipboardText(buf_.begin());
        }
        ImGui::EndPopup();
    }

    // Buttons
    if (ImGui::Button("Clear"))
        Clear();
    ImGui::SameLine();
    if (ImGui::Button("Options"))
        ImGui::OpenPopup("LogOptions");
    ImGui::SameLine();
    if (ImGui::Button("Copy"))
        ImGui::SetClipboardText(buf_.begin());
    ImGui::SameLine();
    ImGui::TextDisabled(" | Output Log");

    ImGui::Separator();

    // Log contents
    if (ImGui::BeginChild("LogScrolling", ImVec2(0, 0), ImGuiChildFlags_None,
        ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        const char* buf_start = buf_.begin();
        const char* buf_end = buf_.end();

        ImGuiListClipper clipper;
        clipper.Begin(line_offsets_.Size);
        while (clipper.Step())
        {
            for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
            {
                const char* line_start = buf_start + line_offsets_[line_no];
                const char* line_end = (line_no + 1 < line_offsets_.Size)
                    ? (buf_start + line_offsets_[line_no + 1] - 1)
                    : buf_end;

                // Determine log level for color coding and filtering
                bool is_error = false;
                bool is_warn = false;
                bool is_info = false;
                ImVec4 color(1.0f, 1.0f, 1.0f, 1.0f);
                bool has_color = false;

                if (line_end > line_start)
                {
                    // Quick check for tag at start of line (avoids full string construction)
                    size_t len = (size_t)(line_end - line_start);
                    if (len >= 7)
                    {
                        if (strncmp(line_start, "[Error]", 7) == 0)
                        {
                            is_error = true;
                            color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                            has_color = true;
                        }
                        else if (strncmp(line_start, "[Warn]", 6) == 0)
                        {
                            is_warn = true;
                            color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
                            has_color = true;
                        }
                        else if (strncmp(line_start, "[Info]", 6) == 0)
                        {
                            is_info = true;
                            color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
                            has_color = true;
                        }
                    }
                    // Fall back to searching in the line if tag is not at start
                    if (!has_color)
                    {
                        std::string line(line_start, line_end);
                        if (line.find("[Error]") != std::string::npos)
                        {
                            is_error = true;
                            color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                            has_color = true;
                        }
                        else if (line.find("[Warn]") != std::string::npos)
                        {
                            is_warn = true;
                            color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
                            has_color = true;
                        }
                        else if (line.find("[Info]") != std::string::npos)
                        {
                            is_info = true;
                            color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
                            has_color = true;
                        }
                    }
                }

                // Apply filter
                if (is_error && !show_errors_) continue;
                if (is_warn && !show_warnings_) continue;
                if (is_info && !show_info_) continue;

                if (has_color) ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(line_start, line_end);

                // Right-click context menu on log lines
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    ImGui::OpenPopup("LogLineMenu");
                }

                if (has_color) ImGui::PopStyleColor();
            }
        }
        clipper.End();

        // Log line context menu
        if (ImGui::BeginPopup("LogLineMenu"))
        {
            if (ImGui::MenuItem("Copy Line"))
            {
                // Copy is handled per-line if needed
            }
            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();

        if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}

} // namespace AGSEditor
