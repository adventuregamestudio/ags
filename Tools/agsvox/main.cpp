//=============================================================================
// agsvox - Build audio.vox and speech.vox for a project
//=============================================================================
#include "build_system.h"
#include "cli_utils.h"

#include <chrono>

using namespace AGSBuild;

namespace {

const char *HELP_STRING = R"EOS(agsvox - Build audio.vox and speech.vox
Copyright (c) AGS Team and contributors

Usage: agsvox --open <Game.agf> [options]

Packages registered music/audio clips and Speech/ voice files into Compiled/Data
audio.vox and speech.vox (and language-specific sp_*.vox when present).

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
            if (!build.ValidateProject(project, result))
                return false;
            if (!build.BuildVoxFiles(project, config, result, build.GetDataDir(config)))
                return false;

            result.success = (result.ErrorCount() == 0);
            result.elapsed_seconds =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            return true;
        });
}
