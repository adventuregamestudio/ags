// AGS Editor ImGui - Script Debugger Controller
// Manages the debug session between the editor and the AGS engine.
// Uses file-based IPC (dbgsend.tmp / dbgrecv.tmp) matching the engine's
// FileBasedAGSDebugger transport for cross-platform compatibility.
//
// Protocol overview (XML messages):
//   Editor -> Engine:  written to dbgsend.tmp, engine reads & deletes
//   Engine -> Editor:  written to dbgrecv.tmp, editor reads & deletes
//
// Editor->Engine commands: START, READY, SETBREAK, DELBREAK, RESUME, STEP, EXIT
// Engine->Editor commands: BREAK (with call stack), ERROR, EXIT, LOG, START
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace AGSEditor
{

// Debug session state — mirrors C# editor's DebugState enum
enum class DebugState
{
    NotRunning,     // No debug session active
    Running,        // Engine running, game executing normally
    Paused          // Engine paused at a breakpoint or after a step
};

// Parsed call stack entry from the engine's BREAK message
struct CallStackEntry
{
    std::string script_name;    // e.g. "GlobalScript.asc"
    int line_number = 0;        // 1-based line number
    std::string function_name;  // if available from the call stack text
};

// A breakpoint as tracked by the debug controller
struct DebugBreakpoint
{
    std::string script_name;    // e.g. "GlobalScript.asc"
    int line_number = 0;        // 1-based
};

// -------------------------------------------------------------------------
// DebugController — manages the debug protocol with the running engine
// -------------------------------------------------------------------------
class DebugController
{
public:
    DebugController();
    ~DebugController();

    // --- Session lifecycle ---

    // Start a debug session. Sets state to Running.
    // working_dir is where dbgsend.tmp / dbgrecv.tmp will be created.
    void StartSession(const std::string& working_dir);

    // End the debug session. Sends EXIT to engine, cleans up temp files.
    void EndSession();

    // Poll for messages from the engine. Call this every frame during a debug session.
    // Returns true if the debug state changed (e.g. engine hit a breakpoint).
    bool Poll();

    // --- Debugger commands (editor -> engine) ---

    // Send all current breakpoints to the engine.
    // breakpoints_by_script: map of script_name -> set of 1-based line numbers
    void SyncBreakpoints(const std::unordered_map<std::string, std::unordered_set<int>>& breakpoints_by_script);

    // Resume execution after a pause
    void Resume();

    // Step to the next script line
    void StepInto();

    // --- State queries ---
    DebugState GetState() const { return state_; }
    bool IsActive() const { return state_ != DebugState::NotRunning; }
    bool IsPaused() const { return state_ == DebugState::Paused; }
    bool IsRunning() const { return state_ == DebugState::Running; }

    // The call stack from the last BREAK message
    const std::vector<CallStackEntry>& GetCallStack() const { return call_stack_; }

    // The error message if the engine sent an ERROR command
    const std::string& GetLastError() const { return last_error_; }
    bool HasError() const { return !last_error_.empty(); }

    // Current execution point (top of call stack when paused)
    const std::string& GetCurrentScript() const { return current_script_; }
    int GetCurrentLine() const { return current_line_; }

    // --- Callbacks ---
    using StateChangeCallback = std::function<void(DebugState new_state)>;
    void SetStateChangeCallback(StateChangeCallback cb) { on_state_change_ = std::move(cb); }

    using ErrorCallback = std::function<void(const std::string& error_msg)>;
    void SetErrorCallback(ErrorCallback cb) { on_error_ = std::move(cb); }

private:
    // Send an XML command to the engine via dbgsend.tmp
    void SendCommand(const std::string& xml);

    // Build an XML command string: <Engine Command="CMD" [attrs] />
    std::string MakeCommand(const std::string& command,
                            const std::string& extra_attrs = "") const;

    // Build a SETBREAK/DELBREAK command
    std::string MakeBreakCommand(const std::string& cmd,
                                 const std::string& script_name,
                                 int line_number) const;

    // Read and parse a message from dbgrecv.tmp (engine -> editor)
    // Returns true if a message was read.
    bool ReadEngineMessage();

    // Parse the engine's XML debugger message
    void ParseEngineMessage(const std::string& xml);

    // Parse a call stack string from BREAK/ERROR messages into entries
    void ParseCallStack(const std::string& callstack_text);

    // Extract CDATA content between <Tag><![CDATA[...]]></Tag>
    static std::string ExtractCData(const std::string& xml, const std::string& tag);

    // Extract attribute value from XML: attr="value"
    static std::string ExtractAttribute(const std::string& xml,
                                        const std::string& attr);

    // Clean up temp files
    void CleanupTempFiles();

    // Set state and fire callback
    void SetState(DebugState new_state);

    // --- State ---
    DebugState state_ = DebugState::NotRunning;
    std::string working_dir_;

    // Paths to the IPC temp files (set in StartSession)
    std::string send_path_;     // editor -> engine (dbgsend.tmp)
    std::string recv_path_;     // engine -> editor (dbgrecv.tmp)

    // Currently synced breakpoints (to compute deltas)
    std::unordered_map<std::string, std::unordered_set<int>> synced_breakpoints_;

    // Call stack from last BREAK
    std::vector<CallStackEntry> call_stack_;
    std::string current_script_;
    int current_line_ = 0;

    // Error state
    std::string last_error_;

    // Callbacks
    StateChangeCallback on_state_change_;
    ErrorCallback on_error_;
};

} // namespace AGSEditor
