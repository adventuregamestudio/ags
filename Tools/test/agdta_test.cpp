#include "agdta_test_compile.h"
#include "project.h"

#include "game/main_game_file.h"
#include "ac/gamesetupstruct.h"
#include "util/file.h"
#include "util/filestream.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <unistd.h>

using namespace AGS::Common;
using namespace AGSBuild;

namespace {

std::string TempBuildDir()
{
    const auto dir = std::filesystem::temp_directory_path() /
        ("agdta_test_" + std::to_string(getpid()));
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir, ec);
    return dir.string();
}

bool CopyFixtureProject(const std::filesystem::path &dst)
{
    std::error_code ec;
    std::filesystem::create_directories(dst, ec);
    const std::filesystem::path src(AGDTA_FIXTURE_DIR);
    for (const auto &entry : std::filesystem::directory_iterator(src, ec))
    {
        if (!entry.is_regular_file())
            continue;
        const std::string name = entry.path().filename().string();
        if (!name.empty() && name[0] == '.')
            continue;
        std::filesystem::copy_file(entry.path(), dst / entry.path().filename(),
            std::filesystem::copy_options::overwrite_existing, ec);
        if (ec)
            return false;
    }
    return true;
}

} // namespace

TEST(AgdtaTest, WritesGame28DtaReadableByEngine)
{
    const std::filesystem::path work = TempBuildDir();
    ASSERT_TRUE(CopyFixtureProject(work));

    Project project;
    ASSERT_TRUE(project.OpenProject((work / "Game.agf").string()));

    BuildConfig config;
    config.SetDefaults(project.GetProjectDir());
    config.output_base_dir = (work / "Compiled").string();

    BuildSystem build;
    BuildResult result;

    ASSERT_TRUE(build.PrepareOutputDirectories(config, result));
    ASSERT_TRUE(build.ValidateProject(project, result));
    ASSERT_TRUE(CompileFixtureScripts(project, config, result))
        << ([&]() {
            std::string msg;
            for (const auto &m : result.messages)
                if (m.type == BuildMessageType::Error)
                    msg += m.message + "\n";
            return msg;
        })();

    const std::string scripts_dir = config.output_base_dir + "/_temp";
    ASSERT_TRUE(build.LoadCompiledScriptsFromDir(scripts_dir, result));

    const std::string dta_path = scripts_dir + "/" + std::string(kCompiledDtaFileName);
    ASSERT_TRUE(build.WriteGameData(project, config, result, dta_path));
    ASSERT_TRUE(std::filesystem::exists(dta_path));

    MainGameSource src;
    HGameFileError open_err = OpenMainGameFile(dta_path.c_str(), src);
    ASSERT_TRUE(open_err)
        << (open_err.HasError() ? open_err->FullMessage().GetCStr() : "unknown error");

    GameSetupStruct game;
    LoadedGameEntities ents(game);
    HGameFileError read_err = ReadGameData(ents, std::move(src.InputStream),
        src.DataVersion, src.CompiledWith);
    ASSERT_TRUE(read_err)
        << (read_err.HasError() ? read_err->FullMessage().GetCStr() : "unknown error");
}
