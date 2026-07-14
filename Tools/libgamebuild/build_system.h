// AGS - Build System
// Headless game build pipeline: script compilation, data file generation, asset packaging.
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace AGSBuild
{

class Project;
struct GameData;

enum class BuildTarget
{
    DataFile,
    Linux,
    Windows,
    MacOS,
    Web,
    Count
};

const char* BuildTargetName(BuildTarget target);
const char* BuildTargetDirName(BuildTarget target);

struct BuildConfig
{
    bool debug_mode = true;
    bool line_numbers = true;

    bool targets[(int)BuildTarget::Count] = {
        true,   // DataFile
        true,   // Linux
        false,  // Windows
        false,  // macOS
        false,  // Web
    };

    std::string output_base_dir;
    std::string engine_linux_path;
    std::string engine_windows_path;
    std::string engine_macos_path;
    std::string engine_web_path;

    std::string script_api_version = "Highest";
    std::string script_compat_level = "Highest";
    int split_size_mb = 0;

    // agsbuild: skip copying engine binaries to platform folders
    bool data_only = false;
    bool force_rebuild = false;

    void SetDefaults(const std::string& project_dir);
};

inline constexpr const char *kCompiledDtaFileName = "game28.dta";

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
    std::string module_name;
};

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

struct CompiledScriptInfo
{
    std::string name;
    std::string obj_file;
    bool is_header = false;
};

class BuildSystem
{
public:
    BuildSystem() = default;
    ~BuildSystem() = default;

    BuildResult BuildGame(Project& project, const BuildConfig& config);

    // Standalone pipeline steps (each maps to one CLI tool).
    bool PrepareOutputDirectories(const BuildConfig& config, BuildResult& result);
    bool ValidateProject(Project& project, BuildResult& result);
    bool CompileProjectScripts(Project& project, const BuildConfig& config, BuildResult& result);
    bool LoadCompiledScriptsFromDir(const std::string& dir, BuildResult& result);
    bool WriteGameData(Project& project, const BuildConfig& config,
                       BuildResult& result, const std::string& output_path);
    bool PackageDataAssets(Project& project, const BuildConfig& config,
                           BuildResult& result, const std::string& data_dir);
    bool BuildVoxFiles(Project& project, const BuildConfig& config,
                       BuildResult& result, const std::string& data_dir);
    bool CompileProjectTranslations(Project& project, const BuildConfig& config,
                                    BuildResult& result, const std::string& data_dir);
    bool WriteSetupConfig(Project& project, const BuildConfig& config,
                          const std::string& target_dir);

    bool IsBuilding() const { return building_; }
    const BuildProgress& GetProgress() const { return progress_; }
    const BuildResult& GetLastResult() const { return last_result_; }
    const BuildConfig& GetConfig() const { return config_; }

    void SetConfig(const BuildConfig& config) { config_ = config; }
    BuildConfig& GetConfigMut() { return config_; }

    using LogCallback = std::function<void(const std::string&)>;
    void SetLogCallback(LogCallback cb) { log_cb_ = std::move(cb); }

    std::string GetCompiledDir(const BuildConfig& config) const;
    std::string GetDataDir(const BuildConfig& config) const;

private:
    void EnsureDefaultFonts(Project& project);
    bool GenerateAutoHeader(Project& project, std::string& out_header);
    bool PreBuildChecks(Project& project, BuildResult& result);
    bool CompileAllScripts(Project& project, const BuildConfig& config, BuildResult& result);
    bool WriteGameDataFile(Project& project, const BuildConfig& config,
                           BuildResult& result, const std::string& output_path);
    bool PackageAssets(Project& project, const BuildConfig& config,
                       BuildResult& result, const std::string& data_dir);
    bool CreateVOXFiles(Project& project, const BuildConfig& config,
                        BuildResult& result, const std::string& data_dir);
    bool CompileRegisteredTranslations(Project& project, const BuildConfig& config,
                                       BuildResult& result, const std::string& data_dir);
    bool GenerateConfigFile(Project& project, const BuildConfig& config,
                            const std::string& target_dir);
    bool CreateBuildDirectories(const BuildConfig& config, BuildResult& result);
    bool CopyEngineFiles(const BuildConfig& config, BuildTarget target,
                         BuildResult& result, const std::string& target_dir);

    void Log(const char* fmt, ...);
    std::string GetTargetDir(const BuildConfig& config, BuildTarget target) const;

    bool building_ = false;
    BuildProgress progress_;
    BuildResult last_result_;
    BuildConfig config_;
    LogCallback log_cb_;
    std::vector<CompiledScriptInfo> compiled_scripts_;
    std::string build_game_title_;
};

} // namespace AGSBuild
