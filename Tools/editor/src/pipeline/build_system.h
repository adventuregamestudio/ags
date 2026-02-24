// AGS Editor ImGui - Build System
// Full game build pipeline: script compilation, data file generation,
// asset packaging, engine launching, and multi-platform support.
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <atomic>
#include <memory>

#include "pipeline/debug_controller.h"

namespace AGSEditor
{

class Project;
struct GameData;

// -------------------------------------------------------------------------
// Build targets — mirrors the C# editor's IBuildTarget hierarchy
// -------------------------------------------------------------------------
enum class BuildTarget
{
    DataFile,       // Always runs first: compiles DTA + creates game.ags pack
    Linux,          // Copies Linux engine binary + shared libs
    Windows,        // Copies Windows engine executable + DLLs
    MacOS,          // Copies macOS engine app bundle
    Web,            // Emscripten / WebAssembly build
    Debug,          // Minimal debug build for F5 testing
    Count
};

const char* BuildTargetName(BuildTarget target);
const char* BuildTargetDirName(BuildTarget target);

// -------------------------------------------------------------------------
// Build configuration
// -------------------------------------------------------------------------
struct BuildConfig
{
    bool debug_mode = true;             // compile with DEBUG macro
    bool line_numbers = true;           // include line numbers in compiled scripts

    // Target platforms
    bool targets[(int)BuildTarget::Count] = {
        true,   // DataFile — always on
        true,   // Linux
        false,  // Windows
        false,  // macOS
        false,  // Web
        false   // Debug (set when running via F5)
    };

    // Paths
    std::string output_base_dir;        // e.g. <project_dir>/Compiled
    std::string engine_linux_path;      // path to `ags` binary
    std::string engine_windows_path;    // path to `acwin.exe`
    std::string engine_macos_path;      // path to macOS app
    std::string engine_web_path;        // path to Emscripten build

    // Script API
    std::string script_api_version = "Highest";
    std::string script_compat_level = "Highest";

    // Package options
    int split_size_mb = 0;              // 0 = no splitting

    void SetDefaults(const std::string& project_dir);
};

// -------------------------------------------------------------------------
// Build messages (errors, warnings, info)
// -------------------------------------------------------------------------
enum class BuildMessageType
{
    Info,
    Warning,
    Error
};

struct BuildMessage
{
    BuildMessageType type = BuildMessageType::Info;
    std::string file;
    int line = 0;
    int column = 0;
    std::string message;
    std::string module_name;  // which script module
};

// -------------------------------------------------------------------------
// Build progress reporting
// -------------------------------------------------------------------------
struct BuildProgress
{
    int current_step = 0;
    int total_steps = 1;
    std::string step_description;
    float percentage = 0.0f;

    void Reset(int total)
    {
        current_step = 0;
        total_steps = total;
        step_description.clear();
        percentage = 0.0f;
    }

    void Advance(const std::string& desc)
    {
        current_step++;
        step_description = desc;
        percentage = (total_steps > 0) ? (float)current_step / (float)total_steps : 1.0f;
    }
};

// -------------------------------------------------------------------------
// Build result
// -------------------------------------------------------------------------
struct BuildResult
{
    bool success = false;
    std::vector<BuildMessage> messages;
    std::vector<std::string> output_files;
    double elapsed_seconds = 0.0;

    int ErrorCount() const;
    int WarningCount() const;

    void AddError(const std::string& file, int line, const std::string& msg,
                  const std::string& module = "");
    void AddWarning(const std::string& file, int line, const std::string& msg,
                    const std::string& module = "");
    void AddInfo(const std::string& msg);
};

// -------------------------------------------------------------------------
// Compiled script info (stored for packaging into DTA)
// -------------------------------------------------------------------------
struct CompiledScriptInfo
{
    std::string name;
    std::string obj_file;   // path to .o compiled script
    bool is_header = false;
};

// -------------------------------------------------------------------------
// Build System — orchestrates the full build pipeline
// -------------------------------------------------------------------------
class BuildSystem
{
public:
    BuildSystem();
    ~BuildSystem();

    // --- Main operations ---
    // Build the game (synchronous, call from UI thread)
    BuildResult BuildGame(Project& project, const BuildConfig& config);

    // Run the compiled game
    bool RunGame(Project& project, const BuildConfig& config, bool debug = false);

    // Run game setup dialog
    bool RunSetup(Project& project, const BuildConfig& config);

    // Stop a running game instance
    void StopGame();

    // --- State queries ---
    bool IsBuilding() const { return building_; }
    bool IsRunning() const { return game_pid_ > 0; }
    const BuildProgress& GetProgress() const { return progress_; }
    const BuildResult& GetLastResult() const { return last_result_; }
    const BuildConfig& GetConfig() const { return config_; }

    // --- Debug controller ---
    DebugController& GetDebugController() { return debug_controller_; }
    const DebugController& GetDebugController() const { return debug_controller_; }

    // Whether the current run was launched in debug mode
    bool IsDebugRun() const { return debug_run_active_; }

    // --- Configuration ---
    void SetConfig(const BuildConfig& config) { config_ = config; }
    BuildConfig& GetConfigMut() { return config_; }

    // --- Log callback ---
    using LogCallback = std::function<void(const std::string&)>;
    void SetLogCallback(LogCallback cb) { log_cb_ = std::move(cb); }

private:
    // --- Pipeline stages ---
    bool PreBuildChecks(Project& project, BuildResult& result);
    void EnsureDefaultFonts(Project& project);
    bool GenerateAutoHeader(Project& project, std::string& out_header);
    bool CompileAllScripts(Project& project, const BuildConfig& config, BuildResult& result);
    bool WriteGameDataFile(Project& project, const BuildConfig& config,
                           BuildResult& result, const std::string& output_path);
    bool PackageAssets(Project& project, const BuildConfig& config,
                       BuildResult& result, const std::string& data_dir);
    bool CreateVOXFiles(Project& project, const BuildConfig& config,
                        BuildResult& result, const std::string& data_dir);
    bool CopyEngineFiles(const BuildConfig& config, BuildTarget target,
                         BuildResult& result, const std::string& target_dir);
    bool GenerateConfigFile(Project& project, const BuildConfig& config,
                            const std::string& target_dir);
    bool CreateBuildDirectories(const BuildConfig& config, BuildResult& result);

    // --- Helper methods ---
    void Log(const char* fmt, ...);
    std::string GetCompiledDir(const BuildConfig& config) const;
    std::string GetDataDir(const BuildConfig& config) const;
    std::string GetTargetDir(const BuildConfig& config, BuildTarget target) const;
    std::string FindEngineExecutable(const BuildConfig& config) const;

    // --- State ---
    bool building_ = false;
    bool debug_run_active_ = false;  // true if current run is a debug session
    int game_pid_ = 0;      // PID of running game process (0 = not running)
    int game_output_fd_ = -1;  // File descriptor for game stdout+stderr pipe (Unix)
    BuildProgress progress_;
    BuildResult last_result_;
    BuildConfig config_;
    LogCallback log_cb_;
    DebugController debug_controller_;

    // Compiled script data for packaging
    std::vector<CompiledScriptInfo> compiled_scripts_;

    // Cached game title for platform builds (set from BuildGame)
    std::string build_game_title_;

public:
    // Poll game process output (call periodically from UI loop)
    void PollGameOutput();
};

} // namespace AGSEditor
