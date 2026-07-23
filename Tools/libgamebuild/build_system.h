// AGS - Build helpers for agdta (game28.dta writer)
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace AGSBuild
{

class Project;
struct GameData;

struct BuildConfig
{
    std::string output_base_dir;

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

struct BuildResult
{
    bool success = false;
    std::vector<BuildMessage> messages;
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
};

class BuildSystem
{
public:
    BuildSystem() = default;
    ~BuildSystem() = default;

    bool PrepareOutputDirectories(const BuildConfig& config, BuildResult& result);
    bool ValidateProject(Project& project, BuildResult& result);
    bool LoadCompiledScriptsFromDir(const std::string& dir, BuildResult& result);
    bool WriteGameData(Project& project, const BuildConfig& config,
                       BuildResult& result, const std::string& output_path);

    using LogCallback = std::function<void(const std::string&)>;
    void SetLogCallback(LogCallback cb) { log_cb_ = std::move(cb); }

    std::string GetCompiledDir(const BuildConfig& config) const;

private:
    void Log(const char* fmt, ...);
    bool CreateBuildDirectories(const BuildConfig& config, BuildResult& result);
    bool PreBuildChecks(Project& project, BuildResult& result);
    bool WriteGameDataFile(Project& project, const BuildConfig& config,
                           BuildResult& result, const std::string& output_path);

    LogCallback log_cb_;
    std::vector<CompiledScriptInfo> compiled_scripts_;
};

} // namespace AGSBuild
