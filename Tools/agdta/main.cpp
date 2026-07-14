//=============================================================================
// agdta - Write game28.dta from Game.agf and compiled script objects
//=============================================================================
#include "build_system.h"
#include "cli_utils.h"
#include "io_utils.h"
#include "path_utils.h"

#include <chrono>

using namespace AGSBuild;

namespace {

const char *HELP_STRING = R"EOS(agdta - Write game28.dta (compiled game data)
Copyright (c) AGS Team and contributors

Usage: agdta --open <Game.agf> [options]

Parses Game.agf and writes the engine game data file (game28.dta) using compiled
script object files produced by agsscripts (or the editor).

Options:
  --open <path>           Path to Game.agf (required)
  --project-dir <dir>     Project root (default: directory containing Game.agf)
  --output <dir>          Build output root (default: <project>/Compiled)
  --scripts-dir <dir>     Directory with *.o files (default: <output>/_temp)
  --dta <path>            Output .dta path (default: <output>/_temp/game28.dta)
  --verbose               Print build log to stderr
  -h, --help              Show this help message

Typical pipeline step:
  agsscripts --open Game.agf --output Compiled
  agdta --open Game.agf --output Compiled
)EOS";

struct AgdtaOptions : ProjectCliOptions
{
    std::string dta_path;
};

AgdtaOptions ParseAgdtaCli(int argc, char *argv[])
{
    AgdtaOptions opts;
    std::set<AGS::Common::String> extra = {"--dta"};
    std::set<AGS::Common::String> with_value = {
        "--open", "--project-dir", "--output", "--scripts-dir", "--dta"};
    AGS::Common::CmdLineOpts::ParseResult pr =
        AGS::Common::CmdLineOpts::Parse(argc, argv, with_value);

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
    opts.dta_path = get_val("--dta");
    opts.verbose = pr.Opt.count("--verbose") > 0 || pr.Opt.count("-v") > 0;
    if (opts.agf_path.empty())
        opts.error = true;
    return opts;
}

} // namespace

int main(int argc, char *argv[])
{
    AgdtaOptions opts = ParseAgdtaCli(argc, argv);
    if (opts.help)
    {
        printf("%s", HELP_STRING);
        return 0;
    }
    if (opts.error)
    {
        fprintf(stderr, "Error: --open <Game.agf> is required\n\n");
        printf("%s", HELP_STRING);
        return 1;
    }

    Project project;
    if (!OpenProjectFromCli(project, opts))
    {
        fprintf(stderr, "Error: failed to open project: %s\n", opts.agf_path.c_str());
        return 1;
    }

    BuildSystem build;
    if (opts.verbose)
    {
        build.SetLogCallback([](const std::string &msg) {
            fprintf(stderr, "%s\n", msg.c_str());
        });
    }

    const auto start = std::chrono::steady_clock::now();
    BuildConfig config = MakeDataBuildConfig(project, opts);
    build.SetConfig(config);

    BuildResult result;
    if (!build.PrepareOutputDirectories(config, result))
    {
        PrintBuildMessages(result, opts.verbose);
        return 1;
    }
    if (!build.ValidateProject(project, result))
    {
        PrintBuildMessages(result, opts.verbose);
        return 1;
    }

    std::string scripts_dir = opts.scripts_dir;
    if (scripts_dir.empty())
        scripts_dir = build.GetCompiledDir(config) + "/_temp";
    scripts_dir = ResolveToAbsolutePath(scripts_dir);

    if (!build.LoadCompiledScriptsFromDir(scripts_dir, result))
    {
        PrintBuildMessages(result, true);
        return 1;
    }

    std::string dta_path = opts.dta_path;
    if (dta_path.empty())
        dta_path = build.GetCompiledDir(config) + "/_temp/" + kCompiledDtaFileName;
    dta_path = ResolveToAbsolutePath(dta_path);

    const size_t slash = dta_path.rfind('/');
    if (slash != std::string::npos)
        EnsureDirectory(dta_path.substr(0, slash));

    const bool ok = build.WriteGameData(project, config, result, dta_path);
    result.success = ok && (result.ErrorCount() == 0);
    result.elapsed_seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    PrintBuildMessages(result, opts.verbose);
    fprintf(stderr, "%s in %.2fs (%d error(s), %d warning(s))\n",
            result.success ? "Done" : "FAILED", result.elapsed_seconds,
            result.ErrorCount(), result.WarningCount());
    return ExitCodeFromResult(result);
}
