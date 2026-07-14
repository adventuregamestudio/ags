//=============================================================================
// agsscripts - Compile all script modules listed in Game.agf
//=============================================================================
#include "build_system.h"
#include "cli_utils.h"

#include <chrono>

using namespace AGSBuild;

namespace {

const char *HELP_STRING = R"EOS(agsscripts - Compile AGS project scripts
Copyright (c) AGS Team and contributors

Usage: agsscripts --open <Game.agf> [options]

Compiles GlobalScript, dialog scripts, _GlobalVariables, and every script module
from the project into Compiled/_temp/*.o object files.

Options:
  --open <path>           Path to Game.agf (required)
  --project-dir <dir>     Project root (default: directory containing Game.agf)
  --output <dir>          Build output root (default: <project>/Compiled)
  --force                 Rebuild even if timestamps unchanged
  --verbose               Print build log to stderr
  -h, --help              Show this help message

See also: agdta, agsassets, agsvox, agstrans, agscfg, trac, agspak, spritepak
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
            if (!build.CompileProjectScripts(project, config, result))
                return false;

            result.success = (result.ErrorCount() == 0);
            result.elapsed_seconds =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            return true;
        });
}
