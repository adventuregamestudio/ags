//=============================================================================
//
// Adventure Game Studio (AGS)
//
// agsbuild - Native headless game compiler (no Windows C# Editor required)
//
//=============================================================================
#include "build_system.h"
#include "path_utils.h"
#include "project.h"

#include "util/cmdlineopts.h"

#include <cstdio>
#include <cstring>
#include <set>
#include <string>

using namespace AGS::Common;
using namespace AGS::Common::CmdLineOpts;
using namespace AGSBuild;

namespace {

const char *HELP_STRING = R"EOS(agsbuild - Convenience wrapper for the data build pipeline
Copyright (c) AGS Team and contributors

Usage: agsbuild --open <Game.agf> [options]

Runs the full Data-target pipeline in one invocation. For composable builds use
the standalone tools instead (see Tools/scripts/build-data.sh):

  agsscripts   compile project scripts to Compiled/_temp/*.o
  agdta        write game28.dta from Game.agf + script objects
  agsvox       build audio.vox / speech.vox
  agstrans     compile registered translations to Compiled/Data/*.tra
  agsassets    package Compiled/Data/<GameFileName>.ags
  agscfg       write Compiled/Data/acsetup.cfg

Options:
  --open <path>           Path to Game.agf (required)
  --project-dir <dir>     Project root (default: directory containing Game.agf)
  --output <dir>          Output directory (default: <project>/Compiled)
  --force                 Rebuild all scripts even if timestamps unchanged
  --verbose               Print build log to stderr
  -h, --help              Show this help message
)EOS";

struct ParsedOptions {
    std::string agf_path;
    std::string project_dir;
    std::string output_dir;
    bool force = false;
    bool verbose = false;
    bool help = false;
    bool error = false;
};

ParsedOptions ParseOptions(int argc, char *argv[])
{
    ParsedOptions opts;
    std::set<String> with_value = {"--open", "--project-dir", "--output"};
    ParseResult pr = Parse(argc, argv, with_value);

    if (pr.HelpRequested) {
        opts.help = true;
        return opts;
    }

    auto get_val = [&](const char *name) -> std::string {
        for (const auto &owv : pr.OptWithValue) {
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
    opts.force = pr.Opt.count("--force") > 0;
    opts.verbose = pr.Opt.count("--verbose") > 0 || pr.Opt.count("-v") > 0;

    if (opts.agf_path.empty()) {
        opts.error = true;
    }
    return opts;
}

} // namespace

int main(int argc, char *argv[])
{
    ParsedOptions opts = ParseOptions(argc, argv);

    if (opts.help) {
        printf("%s", HELP_STRING);
        return 0;
    }
    if (opts.error) {
        fprintf(stderr, "Error: --open <Game.agf> is required\n\n");
        printf("%s", HELP_STRING);
        return 1;
    }

    AGSBuild::Project project;
    if (!project.OpenProject(opts.agf_path)) {
        fprintf(stderr, "Error: failed to open project: %s\n", opts.agf_path.c_str());
        return 1;
    }

    if (!opts.project_dir.empty())
        project.SetProjectDir(ResolveToAbsolutePath(opts.project_dir));

    AGSBuild::BuildSystem build_system;
    if (opts.verbose) {
        build_system.SetLogCallback([](const std::string &msg) {
            fprintf(stderr, "%s\n", msg.c_str());
        });
    }

    AGSBuild::BuildConfig config;
    config.SetDefaults(project.GetProjectDir());
    config.debug_mode = false;
    config.data_only = true;
    config.force_rebuild = opts.force;

    if (!opts.output_dir.empty())
        config.output_base_dir = ResolveToAbsolutePath(opts.output_dir);

    // DataFile target only — no platform engine deployment
    for (int i = 0; i < (int)AGSBuild::BuildTarget::Count; ++i) {
        config.targets[i] = (i == (int)AGSBuild::BuildTarget::DataFile);
    }

    AGSBuild::BuildResult result = build_system.BuildGame(project, config);

    if (opts.verbose || !result.success) {
        for (const auto &msg : result.messages) {
            const char *level = "info";
            if (msg.type == AGSBuild::BuildMessageType::Error)
                level = "error";
            else if (msg.type == AGSBuild::BuildMessageType::Warning)
                level = "warning";

            if (!msg.file.empty())
                fprintf(stderr, "[%s] %s(%d): %s\n", level, msg.file.c_str(), msg.line,
                        msg.message.c_str());
            else
                fprintf(stderr, "[%s] %s\n", level, msg.message.c_str());
        }
    }

    fprintf(stderr, "Build %s in %.2fs (%d error(s), %d warning(s))\n",
            result.success ? "succeeded" : "FAILED", result.elapsed_seconds,
            result.ErrorCount(), result.WarningCount());

    if (!result.success)
        return 1;
    if (result.WarningCount() > 0)
        return 2;
    return 0;
}
