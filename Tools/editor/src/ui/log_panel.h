// AGS Editor ImGui - Log Panel
// All log messages are written to both the in-editor log pane and stderr.
// Other components can use Logger::Log/Info/Warn/Error for unified output.
#pragma once

#include <cstdarg>

#include "imgui.h"

namespace AGSEditor
{

class EditorUI;

class LogPanel
{
public:
    explicit LogPanel(EditorUI& editor);
    ~LogPanel();

    void Draw();

    // Add a log message (also writes to stderr for console visibility)
    void AddLog(const char* fmt, ...) IM_FMTARGS(2);

    // Add a pre-formatted log message (no varargs, used by Logger callback)
    void AddLogDirect(const char* message);

    void Clear();

    // Filter controls
    bool show_info_ = true;
    bool show_warnings_ = true;
    bool show_errors_ = true;

private:
    EditorUI& editor_;
    ImGuiTextBuffer buf_;
    ImVector<int> line_offsets_;
    bool auto_scroll_ = true;
};

} // namespace AGSEditor
