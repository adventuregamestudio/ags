#pragma once
// AGS Editor ImGui - Debug Log Pane
// Provides a debug log viewer for monitoring engine output and editor events

#include <string>
#include <deque>

#include "imgui.h"
#include "ui/editor_ui.h"

namespace AGSEditor
{

enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

struct LogEntry
{
    LogLevel level = LogLevel::Info;
    std::string category;   // e.g. "Editor", "Compiler", "Engine", "Script"
    std::string message;
    std::string timestamp;
};

class DebugLogPane : public EditorPane
{
public:
    explicit DebugLogPane(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Debug Log"; }

    // Add log entries programmatically
    void AddLog(LogLevel level, const std::string& category, const std::string& message);
    void Clear();

    // Convenience methods
    void LogDebug(const std::string& category, const std::string& msg)
        { AddLog(LogLevel::Debug, category, msg); }
    void LogInfo(const std::string& category, const std::string& msg)
        { AddLog(LogLevel::Info, category, msg); }
    void LogWarning(const std::string& category, const std::string& msg)
        { AddLog(LogLevel::Warning, category, msg); }
    void LogError(const std::string& category, const std::string& msg)
        { AddLog(LogLevel::Error, category, msg); }

private:
    EditorUI& editor_;

    std::deque<LogEntry> entries_;
    static constexpr size_t kMaxEntries = 5000;

    // Filters
    char filter_text_[256] = {};
    bool show_debug_ = true;
    bool show_info_ = true;
    bool show_warnings_ = true;
    bool show_errors_ = true;

    bool auto_scroll_ = true;
    bool scroll_to_bottom_ = false;

    std::string GetTimestamp() const;
    ImVec4 GetLevelColor(LogLevel level) const;
    const char* GetLevelLabel(LogLevel level) const;
};

} // namespace AGSEditor
