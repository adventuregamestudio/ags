//=============================================================================
// agstrans - Compile all translations registered in Game.agf
//=============================================================================
#include "build_system.h"
#include "cli_utils.h"

#include <chrono>

using namespace AGSBuild;

namespace {

const char *HELP_STRING = R"EOS(agstrans - Compile project translations (.TRS -> .TRA)
Copyright (c) AGS Team and contributors

Usage: agstrans --open <Game.agf> [options]

Compiles every translation listed in Game.agf into Compiled/Data/*.tra.
For a single file use the existing trac tool instead.

Options:
  --open <path>           Path to Game.agf (required)
  --project-dir <dir>     Project root (default: directory containing Game.agf)
  --output <dir>          Build output root (default: <project>/Compiled)
  --verbose               Print build log to stderr
  -h, --help              Show this help message

See also: trac (single .TRS file)
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
            if (!build.CompileProjectTranslations(project, config, result, build.GetDataDir(config)))
                return false;

            result.success = (result.ErrorCount() == 0);
            result.elapsed_seconds =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            return true;
        });
}
