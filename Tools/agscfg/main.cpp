//=============================================================================
// agscfg - Write acsetup.cfg for a compiled game
//=============================================================================
#include "build_system.h"
#include "cli_utils.h"

#include <chrono>

using namespace AGSBuild;

namespace {

const char *HELP_STRING = R"EOS(agscfg - Generate acsetup.cfg
Copyright (c) AGS Team and contributors

Usage: agscfg --open <Game.agf> [options]

Writes Compiled/Data/acsetup.cfg from Game.agf settings.

Options:
  --open <path>           Path to Game.agf (required)
  --project-dir <dir>     Project root (default: directory containing Game.agf)
  --output <dir>          Build output root (default: <project>/Compiled)
  --verbose               Print build log to stderr
  -h, --help              Show this help message
)EOS";

} // namespace

int main(int argc, char *argv[])
{
    return RunCliMain(argc, argv, HELP_STRING,
        [](Project &project, BuildSystem &build, const ProjectCliOptions &opts, BuildResult &result) {
            const auto start = std::chrono::steady_clock::now();
            BuildConfig config = MakeDataBuildConfig(project, opts);
            build.SetConfig(config);

            if (!build.PrepareOutputDirectories(config, result))
                return false;
            if (!build.WriteSetupConfig(project, config, build.GetDataDir(config)))
            {
                result.AddError("", 0, "Failed to write acsetup.cfg");
                return false;
            }

            result.success = true;
            result.elapsed_seconds =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            return true;
        });
}
