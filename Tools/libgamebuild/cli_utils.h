// Shared CLI helpers for standalone libgamebuild tools.
#pragma once

#include "build_system.h"
#include "path_utils.h"
#include "project.h"

#include "util/cmdlineopts.h"

#include <cstdio>
#include <functional>
#include <set>
#include <string>

namespace AGSBuild
{

struct ProjectCliOptions
{
    std::string agf_path;
    std::string project_dir;
    std::string output_dir;
    std::string scripts_dir;
    bool force = false;
    bool verbose = false;
    bool help = false;
    bool error = false;
};

inline ProjectCliOptions ParseProjectCli(int argc, char *argv[], const char *help_string,
    const std::set<AGS::Common::String> &extra_with_value = {})
{
    ProjectCliOptions opts;
    std::set<AGS::Common::String> with_value = {"--open", "--project-dir", "--output", "--scripts-dir"};
    with_value.insert(extra_with_value.begin(), extra_with_value.end());

    AGS::Common::CmdLineOpts::ParseResult pr = AGS::Common::CmdLineOpts::Parse(argc, argv, with_value);
    if (pr.HelpRequested)
    {
        opts.help = true;
        return opts;
    }

    auto get_val = [&](const char *name) -> std::string {
        for (const auto &owv : pr.OptWithValue)
        {
            if (owv.first == name)
                return owv.second.GetCStr();
        }
        return {};
    };

    opts.agf_path = get_val("--open");
    if (opts.agf_path.empty() && !pr.PosArgs.empty())
        opts.agf_path = pr.PosArgs[0].GetCStr();

    opts.project_dir = get_val("--project-dir");
    opts.output_dir = get_val("--output");
    opts.scripts_dir = get_val("--scripts-dir");
    opts.force = pr.Opt.count("--force") > 0;
    opts.verbose = pr.Opt.count("--verbose") > 0 || pr.Opt.count("-v") > 0;

    if (opts.agf_path.empty())
    {
        opts.error = true;
        (void)help_string;
    }
    return opts;
}

inline bool OpenProjectFromCli(Project &project, const ProjectCliOptions &opts)
{
    if (!project.OpenProject(opts.agf_path))
        return false;

    if (!opts.project_dir.empty())
        project.SetProjectDir(ResolveToAbsolutePath(opts.project_dir));
    return true;
}

inline BuildConfig MakeDataBuildConfig(Project &project, const ProjectCliOptions &opts)
{
    BuildConfig config;
    config.SetDefaults(project.GetProjectDir());
    config.debug_mode = false;
    config.data_only = true;
    config.force_rebuild = opts.force;
    config.targets[(int)BuildTarget::DataFile] = true;

    if (!opts.output_dir.empty())
        config.output_base_dir = ResolveToAbsolutePath(opts.output_dir);

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

inline int RunCliMain(int argc, char *argv[], const char *help_string,
    const std::function<bool(Project &, BuildSystem &, const ProjectCliOptions &, BuildResult &)> &run)
{
    ProjectCliOptions opts = ParseProjectCli(argc, argv, help_string);
    if (opts.help)
    {
        printf("%s", help_string);
        return 0;
    }
    if (opts.error)
    {
        fprintf(stderr, "Error: --open <Game.agf> is required\n\n");
        printf("%s", help_string);
        return 1;
    }

    Project project;
    if (!OpenProjectFromCli(project, opts))
    {
        fprintf(stderr, "Error: failed to open project: %s\n", opts.agf_path.c_str());
        return 1;
    }

    BuildSystem build_system;
    if (opts.verbose)
    {
        build_system.SetLogCallback([](const std::string &msg) {
            fprintf(stderr, "%s\n", msg.c_str());
        });
    }

    BuildResult result;
    const bool ok = run(project, build_system, opts, result);

    PrintBuildMessages(result, opts.verbose);
    fprintf(stderr, "%s in %.2fs (%d error(s), %d warning(s))\n",
            ok && result.success ? "Done" : "FAILED", result.elapsed_seconds,
            result.ErrorCount(), result.WarningCount());

    return ok ? ExitCodeFromResult(result) : 1;
}

} // namespace AGSBuild
