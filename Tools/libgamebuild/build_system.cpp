// AGS Build System implementation for agdta (game28.dta writer).

#include "build_system.h"
#include "game_data_writer.h"
#include "project.h"
#include "game_data.h"

#include "util/file_helpers.h"
#include "util/path_helpers.h"

#include <cstdarg>
#include <cstdio>

namespace AGSBuild
{

using AGS::Common::CollectDirectoryFileNames;
using AGS::Common::EnsureDirectory;

void BuildConfig::SetDefaults(const std::string& project_dir)
{
    output_base_dir = project_dir + "/Compiled";
}

int BuildResult::ErrorCount() const
{
    int count = 0;
    for (const auto& m : messages)
        if (m.type == BuildMessageType::Error) count++;
    return count;
}

int BuildResult::WarningCount() const
{
    int count = 0;
    for (const auto& m : messages)
        if (m.type == BuildMessageType::Warning) count++;
    return count;
}

void BuildResult::AddError(const std::string& file, int line, const std::string& msg,
                            const std::string& module)
{
    messages.push_back({BuildMessageType::Error, file, line, 0, msg, module});
}

void BuildResult::AddWarning(const std::string& file, int line, const std::string& msg,
                              const std::string& module)
{
    messages.push_back({BuildMessageType::Warning, file, line, 0, msg, module});
}

void BuildResult::AddInfo(const std::string& msg)
{
    messages.push_back({BuildMessageType::Info, "", 0, 0, msg, ""});
}

void BuildSystem::Log(const char* fmt, ...)
{
    if (!log_cb_) return;
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    log_cb_(std::string(buf));
}

std::string BuildSystem::GetCompiledDir(const BuildConfig& config) const
{
    return config.output_base_dir;
}

bool BuildSystem::CreateBuildDirectories(const BuildConfig& config, BuildResult& result)
{
    (void)result;
    const std::string base = GetCompiledDir(config);
    EnsureDirectory(base);
    EnsureDirectory(base + "/_temp");
    return true;
}

bool BuildSystem::PreBuildChecks(Project& project, BuildResult& result)
{
    if (!project.IsLoaded())
    {
        result.AddError("", 0, "No project is loaded.");
        return false;
    }

    GameData* gd = project.GetGameData();
    if (!gd)
    {
        result.AddError("", 0, "Game data is null.");
        return false;
    }

    if (gd->script_modules.empty())
    {
        result.AddError("", 0, "No script modules found in the project.");
        return false;
    }

    return true;
}

bool BuildSystem::WriteGameDataFile(Project& project, const BuildConfig& config,
                                     BuildResult& result, const std::string& output_path)
{
    (void)config;
    GameData* gd = project.GetGameData();
    if (!gd) return false;

    Log("[Build] Writing game data: %s", output_path.c_str());

    std::vector<AGSBuild::CompiledScriptRef> scripts;

    const CompiledScriptInfo* global_script = nullptr;
    const CompiledScriptInfo* dialog_script = nullptr;
    const CompiledScriptInfo* global_vars = nullptr;
    std::vector<const CompiledScriptInfo*> modules;

    for (const auto& cs : compiled_scripts_) {
        if (cs.name == "GlobalScript")
            global_script = &cs;
        else if (cs.name == "__DialogScripts")
            dialog_script = &cs;
        else if (cs.name == "_GlobalVariables")
            global_vars = &cs;
        else
            modules.push_back(&cs);
    }

    if (global_script)
        scripts.push_back({global_script->name, global_script->obj_file});
    if (dialog_script)
        scripts.push_back({dialog_script->name, dialog_script->obj_file});
    if (global_vars)
        scripts.push_back({global_vars->name, global_vars->obj_file});
    for (const auto* m : modules)
        scripts.push_back({m->name, m->obj_file});

    if (!AGSBuild::WriteGameDataFile(*gd, scripts, output_path, result)) {
        return false;
    }

    Log("[Build] Game data written successfully.");
    return true;
}

bool BuildSystem::PrepareOutputDirectories(const BuildConfig& config, BuildResult& result)
{
    return CreateBuildDirectories(config, result);
}

bool BuildSystem::ValidateProject(Project& project, BuildResult& result)
{
    return PreBuildChecks(project, result);
}

bool BuildSystem::LoadCompiledScriptsFromDir(const std::string& dir, BuildResult& result)
{
    compiled_scripts_.clear();

    std::vector<std::string> names;
    CollectDirectoryFileNames(dir, [](const std::string& fname) {
        return fname.size() > 2 && fname.compare(fname.size() - 2, 2, ".o") == 0;
    }, names);

    if (names.empty())
    {
        result.AddError("", 0, "No compiled script objects (.o) found in: " + dir);
        return false;
    }

    for (const auto& fname : names)
    {
        CompiledScriptInfo info;
        info.name = fname.substr(0, fname.size() - 2);
        info.obj_file = dir + "/" + fname;
        compiled_scripts_.push_back(std::move(info));
    }

    return true;
}

bool BuildSystem::WriteGameData(Project& project, const BuildConfig& config,
                                BuildResult& result, const std::string& output_path)
{
    return WriteGameDataFile(project, config, result, output_path);
}

} // namespace AGSBuild
