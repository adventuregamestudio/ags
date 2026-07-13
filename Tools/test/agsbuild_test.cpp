#include "build_system.h"
#include "project.h"
#include "game_data.h"

#include "data/multifilelib.h"
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
        ("agsbuild_test_" + std::to_string(getpid()));
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir, ec);
    return dir.string();
}

bool CopyFixtureProject(const std::filesystem::path &dst)
{
    std::error_code ec;
    std::filesystem::create_directories(dst, ec);
    const std::filesystem::path src(AGSBUILD_FIXTURE_DIR);
    for (const auto &entry : std::filesystem::directory_iterator(src, ec))
    {
        if (!entry.is_regular_file())
            continue;
        std::filesystem::copy_file(entry.path(), dst / entry.path().filename(),
            std::filesystem::copy_options::overwrite_existing, ec);
        if (ec)
            return false;
    }
    return true;
}

bool ArchiveContainsAsset(const std::string &ags_path, const char *asset_name)
{
    auto in = std::unique_ptr<Stream>(File::OpenFileRead(ags_path.c_str()));
    if (!in)
        return false;

    AssetLibInfo lib;
    if (MFLUtil::ReadHeader(lib, in.get()) != MFLUtil::kMFLNoError)
        return false;

    for (const auto &ai : lib.AssetInfos)
    {
        if (ai.FileName.Compare(asset_name) == 0)
            return true;
    }
    return false;
}

} // namespace

TEST(AgsBuildTest, BuildsMinimalFixtureAndPackagesGame28Dta)
{
    const std::filesystem::path work = TempBuildDir();
    ASSERT_TRUE(CopyFixtureProject(work));

    Project project;
    const std::string agf = (work / "Game.agf").string();
    ASSERT_TRUE(project.OpenProject(agf));

    BuildConfig config;
    config.SetDefaults(project.GetProjectDir());
    config.data_only = true;
    config.debug_mode = false;
    config.output_base_dir = (work / "Compiled").string();
    for (int i = 0; i < (int)BuildTarget::Count; ++i)
        config.targets[i] = (i == (int)BuildTarget::DataFile);

    BuildSystem build;
    BuildResult result = build.BuildGame(project, config);

    ASSERT_TRUE(result.success) << "Build failed with " << result.ErrorCount() << " error(s)";

    GameData *gd = project.GetGameData();
    ASSERT_NE(gd, nullptr);
    const std::string base_name =
        !gd->game_file_name.empty() ? gd->game_file_name : work.filename().string();
    const std::string ags_path = config.output_base_dir + "/Data/" + base_name + ".ags";
    const std::string cfg_path = config.output_base_dir + "/Data/acsetup.cfg";

    EXPECT_TRUE(std::filesystem::exists(ags_path));
    EXPECT_TRUE(std::filesystem::exists(cfg_path));
    EXPECT_TRUE(ArchiveContainsAsset(ags_path, kCompiledDtaFileName));

    const std::string temp_dta =
        config.output_base_dir + "/_temp/" + std::string(kCompiledDtaFileName);
    EXPECT_FALSE(std::filesystem::exists(temp_dta))
        << "Temporary DTA should be removed after packaging";
}

TEST(AgsBuildTest, GameDataFileIsReadableByEngine)
{
    const std::filesystem::path work = TempBuildDir();
    ASSERT_TRUE(CopyFixtureProject(work));

    Project project;
    ASSERT_TRUE(project.OpenProject((work / "Game.agf").string()));

    BuildConfig config;
    config.SetDefaults(project.GetProjectDir());
    config.data_only = true;
    config.debug_mode = false;
    config.output_base_dir = (work / "Compiled").string();
    for (int i = 0; i < (int)BuildTarget::Count; ++i)
        config.targets[i] = (i == (int)BuildTarget::DataFile);

    BuildSystem build;
    BuildResult result = build.BuildGame(project, config);
    ASSERT_TRUE(result.success);

    GameData *gd = project.GetGameData();
    ASSERT_NE(gd, nullptr);
    const std::string base_name =
        !gd->game_file_name.empty() ? gd->game_file_name : work.filename().string();
    const std::string ags_path = config.output_base_dir + "/Data/" + base_name + ".ags";

    const std::string extract_dir = (work / "extract").string();
    std::filesystem::create_directories(extract_dir);

    auto in = std::unique_ptr<Stream>(File::OpenFileRead(ags_path.c_str()));
    ASSERT_TRUE(in != nullptr);

    AssetLibInfo lib;
    ASSERT_EQ(MFLUtil::ReadHeader(lib, in.get()), MFLUtil::kMFLNoError);

    std::string dta_disk = extract_dir + "/" + kCompiledDtaFileName;
    bool found_dta = false;
    for (size_t i = 0; i < lib.AssetInfos.size(); ++i)
    {
        if (lib.AssetInfos[i].FileName.Compare(kCompiledDtaFileName) != 0)
            continue;
        found_dta = true;
        in->Seek(lib.AssetInfos[i].Offset, kSeekBegin);
        auto out = std::unique_ptr<Stream>(File::CreateFile(dta_disk.c_str()));
        ASSERT_TRUE(out != nullptr);
        ASSERT_EQ(CopyStream(in.get(), out.get(), lib.AssetInfos[i].Size),
                  lib.AssetInfos[i].Size);
        break;
    }
    ASSERT_TRUE(found_dta);

    MainGameSource src;
    HGameFileError open_err = OpenMainGameFile(dta_disk.c_str(), src);
    ASSERT_TRUE(open_err)
        << (open_err.HasError() ? open_err->FullMessage().GetCStr() : "unknown error");

    GameSetupStruct game;
    LoadedGameEntities ents(game);
    HGameFileError read_err = ReadGameData(ents, std::move(src.InputStream),
        src.DataVersion, src.CompiledWith);
    ASSERT_TRUE(read_err)
        << (read_err.HasError() ? read_err->FullMessage().GetCStr() : "unknown error");
}
