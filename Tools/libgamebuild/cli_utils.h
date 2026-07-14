// Shared CLI helpers for agdta.
#pragma once

#include "build_system.h"
#include "project.h"

#include "util/path_helpers.h"

#include <cstdio>
#include <string>

namespace AGSBuild
{

struct ProjectCliOptions
{
    std::string agf_path;
    std::string project_dir;
    std::string output_dir;
    bool verbose = false;
    bool help = false;
    bool error = false;
};

inline bool OpenProjectFromCli(Project &project, const ProjectCliOptions &opts)
{
    if (!project.OpenProject(opts.agf_path))
        return false;

    if (!opts.project_dir.empty())
        project.SetProjectDir(AGS::Common::ResolveToAbsolutePath(opts.project_dir));
    return true;
}

inline BuildConfig MakeDataBuildConfig(Project &project, const ProjectCliOptions &opts)
{
    BuildConfig config;
    config.SetDefaults(project.GetProjectDir());

    if (!opts.output_dir.empty())
        config.output_base_dir = AGS::Common::ResolveToAbsolutePath(opts.output_dir);

    return config;
}

inline void PrintBuildMessages(const BuildResult &result, bool verbose)
{
    if (!verbose && result.success)
        return;

    for (const auto &msg : result.messages)
    {
        const char *level = "info";
        if (msg.type == BuildMessageType::Error)
            level = "error";
        else if (msg.type == BuildMessageType::Warning)
            level = "warning";

        if (!msg.file.empty())
            fprintf(stderr, "[%s] %s(%d): %s\n", level, msg.file.c_str(), msg.line,
                    msg.message.c_str());
        else
            fprintf(stderr, "[%s] %s\n", level, msg.message.c_str());
    }
}

inline int ExitCodeFromResult(const BuildResult &result)
{
    if (!result.success)
        return 1;
    if (result.WarningCount() > 0)
        return 2;
    return 0;
}

} // namespace AGSBuild
