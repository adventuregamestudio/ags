// AGS Editor ImGui - Script Debugger Controller
// Implements file-based IPC protocol for debugging AGS games.
// See debug_controller.h for protocol overview.

#include "pipeline/debug_controller.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdio>
#include <algorithm>

namespace fs = std::filesystem;

namespace AGSEditor
{

DebugController::DebugController() = default;

DebugController::~DebugController()
{
    if (IsActive())
        EndSession();
}

// -------------------------------------------------------------------------
// Session lifecycle
// -------------------------------------------------------------------------

void DebugController::StartSession(const std::string& working_dir)
{
    if (IsActive())
        EndSession();

    working_dir_ = working_dir;
    send_path_ = working_dir_ + "/dbgsend.tmp";
    recv_path_ = working_dir_ + "/dbgrecv.tmp";

    // Clean up any stale temp files from a previous session
    CleanupTempFiles();

    // Clear state
    synced_breakpoints_.clear();
    call_stack_.clear();
    current_script_.clear();
    current_line_ = 0;
    last_error_.clear();

    SetState(DebugState::Running);
}

void DebugController::EndSession()
{
    if (state_ != DebugState::NotRunning)
    {
        // Send EXIT command to engine
        SendCommand(MakeCommand("EXIT"));
    }

    CleanupTempFiles();
    synced_breakpoints_.clear();
    call_stack_.clear();
    current_script_.clear();
    current_line_ = 0;
    last_error_.clear();

    SetState(DebugState::NotRunning);
}

bool DebugController::Poll()
{
    if (state_ == DebugState::NotRunning)
        return false;

    DebugState old_state = state_;
    // Check for messages from engine
    while (ReadEngineMessage())
    {
        // Keep reading until no more messages
    }
    return state_ != old_state;
}

// -------------------------------------------------------------------------
// Debugger commands (editor -> engine)
// -------------------------------------------------------------------------

void DebugController::SyncBreakpoints(
    const std::unordered_map<std::string, std::unordered_set<int>>& breakpoints_by_script)
{
    if (state_ == DebugState::NotRunning)
        return;

    // Find breakpoints to delete (in synced but not in new)
    for (const auto& [script, lines] : synced_breakpoints_)
    {
        auto it = breakpoints_by_script.find(script);
        for (int line : lines)
        {
            if (it == breakpoints_by_script.end() || it->second.count(line) == 0)
            {
                SendCommand(MakeBreakCommand("DELBREAK", script, line));
            }
        }
    }

    // Find breakpoints to add (in new but not in synced)
    for (const auto& [script, lines] : breakpoints_by_script)
    {
        auto it = synced_breakpoints_.find(script);
        for (int line : lines)
        {
            if (it == synced_breakpoints_.end() || it->second.count(line) == 0)
            {
                SendCommand(MakeBreakCommand("SETBREAK", script, line));
            }
        }
    }

    synced_breakpoints_ = breakpoints_by_script;
}

void DebugController::Resume()
{
    if (state_ != DebugState::Paused)
        return;

    last_error_.clear();
    SendCommand(MakeCommand("RESUME"));
    SetState(DebugState::Running);
}

void DebugController::StepInto()
{
    if (state_ != DebugState::Paused)
        return;

    last_error_.clear();
    SendCommand(MakeCommand("STEP"));
    SetState(DebugState::Running);
}

// -------------------------------------------------------------------------
// IPC: Send command to engine
// -------------------------------------------------------------------------

void DebugController::SendCommand(const std::string& xml)
{
    if (send_path_.empty())
        return;

    // Wait for engine to consume previous message (file should not exist)
    // The engine deletes dbgsend.tmp after reading it.
    // We do a brief spin to avoid overwriting an unread message.
    for (int wait = 0; wait < 100 && fs::exists(send_path_); ++wait)
    {
        // Sleep ~10ms  (std::this_thread::sleep_for or usleep)
        struct timespec ts = {0, 10000000}; // 10ms
        nanosleep(&ts, nullptr);
    }

    std::ofstream out(send_path_, std::ios::binary);
    if (out.is_open())
    {
        out.write(xml.data(), (std::streamsize)xml.size());
    }
}

std::string DebugController::MakeCommand(const std::string& command,
                                          const std::string& extra_attrs) const
{
    std::string xml = "<Engine Command=\"" + command + "\"";
    if (!extra_attrs.empty())
        xml += " " + extra_attrs;
    xml += " />";
    return xml;
}

std::string DebugController::MakeBreakCommand(const std::string& cmd,
                                               const std::string& script_name,
                                               int line_number) const
{
    // Format matches the C# editor: <Engine Command="SETBREAK $scriptname$linenum$"></Engine>
    return "<Engine Command=\"" + cmd + " $" + script_name + "$"
           + std::to_string(line_number) + "$\"></Engine>";
}

// -------------------------------------------------------------------------
// IPC: Read message from engine
// -------------------------------------------------------------------------

bool DebugController::ReadEngineMessage()
{
    if (recv_path_.empty() || !fs::exists(recv_path_))
        return false;

    std::string content;
    {
        std::ifstream in(recv_path_, std::ios::binary);
        if (!in.is_open())
            return false;

        std::ostringstream ss;
        ss << in.rdbuf();
        content = ss.str();
    }

    // Delete the file so the engine knows we've consumed it
    std::error_code ec;
    fs::remove(recv_path_, ec);

    if (!content.empty())
        ParseEngineMessage(content);

    return true;
}

// -------------------------------------------------------------------------
// XML parsing (simple — protocol uses fixed format)
// -------------------------------------------------------------------------

void DebugController::ParseEngineMessage(const std::string& xml)
{
    // Engine messages look like:
    //   <?xml version="1.0" encoding="Windows-1252"?>
    //   <Debugger Command="BREAK">
    //     <ScriptState><![CDATA[callstack]]></ScriptState>
    //     <ErrorMessage><![CDATA[msg]]></ErrorMessage>   (only for ERROR)
    //   </Debugger>

    std::string command = ExtractAttribute(xml, "Command");

    if (command == "CYCLESTART" || command == "START")
    {
        // Engine has started and is ready for debugging
        // Send initial breakpoints, then READY
        // The caller should call SyncBreakpoints before or after this
        SendCommand(MakeCommand("START", "EditorWindow=\"0\""));

        // Send all currently synced breakpoints
        for (const auto& [script, lines] : synced_breakpoints_)
        {
            for (int line : lines)
            {
                SendCommand(MakeBreakCommand("SETBREAK", script, line));
            }
        }

        SendCommand(MakeCommand("READY", "EditorWindow=\"0\""));
    }
    else if (command == "CYCLEBREAK" || command == "BREAK")
    {
        // Engine hit a breakpoint or completed a step
        std::string callstack = ExtractCData(xml, "ScriptState");
        ParseCallStack(callstack);
        SetState(DebugState::Paused);
    }
    else if (command == "ERROR")
    {
        std::string callstack = ExtractCData(xml, "ScriptState");
        last_error_ = ExtractCData(xml, "ErrorMessage");
        ParseCallStack(callstack);
        SetState(DebugState::Paused);

        if (on_error_)
            on_error_(last_error_);
    }
    else if (command == "EXIT")
    {
        SetState(DebugState::NotRunning);
    }
    else if (command == "LOG")
    {
        // Log messages — could forward to editor log panel
        // For now, just ignore
    }
}

void DebugController::ParseCallStack(const std::string& callstack_text)
{
    call_stack_.clear();
    current_script_.clear();
    current_line_ = 0;

    if (callstack_text.empty())
        return;

    // The engine's call stack format (from cc_get_callstack) is:
    //   "in ScriptName.asc, line 42\n"
    //   "from ScriptName.asc, line 30\n"
    // Each line is a call stack entry; first is the current execution point.
    std::istringstream stream(callstack_text);
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.empty())
            continue;

        CallStackEntry entry;

        // Try to parse: "in <script>, line <num>" or "from <script>, line <num>"
        // Also handle the format without prefix
        size_t script_start = 0;
        if (line.substr(0, 3) == "in ")
            script_start = 3;
        else if (line.substr(0, 5) == "from ")
            script_start = 5;

        size_t comma = line.find(", line ", script_start);
        if (comma != std::string::npos)
        {
            entry.script_name = line.substr(script_start, comma - script_start);
            // Trim whitespace
            while (!entry.script_name.empty() && entry.script_name.back() == ' ')
                entry.script_name.pop_back();
            entry.line_number = std::atoi(line.c_str() + comma + 7);
        }
        else
        {
            // Fallback: just store the whole line
            entry.script_name = line;
            entry.line_number = 0;
        }

        call_stack_.push_back(entry);
    }

    // Set current execution point from top of stack
    if (!call_stack_.empty())
    {
        current_script_ = call_stack_[0].script_name;
        current_line_ = call_stack_[0].line_number;
    }
}

std::string DebugController::ExtractCData(const std::string& xml, const std::string& tag)
{
    // Find <Tag><![CDATA[...]]></Tag>
    std::string open = "<" + tag + "><![CDATA[";
    std::string close = "]]></" + tag + ">";

    size_t start = xml.find(open);
    if (start == std::string::npos)
        return "";

    start += open.size();
    size_t end = xml.find(close, start);
    if (end == std::string::npos)
        return "";

    return xml.substr(start, end - start);
}

std::string DebugController::ExtractAttribute(const std::string& xml, const std::string& attr)
{
    // Find attr="value"
    std::string pattern = attr + "=\"";
    size_t start = xml.find(pattern);
    if (start == std::string::npos)
        return "";

    start += pattern.size();
    size_t end = xml.find('"', start);
    if (end == std::string::npos)
        return "";

    return xml.substr(start, end - start);
}

void DebugController::CleanupTempFiles()
{
    std::error_code ec;
    if (!send_path_.empty())
        fs::remove(send_path_, ec);
    if (!recv_path_.empty())
        fs::remove(recv_path_, ec);
}

void DebugController::SetState(DebugState new_state)
{
    if (state_ == new_state)
        return;

    state_ = new_state;
    if (on_state_change_)
        on_state_change_(new_state);
}

} // namespace AGSEditor
