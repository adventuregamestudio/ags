//=============================================================================
// agsassets - Package Compiled/Data/<GameFileName>.ags
//=============================================================================
#include "build_system.h"
#include "cli_utils.h"

#include <chrono>

using namespace AGSBuild;

namespace {

const char *HELP_STRING = R"EOS(agsassets - Package game data into a .ags archive
Copyright (c) AGS Team and contributors

Usage: agsassets --open <Game.agf> [options]

Creates Compiled/Data/<GameFileName>.ags containing game28.dta, rooms, sprites,
fonts, and other runtime assets. Expects game28.dta at <output>/_temp/game28.dta.

Options:
  --open <path>           Path to Game.agf (required)
  --project-dir <dir>     Project root (default: directory containing Game.agf)
  --output <dir>          Build output root (default: <project>/Compiled)
  --verbose               Print build log to stderr
  -h, --help              Show this help message

Typical pipeline step (after agdta):
  agsassets --open Game.agf --output Compiled
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
            if (!build.ValidateProject(project, result))
                return false;
            if (!build.PackageDataAssets(project, config, result, build.GetDataDir(config)))
                return false;

            result.success = (result.ErrorCount() == 0);
            result.elapsed_seconds =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            return true;
        });
}
